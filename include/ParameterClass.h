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

  esp_err_t setParameter(std::vector<std::string> params);
  std::vector<std::string> getParameter();



private:

  void setKI();
};

#endif
