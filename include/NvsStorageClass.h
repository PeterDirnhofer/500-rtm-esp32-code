// NvsStorageClass.h

// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
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

#ifndef _NVS_STORAGE_CLASS_
#define _NVS_STORAGE_CLASS_

#include <stdio.h>
#include <cstring>
#include <vector>
#include <esp_log.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "UsbPcInterface.h"

// Copy from
// https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences/src
typedef enum
{
    PT_I8,
    PT_U8,
    PT_I16,
    PT_U16,
    PT_I32,
    PT_U32,
    PT_I64,
    PT_U64,
    PT_STR,
    PT_BLOB,
    PT_INVALID
} PreferenceType;

/**
 * @brief Store data in non volatile ESP NVS memory
 * For many datatypes exists a Put and a Get method
 */
class NvsStorageClass
{
public:
    bool clear();
    bool remove(const char *key);

    // visible in parent ParameterSetiing. Not in main
protected:
    nvs_handle_t mHandle;
    bool mStarted;
    bool mReadOnly;
    bool begin();
    void end();

    NvsStorageClass();
    ~NvsStorageClass();

    // bool begin(const char * name, bool readOnly=false, const char* partition_label=NULL);

    size_t putBool(const char *key, bool value);
    // float putFloat(const char *key, const float value);
    double putDouble(const char *key, const double value);
    size_t putString(const char *key, const char *value);
    size_t putString(const char *key, const string value);

    bool getBool(const char *key, bool defaultValue = false);
    // float getFloat(const char *key, const float defaultValue);
    double getDouble(const char *key, const double defaultValue);
    string getString(const char *key, char *value, size_t maxLen);
    bool isKey(const char *key);

public:
    float putFloat(const char *key, const float value);
    float getFloat(const char *key, const float defaultValue);

private:
    size_t putChar(const char *key, int8_t value);
    size_t putUChar(const char *key, uint8_t value);
    size_t putShort(const char *key, int16_t value);
    size_t putUShort(const char *key, uint16_t value);
    size_t putInt(const char *key, int32_t value);
    size_t putUInt(const char *key, uint32_t value);
    size_t putLong(const char *key, int32_t value);
    size_t putULong(const char *key, uint32_t value);
    size_t putLong64(const char *key, int64_t value);
    size_t putULong64(const char *key, uint64_t value);

    size_t putBytes(const char *key, const void *value, size_t len);

    PreferenceType getType(const char *key);
    int8_t getChar(const char *key, int8_t defaultValue = 0);
    uint8_t getUChar(const char *key, uint8_t defaultValue = 0);
    int16_t getShort(const char *key, int16_t defaultValue = 0);
    uint16_t getUShort(const char *key, uint16_t defaultValue = 0);
    int32_t getInt(const char *key, int32_t defaultValue = 0);
    uint32_t getUInt(const char *key, uint32_t defaultValue = 0);
    int32_t getLong(const char *key, int32_t defaultValue = 0);
    uint32_t getULong(const char *key, uint32_t defaultValue = 0);
    int64_t getLong64(const char *key, int64_t defaultValue = 0);
    uint64_t getULong64(const char *key, uint64_t defaultValue = 0);

    size_t getBytesLength(const char *key);
    size_t getBytes(const char *key, void *buf, size_t maxLen);
    size_t freeEntries();
};

#endif
