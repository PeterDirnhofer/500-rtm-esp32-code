
#include "ParameterSetter.h"
#include <esp_log.h>

using namespace std;


ParameterSetting::ParameterSetting()
{
    this->begin();
}

ParameterSetting::~ParameterSetting()
{
}

static const char *TAG = "ParameterSetting";

esp_err_t ParameterSetting::setParameters(vector<string> params)
{
    // 100 1000 10.0 0.01 0 0 0 199 199``
    // PARAMETER,100,1000,10.0,0.01,0,0,0,199,199
    ESP_LOGI(TAG, "params.size %d\n", (int)params.size());
    if ((int)params.size() != 10)
    {
        ESP_LOGE(TAG, "setparameter needs 9+1 values. Actual %d\n", (int)params.size());

        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

vector<string> ParameterSetting::getParameters()
{
    vector<string> returnVector;
    returnVector.push_back("return1");
    returnVector.push_back("return2");
    return returnVector;
}
