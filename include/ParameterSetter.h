#ifndef PARAMETERSETTER_H
#define PARAMETERSETTER_H

#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>
#include <string>
#include <vector>
#include <utility> // For std::pair
#include "NvsStorageClass.h"
#include <esp_log.h>
#include <esp_err.h>

static const int size_keys = 12;

class ParameterSetting : public NvsStorageClass
{
protected:
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