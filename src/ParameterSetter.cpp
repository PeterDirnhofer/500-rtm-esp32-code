#include "ParameterSetter.h"
#include <esp_log.h>
#include "UsbPcInterface.h"
#include <esp_err.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "globalVariables.h"
#include <array>
#include "esp_log.h"

using namespace std;

static const char *TAG = "ParameterSetting";

static const int size_keys = 11;
static const std::array<const char *, size_keys> keys = {
    "kP", "kI", "kD", "targetNa", "toleranceNa", "startX",
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
    ESP_LOGW("putParameterToFlash", "%s:  %s  \n", key.c_str(), value.c_str());

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

    if (params.size() != size_keys + 1) // params[0]  = "PARAMETER"
    {
        ESP_LOGE("putParametersToFlash", "setparameter needs 11+1 values. Actual %d\n", (int)params.size());
        UsbPcInterface::send("ESP_ERR_INVALID_ARG\n");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGW("putParametersToFlash", "Start Put parameters to flash with size  %d ... \n", (int)params.size());

    // Check if all parameters are numbers
    float f = 0;
    for (size_t i = 1; i <= size_keys; i++)
    {

        if (convertStoFloat(params[i].c_str(), &f) != ESP_OK)
        {
            ESP_LOGE("putParametersToFlash", "Parameter %s is no number", params[i].c_str());
            return ESP_ERR_INVALID_ARG;
        }
    }

    for (int i = 0; i < size_keys; i++)
    {
        // i+1.
        this->putParameterToFlash(keys[i], params[i + 1].c_str());
    }

    return ESP_OK;
}
// List of default Paramater
esp_err_t ParameterSetting::putDefaultParametersToFlash()
{
    vector<string> params = {
        "PARAMETER",
        "300.0", // kP
        "5.0",   // kI
        "0.0",   // kD
        "10",   // targetTunnelCurrentnA
        "1.2",  // toleranceTunnelCurrentnA
        "0",     // startX
        "0",     // startY
        "0",     // direction
        "199",   // maxX
        "199",   // maxY
        "100"    // multiplicator
    };

    esp_err_t result = this->putParametersToFlash(params); // Capture result of the function call
    if (result != ESP_OK)
    {                                                                                            // Check if the result is not OK
        ESP_LOGE(TAG, "Failed to put default parameters to flash: %s", esp_err_to_name(result)); // Log error with message
    }
    else
    {
        ESP_LOGI(TAG, "Default parameter set to default");
    }

    return result; // Return the result
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
    for (int i = 0; i < size_keys; i++)
    {
        ESP_LOGI(TAG, "Checking key: %s", keys[i]); // Log each key before validation
        if (!isKey(keys[i]))
        {
            ESP_LOGE(TAG, "is no valid key: %s", keys[i]); // Log the current key
            return ESP_ERR_NVS_NOT_FOUND;
        }
    }
    ESP_LOGI(TAG, "ALL KEYS OK"); // Log the current key
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

    targetTunnelnA = (double)getFloat(keys[3], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("targetTunnelCurrentnA,%f\n", targetTunnelnA);

    toleranceTunnelnA = (double)getFloat(keys[4], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("toleranceTunnelCurrentnA,%f\n", toleranceTunnelnA);

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
