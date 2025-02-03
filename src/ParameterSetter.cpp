#include "ParameterSetter.h"
#include "defaultParameters.h"
#include <esp_log.h>
#include "UsbPcInterface.h"
#include <esp_err.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "globalVariables.h"
#include <array>
#include <sstream>
#include "esp_log.h"
#include "helper_functions.h"

using namespace std;

static const char *TAG = "ParameterSetting";

// Define the keys for the parameters
static const std::array<const char *, size_keys> keys = {
    "kP", "kI", "kD", "targetNa", "toleranceNa", "startX", "startY", "measureMs", "direction", "maxX", "maxY", "multiplicator"
};

ParameterSetting::ParameterSetting()
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    this->begin();

    // Check if parameters in flash are valid
    if (parametersAreValid() != ESP_OK)
    {
        ESP_LOGW(TAG, "Parameters in flash are invalid. Writing default parameters to flash.");
        putDefaultParametersToFlash();
    }
}

ParameterSetting::~ParameterSetting()
{
}

// Convert string to float and check for validity
esp_err_t ParameterSetting::convertStoFloat(const char *s, float *value)
{
    if (s == nullptr || *s == '\0')
    {
        UsbPcInterface::send("ESP_ERR_INVALID_ARG (empty string)\n");
        return ESP_ERR_INVALID_ARG;
    }

    // Check if the string contains only valid characters (digits and decimal point)
    bool valid = std::string(s).find_first_not_of("0123456789.") == std::string::npos;
    if (!valid)
    {
        UsbPcInterface::send("ESP_ERR_INVALID_ARG (not 0...9.)\n%s\n", s);
        return ESP_ERR_INVALID_ARG;
    }

    // Convert string to float
    *value = std::stof(s);
    return ESP_OK;
}

// Write a parameter to flash memory
esp_err_t ParameterSetting::putParameterToFlash(std::string key, std::string value)
{
    ESP_LOGW("putParameterToFlash", "%s: %s\n", key.c_str(), value.c_str());

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

// Process the received parameter string and write parameters to flash
esp_err_t ParameterSetting::putParametersToFlashFromString(std::string receive)
{
    // Check if the command starts with "PARAMETER,"
    if (receive.rfind("PARAMETER,", 0) != 0)
    {
        ESP_LOGE(TAG, "Invalid command format");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "putParametersToFlashFromString: %s", receive.c_str());

    // Remove "PARAMETER," prefix
    std::string pars1 = receive.substr(10);

    // Split the remaining string by commas
    std::vector<std::string> tokens;
    std::stringstream ss(pars1);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        tokens.push_back(token);
    }

    // Log the tokens
    for (size_t i = 0; i < tokens.size(); ++i)
    {
        ESP_LOGI(TAG, "Token %d: %s", i, tokens[i].c_str());
    }

    // Ensure the number of tokens matches the number of keys
    if (tokens.size() != size_keys)
    {
        ESP_LOGE(TAG, "Incorrect number of parameters");
        return ESP_ERR_INVALID_ARG;
    }

    // Check if all parameters are numbers and write them to flash
    float f = 0;
    for (size_t i = 0; i < size_keys; i++)
    {
        if (convertStoFloat(tokens[i].c_str(), &f) != ESP_OK)
        {
            ESP_LOGE(TAG, "Parameter %s is not a number", tokens[i].c_str());
            return ESP_ERR_INVALID_ARG;
        }

        // Write the parameter to flash
        this->putParameterToFlash(keys[i], tokens[i].c_str());
    }

    return ESP_OK;
}

// Write parameters to flash from a vector of strings
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
        this->putParameterToFlash(keys[i], params[i + 1].c_str());
    }

    return ESP_OK;
}

// Write default parameters to flash
esp_err_t ParameterSetting::putDefaultParametersToFlash()
{
    std::vector<std::string> params = defaultParameters;

    esp_err_t result = this->putParametersToFlash(params); // Capture result of the function call
    if (result != ESP_OK)
    {                                                                                            // Check if the result is not OK
        ESP_LOGE(TAG, "Failed to put default parameters to flash: %s", esp_err_to_name(result)); // Log error with message
    }

    return result; // Return the result
}

// Validate a parameter
bool ParameterSetting::parameterIsValid(std::string key, float minimum, float maximum)
{
    // Implementation of parameter validation
    return true;
}

// Check if parameters in flash are valid
esp_err_t ParameterSetting::parametersAreValid()
{
    for (int i = 0; i < size_keys; i++)
    {
        if (!isKey(keys[i]))
        {
            ESP_LOGE(TAG, "is no valid key: %s", keys[i]); // Log the current key
            return ESP_ERR_NVS_NOT_FOUND;
        }
    }
    return ESP_OK;
}

// Get parameters from flash
esp_err_t ParameterSetting::getParametersFromFlash()
{
    kP = (double)getFloat(keys[0], __FLT_MAX__);
    kI = (double)getFloat(keys[1], __FLT_MAX__);
    kD = (double)getFloat(keys[2], __FLT_MAX__);
    targetTunnelnA = (double)getFloat(keys[3], __FLT_MAX__);
    targetTunnelAdc = calculateAdcFromnA(targetTunnelnA);
    toleranceTunnelnA = (double)getFloat(keys[4], __FLT_MAX__);
    toleranceTunnelAdc = calculateAdcFromnA(toleranceTunnelnA);
    startX = (uint16_t)getFloat(keys[5], __FLT_MAX__);
    rtmGrid.setStartX(startX);
    startY = (uint16_t)getFloat(keys[6], __FLT_MAX__);
    rtmGrid.setStartY(startY);
    measureMs = (uint16_t)getFloat(keys[7], __FLT_MAX__);
    direction = (bool)getFloat(keys[8], __FLT_MAX__);
    uint16_t mMaxX = (uint16_t)getFloat(keys[9], __FLT_MAX__);
    rtmGrid.setMaxX(mMaxX);
    nvs_maxX = mMaxX;
    uint16_t mMaxY = (uint16_t)getFloat(keys[10], __FLT_MAX__);
    nvs_maxY = mMaxY;
    rtmGrid.setMaxY(mMaxY);
    uint16_t mMultiplicator = (uint16_t)getFloat(keys[11], __FLT_MAX__);
    rtmGrid.setMultiplicatorGridAdc(mMultiplicator);

    // Create a string to store all parameters
    std::string parameters;
    parameters += std::string(keys[0]) + "," + std::to_string(kP) + "\n";
    parameters += std::string(keys[1]) + "," + std::to_string(kI) + "\n";
    parameters += std::string(keys[2]) + "," + std::to_string(kD) + "\n";
    parameters += std::string(keys[3]) + "," + std::to_string(targetTunnelnA) + "\n";
    parameters += std::string(keys[4]) + "," + std::to_string(toleranceTunnelnA) + "\n";
    parameters += std::string(keys[5]) + "," + std::to_string(startX) + "\n";
    parameters += std::string(keys[6]) + "," + std::to_string(startY) + "\n";
    parameters += std::string(keys[7]) + "," + std::to_string(measureMs) + "\n";
    parameters += std::string(keys[8]) + "," + std::to_string(direction) + "\n";
    parameters += std::string(keys[9]) + "," + std::to_string(mMaxX) + "\n";
    parameters += std::string(keys[10]) + "," + std::to_string(mMaxY) + "\n";
    parameters += std::string(keys[11]) + "," + std::to_string(mMultiplicator) + "\n";

    this->storedParameters = parameters;

    return ESP_OK;
}

// Get parameters as a string
string ParameterSetting::getParameters()
{
    return this->storedParameters;
}