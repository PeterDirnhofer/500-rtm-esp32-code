#ifndef PARAMETER_SETTING
#define PARAMETER_SETTING

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <string>
#include <vector>
#include "NvsStorageClass.h"
#include <esp_log.h>
#include "esp_err.h"
using namespace std;

/**
 * @brief Put and get parameters from/to non volatile memory
 *
 */
class ParameterSetting : public NvsStorageClass
{
protected:
public:
  ParameterSetting();  // der Default-Konstruktor
  ~ParameterSetting(); // Destructor

  

  esp_err_t putParametersToFlash(vector<string> params);
  esp_err_t getParametersFromFlash(bool display=false);

  esp_err_t putDefaultParametersToFlash();

  bool parameterIsValid(string key, float minimum, float maximum);
  esp_err_t parametersAreValid();


private:
  esp_err_t convertStoFloat(string s, float *value);
  esp_err_t putParameterToFlash(string key, string value);

  //esp_err_t getParameterToFlash(string key, float *value);
  
};

#endif