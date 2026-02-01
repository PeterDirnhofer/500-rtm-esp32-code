#include "WifiPcInterface.h"
#include "globalVariables.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <esp_event.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <lwip/inet.h>
#include <mutex>
#include <string>
#include <vector>

static const char *TAG = "WifiPcInterface";

static bool active = false;
static httpd_handle_t http_server = NULL;
static std::vector<QueueHandle_t> clients;
static std::mutex clients_mutex;

// WiFi AP settings
static const char *AP_SSID = "ESP_Server";
static const char *AP_PASS = "esp12345";

static esp_err_t send_to_all_clients(const char *msg) {
  std::lock_guard<std::mutex> lock(clients_mutex);
  for (auto q : clients) {
    if (q != NULL) {
      // make a local copy
      char buf[256];
      snprintf(buf, sizeof(buf), "data: %s\n\n", msg);
      xQueueSend(q, buf, 0);
    }
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

static esp_err_t sse_events_handler(httpd_req_t *req) {
  // Set headers for SSE
  httpd_resp_set_type(req, "text/event-stream");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
  httpd_resp_set_hdr(req, "Connection", "keep-alive");

  // Create a queue for this client
  QueueHandle_t clientQ = xQueueCreate(10, 256);
  if (clientQ == NULL) {
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(clientQ);
  }

  // Keep sending chunks when messages arrive for this client
  char outBuf[512];
  while (1) {
    // Wait for a message destined to this client
    if (xQueueReceive(clientQ, outBuf, portMAX_DELAY) == pdPASS) {
      size_t len = strlen(outBuf);
      esp_err_t rc = httpd_resp_send_chunk(req, outBuf, len);
      if (rc != ESP_OK) {
        break; // client disconnected or error
      }
    } else {
      break;
    }
  }

  // Cleanup
  httpd_resp_send_chunk(req, NULL, 0);
  {
    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.erase(std::remove(clients.begin(), clients.end(), clientQ),
                  clients.end());
  }
  vQueueDelete(clientQ);
  return ESP_OK;
}

static httpd_uri_t send_uri = {.uri = "/send",
                               .method = HTTP_POST,
                               .handler = post_send_handler,
                               .user_ctx = NULL};

static httpd_uri_t events_uri = {.uri = "/events",
                                 .method = HTTP_GET,
                                 .handler = sse_events_handler,
                                 .user_ctx = NULL};

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
    ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
  }
}

void WifiPcInterface::startStation(const char *ssid, const char *password) {
  if (active)
    return;
  esp_log_level_set(TAG, ESP_LOG_INFO);

  esp_netif_init();
  esp_event_loop_create_default();

  // Create default WiFi station
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  // Register event handlers for WiFi and IP events
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler,
                             NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler,
                             NULL);

  wifi_config_t sta_config = {};
  strncpy((char *)sta_config.sta.ssid, ssid, sizeof(sta_config.sta.ssid));
  strncpy((char *)sta_config.sta.password, password,
          sizeof(sta_config.sta.password));
  sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(WIFI_IF_STA, &sta_config);
  esp_wifi_start();

  ESP_LOGI(TAG, "Started WiFi STA: '%s' (attempting connect)", ssid);

  // Start HTTP server so services are available once connected
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  if (httpd_start(&http_server, &config) == ESP_OK) {
    httpd_register_uri_handler(http_server, &send_uri);
    httpd_register_uri_handler(http_server, &events_uri);
    active = true;
    ESP_LOGI(TAG, "HTTP server started (STA mode)");
  } else {
    ESP_LOGE(TAG, "Failed to start HTTP server");
  }

  if (queueFromPc == NULL) {
    queueFromPc = xQueueCreate(2, sizeof(char) * 255);
  }
}

void WifiPcInterface::start() {
  if (active)
    return;

  esp_log_level_set(TAG, ESP_LOG_INFO);

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

  ESP_LOGI(TAG, "Started AP '%s' (AP IP: 192.168.4.1)", AP_SSID);

  // Start HTTP server
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  if (httpd_start(&http_server, &config) == ESP_OK) {
    httpd_register_uri_handler(http_server, &send_uri);
    httpd_register_uri_handler(http_server, &events_uri);
    active = true;
    ESP_LOGI(TAG, "HTTP server started");
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
