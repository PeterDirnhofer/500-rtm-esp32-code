
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
    esp_err_t err=ESP_OK;
    ESP_LOGI(TAG, "params.size %d\n", (int)params.size());
    if ((int)params.size() != 10)
    {
        ESP_LOGE(TAG, "setparameter needs 9+1 values. Actual %d\n", (int)params.size());

        return ESP_ERR_INVALID_ARG;
    }
    float f=0;
    
    ESP_ERROR_CHECK(f= stof(params[1]));
       // dynamic_cast
    return ESP_OK;
}

vector<string> ParameterSetting::getParameters()
{
    vector<string> returnVector;
    returnVector.push_back("return1");
    returnVector.push_back("return2");
    return returnVector;
}
