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


/**
 * @brief Put and get parameters from/to non volatile memory
 * 
 */
class ParameterSetting:public NvsStorageClass
{
protected:
  

public:
  ParameterSetting();  // der Default-Konstruktor
  ~ParameterSetting(); // Destructor
  

 void adMethod();
  


private:

};

#endif