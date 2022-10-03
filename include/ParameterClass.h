#ifndef PARAMETER
#define PARAMETER

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <string>
#include <vector>
#include "NvsStorageClass.h"
#include <esp_log.h>


class ParameterClass
{
protected:
  NvsStorageClass _nvs;

public:
  ParameterClass();  // der Default-Konstruktor
  ~ParameterClass(); // Destructor

  esp_err_t setParameters(std::vector<std::string> params);
  std::vector<std::string> getParameters();



private:

  void setKI();
};

#endif
