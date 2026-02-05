#include "nvs_helpers.h"
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <vector>


static const char *TAG = "nvs_helpers";

bool nvs_read_wifi_credentials(std::string &out_ssid, std::string &out_pass) {
  nvs_handle_t h;
  esp_err_t err = nvs_open("nvsparam", NVS_READONLY, &h);
  if (err != ESP_OK) {
    return false;
  }
  size_t required = 0;
  if (nvs_get_str(h, "wifi_ssid", NULL, &required) != ESP_OK) {
    nvs_close(h);
    return false;
  }
  std::vector<char> buf(required);
  if (nvs_get_str(h, "wifi_ssid", buf.data(), &required) != ESP_OK) {
    nvs_close(h);
    return false;
  }
  out_ssid.assign(buf.data());

  required = 0;
  if (nvs_get_str(h, "wifi_pass", NULL, &required) != ESP_OK) {
    nvs_close(h);
    return false;
  }
  buf.resize(required);
  if (nvs_get_str(h, "wifi_pass", buf.data(), &required) != ESP_OK) {
    nvs_close(h);
    return false;
  }
  out_pass.assign(buf.data());
  nvs_close(h);
  return true;
}

bool nvs_write_wifi_credentials(const char *ssid, const char *pass) {
  nvs_handle_t h;
  esp_err_t err = nvs_open("nvsparam", NVS_READWRITE, &h);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_open failed: %d", err);
    return false;
  }
  if ((err = nvs_set_str(h, "wifi_ssid", ssid)) != ESP_OK) {
    ESP_LOGW(TAG, "nvs_set_str(ssid) failed: %d", err);
    nvs_close(h);
    return false;
  }
  if ((err = nvs_set_str(h, "wifi_pass", pass)) != ESP_OK) {
    ESP_LOGW(TAG, "nvs_set_str(pass) failed: %d", err);
    nvs_close(h);
    return false;
  }
  if ((err = nvs_commit(h)) != ESP_OK) {
    ESP_LOGW(TAG, "nvs_commit failed: %d", err);
    nvs_close(h);
    return false;
  }
  nvs_close(h);
  return true;
}

// Shared handle for convenience (mimics lightweight behavior of
// NvsStorageClass)
static nvs_handle_t g_nvs_handle = 0;
static bool g_nvs_started = false;

bool nvs_begin() {
  if (g_nvs_started)
    return true;
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_flash_init failed: %d", err);
    return false;
  }
  err = nvs_open("nvsparam", NVS_READWRITE, &g_nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_open failed: %d", err);
    return false;
  }
  g_nvs_started = true;
  return true;
}

void nvs_end() {
  if (!g_nvs_started)
    return;
  nvs_close(g_nvs_handle);
  g_nvs_handle = 0;
  g_nvs_started = false;
}

bool nvs_clear_all() {
  if (!g_nvs_started && !nvs_begin())
    return false;
  esp_err_t err = nvs_erase_all(g_nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_erase_all failed: %d", err);
    return false;
  }
  err = nvs_commit(g_nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_commit failed: %d", err);
    return false;
  }
  return true;
}

size_t nvs_put_float(const char *key, float value) {
  if (!g_nvs_started && !nvs_begin())
    return 0;
  esp_err_t err = nvs_set_blob(g_nvs_handle, key, &value, sizeof(float));
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_set_blob(float) failed: %d", err);
    return 0;
  }
  err = nvs_commit(g_nvs_handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_commit failed: %d", err);
    return 0;
  }
  return sizeof(float);
}

float nvs_get_float(const char *key, float defaultValue) {
  if (!g_nvs_started && !nvs_begin())
    return defaultValue;
  float v = defaultValue;
  size_t required = 0;
  esp_err_t err = nvs_get_blob(g_nvs_handle, key, NULL, &required);
  if (err != ESP_OK || required != sizeof(float)) {
    return defaultValue;
  }
  err = nvs_get_blob(g_nvs_handle, key, &v, &required);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "nvs_get_blob(float) failed: %d", err);
    return defaultValue;
  }
  return v;
}

bool nvs_is_key(const char *key) {
  if (!g_nvs_started && !nvs_begin())
    return false;
  // Try to get any type; use nvs_get_blob len check
  size_t required = 0;
  esp_err_t err = nvs_get_blob(g_nvs_handle, key, NULL, &required);
  if (err == ESP_OK)
    return true;
  // try str
  err = nvs_get_str(g_nvs_handle, key, NULL, &required);
  return (err == ESP_OK);
}
