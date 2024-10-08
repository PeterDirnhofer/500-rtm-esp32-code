#include "ParameterSetter.h"
#include <esp_log.h>
#include "UsbPcInterface.h"
#include <esp_err.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "globalVariables.h"

using namespace std;

ParameterSetting::ParameterSetting()
{
    this->begin();
}

ParameterSetting::~ParameterSetting()
{
}
static const char *TAG = "ParameterSetting";
const char *keys[] = {"kI", "kP", "kD", "destinatioNa", "remainingNa", "startX", "startY", "direction", "maxX", "maxY", "multiplicator"};
// typical

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
        UsbPcInterface::send("ESP_ERR_INVALID_ARG (not 0...9.)\n%s\n", s);
        return ESP_ERR_INVALID_ARG;
    }

    *value = stof(s);

    // UsbPcInterface::send("*value: %f\n", *value);
    return ESP_OK;
}

esp_err_t ParameterSetting::putParameterToFlash(string key, string value)
{
    float vFloat = 0;
    convertStoFloat(value.c_str(), &vFloat);
    float resultF = 0;

    // UsbPcInterface::send("%s %f\n", key.c_str(), vFloat);
    // parameterSetter.putFloat("P1",testFloat);
    resultF = putFloat(key.c_str(), vFloat);
    if (resultF != sizeof(float))
    {
        UsbPcInterface::send("ESP_ERR_NVS_INVALID_LENGTH Error putFloat to nvs %s %f\n", key.c_str(), vFloat);
        return (ESP_ERR_NVS_INVALID_LENGTH);
    }

    return ESP_OK;
}

esp_err_t ParameterSetting::putParametersToFlash(vector<string> params)
{

    // Clear flash
    this->clear();

    // ESP_LOGI(TAG, "params.size %d\n", (int)params.size());
    if ((int)params.size() != 11)
    {
        ESP_LOGE(TAG, "setparameter needs 9+1 values. Actual %d\n", (int)params.size());
        UsbPcInterface::send("ESP_ERR_INVALID_ARG\n");
        return ESP_ERR_INVALID_ARG;
    }
    // Check, if all parameters are Numbers
    float f = 0;
    for (size_t i = 1; i < 11; i++)
    {

        if (convertStoFloat(params[i].c_str(), &f) != ESP_OK)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }

    for (int i = 1; i < 11; i++)
    {
        // UsbPcInterface::send("putParameter(%s,%s)\n", keys[i], params[i].c_str());
        this->putParameterToFlash(keys[i - 1], params[i].c_str());
    }

    return ESP_OK;
}

esp_err_t ParameterSetting::putDefaultParametersToFlash()
{

    // UsbPcInterface::send("START putDefaaultParametesrToFlash\n");
    vector<string> params;
    params.push_back("PARAMETER");
    params.push_back("1000"); // kI
    params.push_back("3");    // kP
    params.push_back("500");  // kD
    params.push_back("10.0"); // targetTunnelCurrentnA
    params.push_back("0.01"); // toleranceTunnelCurrentnA
    params.push_back("0");    // startX
    params.push_back("0");    // startY
    params.push_back("0");    // direction
    params.push_back("199");  // maxX
    params.push_back("199");  // maxY
    params.push_back("100");  // multiplicator

    esp_err_t err;
    err = this->putParametersToFlash(params);

    return err;
}

/**
 * @brief Check if valid parameters in flash. If not set default parameters
 *
 * @param key
 * @param minimum
 * @param maximum
 * @param value
 * @return esp_err_t
 */
bool ParameterSetting::parameterIsValid(string key, float minimum, float maximum)
{
    float resF = 0;

    if (!isKey(key.c_str()))
    {
        UsbPcInterface::send("key: %s not existing.\n", key.c_str());
        return false;
    }

    resF = getFloat(key.c_str(), __FLT_MAX__);

    if ((resF < minimum) or (resF > maximum))
    {
        UsbPcInterface::send("key: %s = %f out off limit %f .. %f\n", key.c_str(), resF, minimum, maximum);
        return false;
    }
    UsbPcInterface::send("key: %s= %f OK\n", key.c_str(), resF);

    return true;
}

esp_err_t ParameterSetting::parametersAreValid()
{
    for (int i = 0; i < 10; i++)
    {

        if (!isKey(keys[i]))
            return ESP_ERR_NVS_NOT_FOUND;
    }

    return ESP_OK;
}

esp_err_t ParameterSetting::getParametersFromFlash(bool display)
{
    kI = (double)getFloat(keys[0], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("kI,%f\n", kI);

    kP = (double)getFloat(keys[1], __FLT_MAX__);
    if (display)
        UsbPcInterface::sendParameter("kP,%f\n", kP);

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
