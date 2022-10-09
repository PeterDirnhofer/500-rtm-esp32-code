
#include "ParameterSetter.h"
#include <esp_log.h>
#include "UsbPcInterface.h"
#include <esp_err.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

using namespace std;



ParameterSetting::ParameterSetting()
{
    this->begin();
}

ParameterSetting::~ParameterSetting()
{
}
static const char *TAG = "ParameterSetting";

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

esp_err_t ParameterSetting::putParameter(string key, string value)
{
    float vFloat = 0;
    convertStoFloat(value.c_str(), &vFloat);
    float resultF = 0;
    resultF = this->putFloat(key.c_str(), resultF);
    if (resultF != sizeof(float))
    {
        UsbPcInterface::send("ESP_ERR_NVS_INVALID_LENGTH Error putFloat to nvs %s %f\n", key.c_str(), vFloat);
        return (ESP_ERR_NVS_INVALID_LENGTH);
    }

    // UsbPcInterface::send("putParameter key %s value %f result %f\n",key.c_str(),vFloat,resultF);
    return ESP_OK;
}

esp_err_t ParameterSetting::putParameters(vector<string> params)
{

    esp_err_t err = ESP_OK;
    ESP_LOGI(TAG, "params.size %d\n", (int)params.size());
    if ((int)params.size() != 10)
    {
        ESP_LOGE(TAG, "setparameter needs 9+1 values. Actual %d\n", (int)params.size());
        UsbPcInterface::send("ESP_ERR_INVALID_ARG\n");
        return ESP_ERR_INVALID_ARG;
    }
    // Check, if all parameters are Numbers
    float f = 0;
    for (size_t i = 1; i < 10; i++)
    {
        if (convertStoFloat(params[i].c_str(), &f) != ESP_OK)
        {
            return ESP_ERR_INVALID_ARG;
        }
    }

    this->putParameter("kI", params[1]);
    this->putParameter("kP", params[2]);
    this->putParameter("destinatioNa", params[3]);
    this->putParameter("remainingNa", params[4]);
    this->putParameter("startX", params[5]);
    this->putParameter("startY", params[6]);
    this->putParameter("direction", params[7]);
    this->putParameter("maxX", params[8]);
    this->putParameter("maxY", params[9]);

    return ESP_OK;
}

esp_err_t ParameterSetting::putDefaultParameters()
{

    vector<string> params;
    params.push_back("PARAMETER");
    params.push_back("10");   // kI
    params.push_back("1000"); // kP
    params.push_back("10.0"); // destinationTunnelCurrentnA
    params.push_back("0.01"); // remainingTunnelCurrentDifferencenA
    params.push_back("0");    // startX
    params.push_back("0");    // startY
    params.push_back("0");    // direction
    params.push_back("199");  // maxX
    params.push_back("199");  // maxY

    putParameters(params);

    return ESP_OK;
}

vector<string> ParameterSetting::getParameters()
{
    vector<string> returnVector;
    returnVector.push_back("return1");
    returnVector.push_back("return2");
    return returnVector;
}
