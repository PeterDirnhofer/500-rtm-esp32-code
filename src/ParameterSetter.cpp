
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

    UsbPcInterface::send("*value: %f\n", *value);
    return ESP_OK;
}

esp_err_t ParameterSetting::putParameters(vector<string> params)
{
    // 100 1000 10.0 0.01 0 0 0 199 199``
    /*
    kI = atof(argv[1]);
    kP = atof(argv[2]);
    destinationTunnelCurrentnA = atof(argv[3]);
    remainingTunnelCurrentDifferencenA = atof(argv[4]);
    startX = (uint16_t) atoi(argv[5]);
    startY = (uint16_t) atoi(argv[6]);
    direction = (bool) atoi(argv[7]);
    maxX = (uint16_t) atoi(argv[8]);
    maxY = (uint16_t) atoi(argv[9]);

    */
    esp_err_t err = ESP_OK;
    ESP_LOGI(TAG, "params.size %d\n", (int)params.size());
    if ((int)params.size() != 10)
    {
        ESP_LOGE(TAG, "setparameter needs 9+1 values. Actual %d\n", (int)params.size());
        UsbPcInterface::send("ESP_ERR_INVALID_ARG\n");
        return ESP_ERR_INVALID_ARG;
    }

    float f = 0;

    if (convertStoFloat(params[1].c_str(), &f) != ESP_OK)
    {
        return ESP_ERR_INVALID_ARG;
    }

    UsbPcInterface::send("NACH stof f=%f \n", f);

    return ESP_OK;
}

vector<string> ParameterSetting::getParameters()
{
    vector<string> returnVector;
    returnVector.push_back("return1");
    returnVector.push_back("return2");
    return returnVector;
}
