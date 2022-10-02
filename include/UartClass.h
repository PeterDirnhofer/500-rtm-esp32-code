// Header DECLARATION
// https://marketplace.visualstudio.com/items?itemName=FleeXo.cpp-class-creator
#ifndef UARTCLASS_H
#define UARTCLASS_H

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <string>

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
  void getParameters();

private:
  static void uartRcvLoop(void *unused);
  
  TaskHandle_t task_handle;
  std::string parameters[10];
  int workingMode;


  
};

#endif