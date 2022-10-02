#ifndef PARAMETER
#define PARAMETER

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <string>
#include "NvsStorageClass.h"

class ParameterClass
{
protected:
  NvsStorageClass _nvs;

public:
  ParameterClass();  // der Default-Konstruktor
  ~ParameterClass(); // Destructor

private:
};

#endif
