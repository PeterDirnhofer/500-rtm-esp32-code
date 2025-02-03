#ifndef PARAMETERSETTER_H
#define PARAMETERSETTER_H

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <string>
#include <vector>
#include <utility> // For std::pair
#include "NvsStorageClass.h"
#include <esp_log.h>
#include "esp_err.h"
using namespace std;

static const int size_keys = 12;

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
  esp_err_t getParametersFromFlash();
  esp_err_t putParametersToFlashFromString(string receive);

  esp_err_t putDefaultParametersToFlash();
  esp_err_t parametersAreValid();
  std::string getParameters();

private:
  bool parameterIsValid(string key, float minimum, float maximum);
  esp_err_t convertStoFloat(const char *s, float *value);
  esp_err_t putParameterToFlash(string key, string value);

  std::string storedParameters;
};

#endif // PARAMETERSETTER_H