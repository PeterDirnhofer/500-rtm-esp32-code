#ifndef NVS_HELPERS_H
#define NVS_HELPERS_H

#include <string>

// Read WiFi credentials from NVS namespace "nvsparam".
// Returns true if both SSID and PASS were read successfully.
bool nvs_read_wifi_credentials(std::string &out_ssid, std::string &out_pass);

// Write WiFi credentials to NVS namespace "nvsparam".
// Returns true on success.
bool nvs_write_wifi_credentials(const char *ssid, const char *pass);

// Initialize NVS helpers (open namespace). Returns true on success.
bool nvs_begin();

// Close NVS namespace opened by nvs_begin.
void nvs_end();

// Erase all keys in the namespace. Returns true on success.
bool nvs_clear_all();

// Float helpers: store and retrieve a float value under `key`.
// `nvs_put_float` returns number of bytes written (sizeof(float)) on success, 0
// on failure.
size_t nvs_put_float(const char *key, float value);
// `nvs_get_float` returns value or `defaultValue` if not found.
float nvs_get_float(const char *key, float defaultValue);

// Check if a key exists.
bool nvs_is_key(const char *key);

#endif // NVS_HELPERS_H
