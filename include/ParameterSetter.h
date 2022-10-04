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

private:
};

#endif