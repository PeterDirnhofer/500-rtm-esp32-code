// NvsStorageClass.cpp

// Copyright 2015-2021 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences/src

#include "NvsStorageClass.h"

static const char *TAG = "preferences";
const char *nvs_errors[] = {"OTHER", "NOT_INITIALIZED", "NOT_FOUND", "TYPE_MISMATCH", "READ_ONLY", "NOT_ENOUGH_SPACE", "INVALID_NAME", "INVALID_HANDLE", "REMOVE_FAILED", "KEY_TOO_LONG", "PAGE_FULL", "INVALID_STATE", "INVALID_LENGTH"};

#define nvs_error(e) (((e) > ESP_ERR_NVS_BASE) ? nvs_errors[(e) & ~(ESP_ERR_NVS_BASE)] : nvs_errors[0])
#define STORAGE_NAMESPACE "nvsparam"

NvsStorageClass::NvsStorageClass()
    : mHandle(0), mStarted(false), mReadOnly(false)
{
}

NvsStorageClass::~NvsStorageClass()
{
    end();
}

bool NvsStorageClass::begin()
{
    if (mStarted)
    {
        ESP_LOGI(TAG, "nvs_arleaady started\n");
        return false;
    }
    mReadOnly = false;

    // Initialize NVS
    // https://github.com/espressif/esp-idf/blob/master/examples/storage/nvs_rw_value/main/nvs_value_example_main.c
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was trucated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "%s", esp_err_to_name(err));
        return false;
    }

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &mHandle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "%s", esp_err_to_name(err));
        return false;
    }
    ESP_LOGI(TAG, "+++ nvs_flash opened\n");

    mStarted = true;
    return true;
}

void NvsStorageClass::end()
{
    if (!mStarted)
    {
        ESP_LOGE(TAG, "nvs_not started or _readOnly or \n");
        return;
    }
    nvs_close(mHandle);
    mStarted = false;
}

bool NvsStorageClass::clear()
{
    if (!mStarted || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started or _readOnly or \n");
        return false;
    }
    esp_err_t err = nvs_erase_all(mHandle);

    if (err)
    {
        ESP_LOGE(TAG, "nvs_erase_all fail: %s", nvs_error(err));
        return false;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s", nvs_error(err));
        return false;
    }
    return true;
}

bool NvsStorageClass::remove(const char *key)
{
    if (!mStarted || !key || mReadOnly)
    {
        return false;
    }
    esp_err_t err = nvs_erase_key(mHandle, key);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_erase_key fail: %s %s", key, nvs_error(err));
        return false;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return false;
    }
    return true;
}

// private ******************** P

/*
 * Put a key value
 * */
float NvsStorageClass::putFloat(const char *key, const float value)
{
    return putBytes(key, (void *)&value, sizeof(float));
}

double NvsStorageClass::putDouble(const char *key, const double value)
{
    return putBytes(key, (void *)&value, sizeof(double));
}

size_t NvsStorageClass::putChar(const char *key, int8_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }

    esp_err_t err = nvs_set_i8(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_i8 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 1;
}

size_t NvsStorageClass::putUChar(const char *key, uint8_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_u8(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_u8 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 1;
}

size_t NvsStorageClass::putShort(const char *key, int16_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_i16(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_i16 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 2;
}

size_t NvsStorageClass::putUShort(const char *key, uint16_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_u16(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_u16 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 2;
}

size_t NvsStorageClass::putInt(const char *key, int32_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_i32(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_i32 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 4;
}

size_t NvsStorageClass::putUInt(const char *key, uint32_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_u32(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_u32 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 4;
}

size_t NvsStorageClass::putLong(const char *key, int32_t value)
{
    return putInt(key, value);
}

size_t NvsStorageClass::putULong(const char *key, uint32_t value)
{
    return putUInt(key, value);
}

size_t NvsStorageClass::putLong64(const char *key, int64_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_i64(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_i64 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 8;
}

size_t NvsStorageClass::putULong64(const char *key, uint64_t value)
{
    if (!mStarted || !key || mReadOnly)
    {
        ESP_LOGE(TAG, "nvs_not started in printWhatSaved\n");
        return 0;
    }
    esp_err_t err = nvs_set_u64(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_u64 fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return 8;
}

size_t NvsStorageClass::putBool(const char *key, const bool value)
{
    return putUChar(key, (uint8_t)(value ? 1 : 0));
}

size_t NvsStorageClass::putBytes(const char *key, const void *value, size_t len)
{
    if (!mStarted || !key || !value || !len || mReadOnly)
    {
        return 0;
    }

    esp_err_t err = nvs_set_blob(mHandle, key, value, len);
    if (err)
    {

        ESP_LOGE(TAG, "nvs_set_blob fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t NvsStorageClass::putString(const char *key, const char *value)
{
    if (mStarted || !key || !value || mReadOnly)
    {
        return 0;
    }
    esp_err_t err = nvs_set_str(mHandle, key, value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_set_str fail: %s %s", key, nvs_error(err));
        return 0;
    }
    err = nvs_commit(mHandle);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_commit fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return strlen(value);
}

size_t NvsStorageClass::putString(const char *key, const string value)
{
    return putString(key, value.c_str());
}

PreferenceType NvsStorageClass::getType(const char *key)
{
    if (!mStarted || !key || strlen(key) > 15)
    {
        return PT_INVALID;
    }
    int8_t mt1;
    uint8_t mt2;
    int16_t mt3;
    uint16_t mt4;
    int32_t mt5;
    uint32_t mt6;
    int64_t mt7;
    uint64_t mt8;
    size_t len = 0;
    if (nvs_get_i8(mHandle, key, &mt1) == ESP_OK)
        return PT_I8;
    if (nvs_get_u8(mHandle, key, &mt2) == ESP_OK)
        return PT_U8;
    if (nvs_get_i16(mHandle, key, &mt3) == ESP_OK)
        return PT_I16;
    if (nvs_get_u16(mHandle, key, &mt4) == ESP_OK)
        return PT_U16;
    if (nvs_get_i32(mHandle, key, &mt5) == ESP_OK)
        return PT_I32;
    if (nvs_get_u32(mHandle, key, &mt6) == ESP_OK)
        return PT_U32;
    if (nvs_get_i64(mHandle, key, &mt7) == ESP_OK)
        return PT_I64;
    if (nvs_get_u64(mHandle, key, &mt8) == ESP_OK)
        return PT_U64;
    if (nvs_get_str(mHandle, key, NULL, &len) == ESP_OK)
        return PT_STR;
    if (nvs_get_blob(mHandle, key, NULL, &len) == ESP_OK)
        return PT_BLOB;
    return PT_INVALID;
}

bool NvsStorageClass::isKey(const char *key)
{
    return getType(key) != PT_INVALID;
}

/*
 * Get a key value
 * */

int8_t NvsStorageClass::getChar(const char *key, const int8_t defaultValue)
{
    int8_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_i8(mHandle, key, &value);
    if (err)
    {

        // ESP_LOGE(TAG,"\n+++ START ++++++++++++\n");
        ESP_LOGE(TAG, "nvs_get_i8 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

uint8_t NvsStorageClass::getUChar(const char *key, const uint8_t defaultValue)
{
    uint8_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_u8(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_u8 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

int16_t NvsStorageClass::getShort(const char *key, const int16_t defaultValue)
{
    int16_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_i16(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_i16 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

uint16_t NvsStorageClass::getUShort(const char *key, const uint16_t defaultValue)
{
    uint16_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_u16(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_u16 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

int32_t NvsStorageClass::getInt(const char *key, const int32_t defaultValue)
{
    int32_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_i32(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_i32 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

uint32_t NvsStorageClass::getUInt(const char *key, const uint32_t defaultValue)
{
    uint32_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_u32(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_u32 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

int32_t NvsStorageClass::getLong(const char *key, const int32_t defaultValue)
{
    return getInt(key, defaultValue);
}

uint32_t NvsStorageClass::getULong(const char *key, const uint32_t defaultValue)
{
    return getUInt(key, defaultValue);
}

int64_t NvsStorageClass::getLong64(const char *key, const int64_t defaultValue)
{
    int64_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_i64(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_i64 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

uint64_t NvsStorageClass::getULong64(const char *key, const uint64_t defaultValue)
{
    uint64_t value = defaultValue;
    if (!mStarted || !key)
    {
        return value;
    }
    esp_err_t err = nvs_get_u64(mHandle, key, &value);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_u64 fail: %s %s", key, nvs_error(err));
    }
    return value;
}

bool NvsStorageClass::getBool(const char *key, const bool defaultValue)
{
    return getUChar(key, defaultValue ? 1 : 0) == 1;
}

float NvsStorageClass::getFloat(const char *key, const float defaultValue)
{
    float value = defaultValue;
    getBytes(key, (void *)&value, sizeof(float));
    return value;
}

double NvsStorageClass::getDouble(const char *key, const double defaultValue)
{
    double value = defaultValue;
    getBytes(key, (void *)&value, sizeof(double));
    return value;
}

size_t NvsStorageClass::getBytesLength(const char *key)
{
    size_t len = 0;
    if (!mStarted || !key)
    {
        return 0;
    }
    esp_err_t err = nvs_get_blob(mHandle, key, NULL, &len);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_blob len fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t NvsStorageClass::getBytes(const char *key, void *buf, size_t maxLen)
{
    size_t len = getBytesLength(key);
    if (!len || !buf || !maxLen)
    {
        return len;
    }
    if (len > maxLen)
    {
        ESP_LOGE(TAG, "not enough space in buffer: %u < %u", maxLen, len);
        return 0;
    }
    esp_err_t err = nvs_get_blob(mHandle, key, buf, &len);
    if (err)
    {
        ESP_LOGE(TAG, "nvs_get_blob fail: %s %s", key, nvs_error(err));
        return 0;
    }
    return len;
}

size_t NvsStorageClass::freeEntries()
{
    nvs_stats_t nvs_stats;
    esp_err_t err = nvs_get_stats(NULL, &nvs_stats);
    if (err)
    {
        ESP_LOGE(TAG, "Failed to get nvs statistics");
        return 0;
    }
    return nvs_stats.free_entries;
}
