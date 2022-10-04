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


class ParameterSetting:public NvsStorageClass
{
protected:
  

public:
  ParameterSetting();  // der Default-Konstruktor
  ~ParameterSetting(); // Destructor

  


private:

};

#endif