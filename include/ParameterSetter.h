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
  esp_err_t putDefaultParameters();
  esp_err_t putParameter(string key, string value);


  vector<string> getParameters();

private:
esp_err_t convertStoFloat(string s, float * value);
};

#endif