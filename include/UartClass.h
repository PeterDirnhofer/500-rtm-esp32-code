// Header DECLARATION
// https://marketplace.visualstudio.com/items?itemName=FleeXo.cpp-class-creator
#ifndef UARTCLASS_H
#define UARTCLASS_H

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <string>
#include <vector>

class UartClass
{
public:
  UartClass();  // der Default-Konstruktor
  ~UartClass(); // Destructor

  void start();
  static int send(const char *fmt, ...);
  int getPcCommad();
  

  static bool usbAvailable;
  static std::string usbReceive;
  int getworkingMode();
  std::vector<std::string> getParameters();

private:
  static void uartRcvLoop(void *unused);
  
  TaskHandle_t task_handle;
  bool _started;
  std::vector<std::string> parametersVector;

  int workingMode;


  
};

#endif