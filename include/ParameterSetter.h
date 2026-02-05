#ifndef PARAMETERSETTER_H
#define PARAMETERSETTER_H

#pragma once

#include "nvs_helpers.h"
#include <cstring>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include <utility> // For std::pair
#include <vector>


static const int size_keys = 12;

class ParameterSetting {
public:
  ParameterSetting();
  ~ParameterSetting();
  esp_err_t putParametersToFlash(std::vector<std::string> params);
  esp_err_t getParametersFromFlash();
  esp_err_t putParametersToFlashFromString(std::string receive);

  esp_err_t putDefaultParametersToFlash();
  esp_err_t parametersAreValid();
  std::string getParameters();

private:
  bool parameterIsValid(std::string key, float minimum, float maximum);
  esp_err_t convertStoFloat(const char *s, float *value);
  esp_err_t putParameterToFlash(std::string key, std::string value);

  std::string storedParameters;
};

#endif // PARAMETERSETTER_H