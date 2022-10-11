
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
const char *keys[] = {"kI", "kP", "destinatioNa", "remainingNa", "startX", "startY", "direction", "maxX", "maxY"};
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

    UsbPcInterface::send("%s %f\n", key.c_str(), vFloat);
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

    esp_err_t err = ESP_OK;
    // ESP_LOGI(TAG, "params.size %d\n", (int)params.size());
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

    for (int i = 1; i < 10; i++)
    {
        //UsbPcInterface::send("putParameter(%s,%s)\n", keys[i], params[i].c_str());
        this->putParameterToFlash(keys[i - 1], params[i].c_str());
    }

    return ESP_OK;
}

esp_err_t ParameterSetting::putDefaultParametersToFlash()
{

    //UsbPcInterface::send("START putDefaaultParametesrToFlash\n");
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

    this->putParametersToFlash(params);

    return ESP_OK;
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

    resF = getFloat(key.c_str(), NULL);
    
    
    if((resF < minimum) or (resF>maximum)){
        UsbPcInterface::send("key: %s = %f out off limit %f .. %f\n",key.c_str(),resF,minimum,maximum);
        return false;
    }
    UsbPcInterface::send("key: %s= %f OK\n", key.c_str(), resF);
    

    return true;
}

esp_err_t ParameterSetting::parametersAreValid(){

    return ESP_OK;
}

vector<string> ParameterSetting::getParametersFromFlash()
{
    vector<string> returnVector;

    if(!parameterIsValid("kI",0,0)){
        
    }


    float resultF = 0;

    resultF = this->getFloat("kP", -999.999);

    UsbPcInterface::send("kP resultF= %f \n", resultF);

    string help = to_string(resultF);
    returnVector.push_back(help.c_str());
    returnVector.push_back("return2");
    return returnVector;
}
