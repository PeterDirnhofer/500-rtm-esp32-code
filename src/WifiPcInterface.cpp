#include "WifiPcInterface.h"
#include "globalVariables.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <lwip/inet.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>
#include <mutex>
#include <nvs.h>
#include <nvs_flash.h>
#include <string>
#include <sys/socket.h>
#include <vector>

static const char *TAG = "WifiPcInterface";

static bool active = false;
static httpd_handle_t http_server = NULL;
// Per-client info: socket fd + outbound queue
struct ClientInfo {
  int fd;
  QueueHandle_t q;
};
static std::vector<ClientInfo> clients;
static std::mutex clients_mutex;

// Event group to signal when station has obtained an IP
static EventGroupHandle_t s_wifi_event_group = NULL;
#define WIFI_CONNECTED_BIT BIT0
static char s_ip_str[16] = {0};

// No raw fallback: use esp_http_server helper APIs only for WebSocket handling

// WiFi AP settings
static const char *AP_SSID = "ESP_Server";
static const char *AP_PASS = "esp12345";

static esp_err_t send_to_all_clients(const char *msg) {
  std::lock_guard<std::mutex> lock(clients_mutex);
  for (auto it = clients.begin(); it != clients.end();) {
    ClientInfo &ci = *it;
    if (ci.q == NULL && ci.fd < 0) {
      ++it;
      continue;
    }

    // Try to send directly to socket first to deliver the complete line
    bool sent_direct = false;
    bool remove_client = false;
    if (ci.fd >= 0) {
      const char *outmsg = msg;
      size_t outlen = strlen(outmsg);
      uint8_t whdr[10];
      size_t whdr_len = 0;
      whdr[0] = 0x81; // FIN + text
      if (outlen <= 125) {
        whdr[1] = outlen;
        whdr_len = 2;
      } else if (outlen <= 0xFFFF) {
        whdr[1] = 126;
        whdr[2] = (outlen >> 8) & 0xFF;
        whdr[3] = outlen & 0xFF;
        whdr_len = 4;
      } else {
        whdr[1] = 127;
        for (int i = 0; i < 8; ++i)
          whdr[9 - i] = (outlen >> (8 * i)) & 0xFF;
        whdr_len = 10;
      }

      // Attempt header then payload; if send would block, fall back to queue
      ssize_t rh = send(ci.fd, whdr, whdr_len, MSG_DONTWAIT);
      if (rh >= 0) {
        ssize_t rp = 0;
        if (outlen > 0)
          rp = send(ci.fd, outmsg, outlen, MSG_DONTWAIT);
        if ((outlen == 0 && rh == (ssize_t)whdr_len) ||
            (rp >= 0 && (size_t)rp == outlen)) {
          sent_direct = true;
        }
      } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          // transient, will try to queue below
        } else {
          ESP_LOGW(TAG, "direct send failed (fd=%d): %d -- removing client",
                   ci.fd, errno);
          remove_client = true;
        }
      }
    }

    if (sent_direct) {
      ++it;
      continue;
    }

    if (remove_client) {
      // Clean up queue if present, close socket and remove client
      if (ci.q) {
        vQueueDelete(ci.q);
      }
      if (ci.fd >= 0) {
        shutdown(ci.fd, SHUT_WR);
      }
      it = clients.erase(it);
      continue;
    }

    // Fallback: enqueue into client's queue so the worker will send it
    if (ci.q != NULL) {
      char buf[512];
      snprintf(buf, sizeof(buf), "%s", msg);
      if (xQueueSend(ci.q, buf, pdMS_TO_TICKS(50)) != pdPASS) {
        ESP_LOGW(TAG, "send_to_all_clients: queue full, dropped message");
      }
    }
    ++it;
  }
  return ESP_OK;
}

static esp_err_t post_send_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  // Trim newline, whitespace
  std::string s(buf);
  s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());

  // ensure queueFromPc exists
  if (queueFromPc == NULL) {
    queueFromPc = xQueueCreate(2, sizeof(char) * 255);
  }

  if (queueFromPc != NULL) {
    if (xQueueSend(queueFromPc, s.c_str(), pdMS_TO_TICKS(50)) != pdPASS) {
      ESP_LOGW(TAG, "Failed to enqueue incoming WiFi command");
    }
  }

  httpd_resp_set_status(req, "200 OK");
  httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
  return ESP_OK;
}

// Raw worker arg used by queued worker
struct RawWsArg {
  httpd_handle_t hd;
  int fd;
  QueueHandle_t q;
};

static void raw_ws_worker(void *a) {
  RawWsArg *ra = (RawWsArg *)a;
  int client_sock = ra->fd;
  QueueHandle_t clientQ_local = ra->q;

  // set recv timeout (shorter so outbound queue is serviced promptly)
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 20000; // 20 ms
  setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv,
             sizeof(tv));

  // Loop: read frames and echo or enqueue commands
  char buf[1024];
  while (1) {
    uint8_t hdr[2];
    ssize_t n = recv(client_sock, hdr, 2, 0);
    if (n == 0)
      break;
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data from socket right now — check outgoing queue for messages
        if (clientQ_local) {
          char outmsg[256];
          if (xQueueReceive(clientQ_local, outmsg, 0) == pdPASS) {
            size_t outlen = strlen(outmsg);
            uint8_t whdr[10];
            size_t whdr_len = 0;
            whdr[0] = 0x81; // FIN + text opcode
            if (outlen <= 125) {
              whdr[1] = outlen;
              whdr_len = 2;
            } else if (outlen <= 0xFFFF) {
              whdr[1] = 126;
              whdr[2] = (outlen >> 8) & 0xFF;
              whdr[3] = outlen & 0xFF;
              whdr_len = 4;
            } else {
              whdr[1] = 127;
              for (int i = 0; i < 8; ++i)
                whdr[9 - i] = (outlen >> (8 * i)) & 0xFF;
              whdr_len = 10;
            }
            send(client_sock, whdr, whdr_len, 0);
            if (outlen)
              send(client_sock, outmsg, outlen, 0);
            continue; // go back to recv loop
          }
        }
        // small delay to yield CPU but keep low latency for outbound sends
        vTaskDelay(pdMS_TO_TICKS(1));
        continue;
      }
      break;
    }
    uint8_t b1 = hdr[0];
    uint8_t b2 = hdr[1];
    uint8_t opcode = b1 & 0x0F;
    bool masked = (b2 & 0x80) != 0;
    uint64_t payload_len = b2 & 0x7F;
    if (payload_len == 126) {
      uint8_t ext[2];
      if (recv(client_sock, ext, 2, 0) != 2)
        break;
      payload_len = (ext[0] << 8) | ext[1];
    } else if (payload_len == 127) {
      uint8_t ext[8];
      if (recv(client_sock, ext, 8, 0) != 8)
        break;
      payload_len = 0;
      for (int i = 0; i < 8; ++i)
        payload_len = (payload_len << 8) | ext[i];
    }
    uint8_t mask[4] = {0};
    if (masked) {
      if (recv(client_sock, mask, 4, 0) != 4)
        break;
    }
    if (payload_len > sizeof(buf) - 1)
      payload_len = sizeof(buf) - 1;
    size_t got = 0;
    while (got < payload_len) {
      ssize_t r = recv(client_sock, buf + got, payload_len - got, 0);
      if (r <= 0)
        break;
      got += r;
    }
    if (got < payload_len)
      break;
    if (masked) {
      for (size_t i = 0; i < payload_len; ++i)
        buf[i] ^= mask[i % 4];
    }
    buf[payload_len] = '\0';

    if (opcode == 0x8)
      break;
    if (opcode == 0x9) {
      // send pong
      uint8_t whdr[10];
      size_t whdr_len = 0;
      whdr[0] = 0x8A;
      if (payload_len <= 125) {
        whdr[1] = payload_len;
        whdr_len = 2;
      } else if (payload_len <= 0xFFFF) {
        whdr[1] = 126;
        whdr[2] = (payload_len >> 8) & 0xFF;
        whdr[3] = payload_len & 0xFF;
        whdr_len = 4;
      } else {
        whdr[1] = 127;
        for (int i = 0; i < 8; ++i)
          whdr[9 - i] = (payload_len >> (8 * i)) & 0xFF;
        whdr_len = 10;
      }
      send(client_sock, whdr, whdr_len, 0);
      if (payload_len)
        send(client_sock, buf, payload_len, 0);
      continue;
    }
    if (opcode == 0x1) {
      ESP_LOGI(TAG, "raw worker: TEXT '%s'", buf);
      std::string s(buf);
      s.erase(std::remove_if(s.begin(), s.end(), ::isspace), s.end());
      if (!s.empty()) {
        if (queueFromPc == NULL)
          queueFromPc = xQueueCreate(2, sizeof(char) * 255);
        if (queueFromPc)
          xQueueSend(queueFromPc, s.c_str(), pdMS_TO_TICKS(50));
      }
      // echo back
      size_t outlen = payload_len;
      uint8_t whdr[10];
      size_t whdr_len = 0;
      whdr[0] = 0x81;
      if (outlen <= 125) {
        whdr[1] = outlen;
        whdr_len = 2;
      } else if (outlen <= 0xFFFF) {
        whdr[1] = 126;
        whdr[2] = (outlen >> 8) & 0xFF;
        whdr[3] = outlen & 0xFF;
        whdr_len = 4;
      } else {
        whdr[1] = 127;
        for (int i = 0; i < 8; ++i)
          whdr[9 - i] = (outlen >> (8 * i)) & 0xFF;
        whdr_len = 10;
      }
      send(client_sock, whdr, whdr_len, 0);
      if (outlen)
        send(client_sock, buf, outlen, 0);
    }
  }

  // cleanup: shutdown write side and let httpd close the socket to avoid
  // leaving httpd with an invalid FD in its select() loop.
  shutdown(client_sock, SHUT_WR);
  if (clientQ_local) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove_if(clients.begin(), clients.end(),
                                 [&](const ClientInfo &ci) {
                                   return ci.q == clientQ_local;
                                 }),
                  clients.end());
    vQueueDelete(clientQ_local);
  }
  free(ra);
}

static esp_err_t ws_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "ws_handler: new connection (using example-style worker)");

  // Log common headers for debugging the upgrade request
  const char *hdrs[] = {"Host",
                        "Upgrade",
                        "Connection",
                        "Sec-WebSocket-Key",
                        "Sec-WebSocket-Version",
                        "Origin",
                        "User-Agent",
                        "Sec-WebSocket-Protocol"};
  char hv[128];
  for (size_t i = 0; i < sizeof(hdrs) / sizeof(hdrs[0]); ++i) {
    if (httpd_req_get_hdr_value_str(req, hdrs[i], hv, sizeof(hv)) == ESP_OK) {
      ESP_LOGI(TAG, "WS Header %s: %s", hdrs[i], hv);
    } else {
      ESP_LOGI(TAG, "WS Header %s: <missing>", hdrs[i]);
    }
  }

  // Extract Sec-WebSocket-Key
  char keybuf[128] = {0};
  if (httpd_req_get_hdr_value_str(req, "Sec-WebSocket-Key", keybuf,
                                  sizeof(keybuf)) != ESP_OK) {
    ESP_LOGW(TAG, "ws_handler: no Sec-WebSocket-Key header, rejecting");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "Missing Sec-WebSocket-Key");
    return ESP_FAIL;
  }

  // Compute Sec-WebSocket-Accept
  const char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  char concat[256];
  snprintf(concat, sizeof(concat), "%s%s", keybuf, GUID);
  unsigned char sha[20];
  mbedtls_sha1((const unsigned char *)concat, strlen(concat), sha);
  unsigned char b64[64];
  size_t olen = 0;
  mbedtls_base64_encode(b64, sizeof(b64), &olen, sha, sizeof(sha));

  // Send raw 101 Switching Protocols response using httpd_socket_send
  int sockfd = httpd_req_to_sockfd(req);
  if (sockfd < 0) {
    ESP_LOGW(TAG, "ws_handler: failed to get sockfd");
    return ESP_FAIL;
  }

  char resp[256];
  int rlen = snprintf(resp, sizeof(resp),
                      "HTTP/1.1 101 Switching Protocols\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Accept: %.*s\r\n\r\n",
                      (int)olen, b64);
  httpd_socket_send(req->handle, sockfd, resp, rlen, 0);

  // Create a per-client queue for future outbound messages (optional)
  // Larger per-client queue to handle bursts (e.g., measure outputs)
  QueueHandle_t clientQ = xQueueCreate(32, 512);
  if (clientQ == NULL) {
    ESP_LOGW(TAG, "ws_handler: failed to create client queue");
  } else {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(ClientInfo{sockfd, clientQ});
  }

  // Prepare worker arg and schedule raw socket worker via httpd_queue_work
  RawWsArg *arg = (RawWsArg *)malloc(sizeof(RawWsArg));
  if (!arg) {
    ESP_LOGW(TAG, "ws_handler: malloc failed for RawWsArg");
    return ESP_FAIL;
  }
  arg->hd = req->handle;
  arg->fd = sockfd;
  arg->q = clientQ;

  if (httpd_queue_work(req->handle, raw_ws_worker, arg) != ESP_OK) {
    ESP_LOGW(TAG, "ws_handler: httpd_queue_work failed");
    if (clientQ) {
      std::lock_guard<std::mutex> lock(clients_mutex);
      clients.erase(
          std::remove_if(clients.begin(), clients.end(),
                         [&](const ClientInfo &ci) { return ci.q == clientQ; }),
          clients.end());
      vQueueDelete(clientQ);
    }
    free(arg);
    return ESP_FAIL;
  }

  // Return ESP_OK — worker handles socket from here
  return ESP_OK;
}

static httpd_uri_t send_uri;
static httpd_uri_t ws_uri;
static httpd_uri_t setwifi_uri;

// Initialize and register HTTP URI handlers (centralized to avoid duplication)
static void init_and_register_uris(httpd_handle_t server) {
  // populate send_uri
  memset(&send_uri, 0, sizeof(send_uri));
  send_uri.uri = "/send";
  send_uri.method = HTTP_POST;
  send_uri.handler = post_send_handler;
  send_uri.user_ctx = NULL;

  // populate ws_uri
  memset(&ws_uri, 0, sizeof(ws_uri));
  ws_uri.uri = "/ws";
  ws_uri.method = HTTP_GET;
  ws_uri.handler = ws_handler;
  ws_uri.user_ctx = NULL;
  ws_uri.is_websocket = false;
  ws_uri.handle_ws_control_frames = 0;
  ws_uri.supported_subprotocol = NULL;

  httpd_register_uri_handler(server, &send_uri);
  httpd_register_uri_handler(server, &ws_uri);
  // handler to receive new WiFi credentials for provisioning
  memset(&setwifi_uri, 0, sizeof(setwifi_uri));
  setwifi_uri.uri = "/setwifi";
  setwifi_uri.method = HTTP_POST;
  setwifi_uri.handler = [](httpd_req_t *req) -> esp_err_t {
    // delegate to concrete handler below
    extern esp_err_t post_setwifi_handler(httpd_req_t * req);
    return post_setwifi_handler(req);
  };
  setwifi_uri.user_ctx = NULL;
  httpd_register_uri_handler(server, &setwifi_uri);
}

// Use lightweight NVS helpers (see include/nvs_helpers.h)
#include "nvs_helpers.h"

// Attempt a single STA connect attempt with a timeout (ms). Returns true if got
// IP.
static bool try_connect_sta_once(const char *ssid, const char *password,
                                 uint32_t timeout_ms) {
  // Ensure station netif exists
  esp_netif_create_default_wifi_sta();

  wifi_config_t sta_config = {};
  strncpy((char *)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid));
  strncpy((char *)sta_config.sta.password, password,
          sizeof(sta_config.sta.password));
  sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  esp_wifi_set_mode(WIFI_MODE_APSTA); // keep AP if already running
  esp_wifi_set_config(WIFI_IF_STA, &sta_config);
  esp_wifi_start();
  esp_wifi_connect();

  if (s_wifi_event_group == NULL) {
    s_wifi_event_group = xEventGroupCreate();
  }

  EventBits_t bits =
      xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE,
                          pdFALSE, pdMS_TO_TICKS(timeout_ms));
  return (bits & WIFI_CONNECTED_BIT) != 0;
}

// Handler to receive and validate credentials, then persist them on success
esp_err_t post_setwifi_handler(httpd_req_t *req) {
  char buf[256];
  int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
  if (ret <= 0) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  buf[ret] = '\0';

  // Expect body in form: ssid=...&pass=...
  std::string body(buf);
  auto find_kv = [&](const std::string &k) -> std::string {
    size_t p = body.find(k + "=");
    if (p == std::string::npos)
      return std::string();
    p += k.size() + 1;
    size_t e = body.find('&', p);
    if (e == std::string::npos)
      e = body.size();
    return body.substr(p, e - p);
  };

  std::string ssid = find_kv("ssid");
  std::string pass = find_kv("pass");
  if (ssid.empty()) {
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Provisioning: received SSID='%s' PASS='%s'", ssid.c_str(),
           pass.c_str());

  bool ok = try_connect_sta_once(ssid.c_str(), pass.c_str(), 10000);
  if (ok) {
    if (!nvs_write_wifi_credentials(ssid.c_str(), pass.c_str())) {
      ESP_LOGW(TAG, "Provisioning: connected but failed to save credentials");
    } else {
      ESP_LOGI(TAG, "Provisioning: credentials saved to NVS");
    }
    // Try to switch to STA-only mode
    esp_wifi_set_mode(WIFI_MODE_STA);
    httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
  } else {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  // retry counter to detect repeated failures
  static int s_retry_num = 0;
  const int MAX_RETRY = 10;

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    s_retry_num = 0;
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    s_retry_num++;
    ESP_LOGW(TAG, "Disconnected from AP. Reconnecting (attempt %d)...",
             s_retry_num);
    esp_wifi_connect();
    if (s_retry_num >= MAX_RETRY) {
      ESP_LOGW(TAG, "Could not connect as station after %d attempts",
               s_retry_num);
      s_retry_num = 0; // reset to avoid spamming
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    // store IP and signal waiting task
    snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
  }
}

void WifiPcInterface::startStation(const char *ssid, const char *password) {
  if (active)
    return;
  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  esp_netif_init();
  esp_event_loop_create_default();

  // Register event handlers for WiFi and IP events
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                             NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                             NULL);

  if (s_wifi_event_group == NULL) {
    s_wifi_event_group = xEventGroupCreate();
  }

  // Initialize WiFi driver
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  // 1) Try saved credentials from NVS first (if present)
  std::string saved_ssid, saved_pass;
  const char *use_ssid = ssid;
  const char *use_pass = password;
  if (nvs_read_wifi_credentials(saved_ssid, saved_pass)) {
    ESP_LOGI(TAG, "Found saved WiFi credentials, trying saved SSID '%s'",
             saved_ssid.c_str());
    use_ssid = saved_ssid.c_str();
    use_pass = saved_pass.c_str();
  }

  // Configure STA and attempt connect (short attempt)
  wifi_config_t sta_config = {};
  strncpy((char *)sta_config.sta.ssid, use_ssid, sizeof(sta_config.sta.ssid));
  strncpy((char *)sta_config.sta.password, use_pass,
          sizeof(sta_config.sta.password));
  sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  // create default WiFi station netif and apply config
  esp_netif_create_default_wifi_sta();
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &sta_config);

  esp_wifi_start();

  // Wait up to 10s for a connection
  EventBits_t bits =
      xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE,
                          pdFALSE, pdMS_TO_TICKS(10000));
  if (bits & WIFI_CONNECTED_BIT) {
    // Success: start HTTP server and enable services
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&http_server, &config) == ESP_OK) {
      init_and_register_uris(http_server);
      active = true;
      ESP_LOGI(TAG, "HTTP server started (STA mode)");
      ESP_LOGI(TAG, "Started WiFi STA: '%s'", use_ssid);
    } else {
      ESP_LOGE(TAG, "Failed to start HTTP server");
    }
  } else {
    // Failed to connect as STA -> fall back to AP provisioning
    ESP_LOGW(
        TAG,
        "Failed to connect as STA (tried '%s'), starting AP for provisioning",
        use_ssid);
    // Stop wifi to ensure AP start() does its own init
    esp_wifi_stop();
    start();
  }

  if (queueFromPc == NULL) {
    queueFromPc = xQueueCreate(2, sizeof(char) * 255);
  }
}

void WifiPcInterface::start() {
  if (active)
    return;

  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  // Initialize TCP/IP stack and default event loop
  esp_netif_init();
  esp_event_loop_create_default();

  // Create default AP
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  wifi_config_t ap_config = {};
  strncpy((char *)ap_config.ap.ssid, AP_SSID, sizeof(ap_config.ap.ssid));
  ap_config.ap.ssid_len = strlen(AP_SSID);
  ap_config.ap.channel = 1;
  ap_config.ap.max_connection = 4;
  strncpy((char *)ap_config.ap.password, AP_PASS,
          sizeof(ap_config.ap.password));
  ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;

  esp_wifi_set_mode(WIFI_MODE_AP);
  esp_wifi_set_config(WIFI_IF_AP, &ap_config);
  esp_wifi_start();

  ESP_LOGI(TAG, "Started AP '%s' IP='192.168.4.1' SSID='%s' PASS='%s'", AP_SSID,
           AP_SSID, AP_PASS);

  // Start HTTP server
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  if (httpd_start(&http_server, &config) == ESP_OK) {
    init_and_register_uris(http_server);
    active = true;
    ESP_LOGI(TAG, "HTTP server started");
    // Note: raw WebSocket listener removed — using httpd helper only
  } else {
    ESP_LOGE(TAG, "Failed to start HTTP server");
  }

  // Ensure queueFromPc exists so dispatcher can read incoming WiFi commands
  if (queueFromPc == NULL) {
    queueFromPc = xQueueCreate(2, sizeof(char) * 255);
  }
}

int WifiPcInterface::send(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char s[200];
  vsnprintf(s, sizeof(s), fmt, ap);
  va_end(ap);

  // Also send to connected SSE clients
  if (active) {
    send_to_all_clients(s);
  }

  return 0; // Return value not used
}

bool WifiPcInterface::isActive() { return active; }

// Raw TCP WebSocket listener removed; WebSocket handling is now helper-only
