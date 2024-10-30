#include "ParameterSetter.h"
#include <esp_log.h>
#include "UsbPcInterface.h"
#include <esp_err.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "globalVariables.h"
#include <array>

using namespace std;

static const char *TAG = "ParameterSetting";

static const std::array<const char *, 11> keys = {
    "kP", "kI", "kD", "destinatioNa", "remainingNa", "startX",
    "startY", "direction", "maxX", "maxY", "multiplicator"};

ParameterSetting::ParameterSetting()
{
    this->begin();
}

ParameterSetting::~ParameterSetting()
{
}

esp_err_t ParameterSetting::convertStoFloat(string s, float *value)
{
    if (s.empty())
    {
        UsbPcInterface::send("ESP_ERR_INVALID_ARG (empty string)\n");
        return ESP_ERR_INVALID_ARG;
    }

    bool valid = s.find_first_not_of("0123456789.") == std::string::npos;
    if (!valid)
    {
        UsbPcInterface::send("ESP_ERR_INVALID_ARG (not 0...9.)\n%s\n", s.c_str());
        return ESP_ERR_INVALID_ARG;
    }

    *value = stof(s);
    return ESP_OK;
}

esp_err_t ParameterSetting::putParameterToFlash(string key, string value)
{
    float vFloat = 0;
    esp_err_t err = convertStoFloat(value.c_str(), &vFloat);
    if (err != ESP_OK)
    {
        return err;
    }

    float resultF = putFloat(key.c_str(), vFloat);
    if (resultF != sizeof(float))
    {
        UsbPcInterface::send("ESP_ERR_NVS_INVALID_LENGTH Error putFloat to nvs %s %f\n", key.c_str(), vFloat);
        return ESP_ERR_NVS_INVALID_LENGTH;
    }

    return ESP_OK;
}

esp_err_t ParameterSetting::putParametersToFlash(vector<string> params)
{
    // Clear flash
    this->clear();

    if (params.size() != 12)
    {
        ESP_LOGE(TAG, "setparameter needs 11+1 values. Actual %d\n", (int)params.size());
        UsbPcInterface::send("ESP_ERR_INVALID_ARG\n");
        return ESP_ERR_INVALID_ARG;
    }

    // Check if all parameters are numbers
    float f = 0;
    for (size_t i = 1; i < 11; i++)
    {
        if (convertStoFloat(params[i].c_str(), &f) != ESP_OK)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }

    for (int i = 1; i <= 10; i++)
    {
        this->putParameterToFlash(keys[i - 1], params[i].c_str());
    }

    return ESP_OK;
}

esp_err_t ParameterSetting::putDefaultParametersToFlash()
{
    vector<string> params = {
        "PARAMETER",
        "3.0",  // kP
        "0.3",  // kI
        "0.03", // kD
        "10",   // targetTunnelCurrentnA
        "0.01", // toleranceTunnelCurrentnA
        "0",    // startX
        "0",    // startY
        "0",    // direction
        "199",  // maxX
        "199",  // maxY
        "100"   // multiplicator
    };

    return this->putParametersToFlash(params);
}

bool ParameterSetting::parameterIsValid(string key, float minimum, float maximum)
{
    if (!isKey(key.c_str()))
    {
        UsbPcInterface::send("key: %s not existing.\n", key.c_str());
        return false;
    }

    float resF = getFloat(key.c_str(), __FLT_MAX__);
    if ((resF < minimum) || (resF > maximum))
    {
        UsbPcInterface::send("key: %s = %f out of limit %f .. %f\n", key.c_str(), resF, minimum, maximum);
        return false;
    }

    UsbPcInterface::send("key: %s= %f OK\n", key.c_str(), resF);
    return true;
}

esp_err_t ParameterSetting::parametersAreValid()
{
    for (int i = 0; i < 11; i++)
    {
        if (!isKey(keys[i]))
        {
            return ESP_ERR_NVS_NOT_FOUND;
        }
    }
    return ESP_OK;
}

esp_err_t ParameterSetting::getParametersFromFlash(bool display)
{
    kP = (double)getFloat(keys[0], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("kP,%f\n", kP);

    kI = (double)getFloat(keys[1], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("kI,%f\n", kI);

    kD = (double)getFloat(keys[2], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("kD,%f\n", kD);

    targetTunnelCurrentnA = (double)getFloat(keys[3], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("targetTunnelCurrentnA,%f\n", targetTunnelCurrentnA);

    toleranceTunnelCurrentnA = (double)getFloat(keys[4], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("toleranceTunnelCurrentnA,%f\n", toleranceTunnelCurrentnA);

    startX = (uint16_t)getFloat(keys[5], __FLT_MAX__);
    rtmGrid.setStartX(startX);
    if (display)
        UsbPcInterface::sendParameter("startX,%u\n", startX);

    startY = (uint16_t)getFloat(keys[6], __FLT_MAX__);
    rtmGrid.setStartY(startY);
    if (display)
        UsbPcInterface::sendParameter("startY,%u\n", startY);

    direction = (bool)getFloat(keys[7], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("direction,%d\n", direction);

    uint16_t mMaxX = (uint16_t)getFloat(keys[8], __FLT_MAX__);
    rtmGrid.setMaxX(mMaxX);
    nvs_maxX = mMaxX;
    if (display)
        UsbPcInterface::sendParameter("maxX,%d\n", mMaxX);

    uint16_t mMaxY = (uint16_t)getFloat(keys[9], __FLT_MAX__);
    nvs_maxY = mMaxY;
    rtmGrid.setMaxY(mMaxY);
    if (display)
        UsbPcInterface::sendParameter("maxY,%d\n", mMaxY);

    uint16_t mMultiplicator = (uint16_t)getFloat(keys[10], __FLT_MAX__);
    rtmGrid.setMultiplicatorGridAdc(mMultiplicator);
    if (display)
        UsbPcInterface::sendParameter("MultiplicatorGridAdc,%d\n", mMultiplicator);

    return ESP_OK;
}
