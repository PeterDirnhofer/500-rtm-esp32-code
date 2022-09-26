// Header DECLARATION
// https://marketplace.visualstudio.com/items?itemName=FleeXo.cpp-class-creator
#ifndef UARTCLASS_H
#define UARTCLASS_H

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class UartClass
{
public:
  UartClass();  // der Default-Konstruktor
  ~UartClass(); // Destructor

  void start();

private: // privat
  void uartInit();
  static void uartRcvLoop(void *unused);
  TaskHandle_t task_handle;


};

#endif