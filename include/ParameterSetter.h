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

  

  esp_err_t putParameters(vector<string> params);
  vector<string> getParameters();

  esp_err_t putDefaultParameters();

private:
  esp_err_t convertStoFloat(string s, float *value);
  esp_err_t putParameter(string key, string value);
  esp_err_t getParameter(string key, float *value);


};

#endif