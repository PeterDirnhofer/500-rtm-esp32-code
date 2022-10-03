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

class UsbPcInterface
{
public:
  UsbPcInterface();  // der Default-Konstruktor
  ~UsbPcInterface(); // Destructor

  void start();
  static int send(const char *fmt, ...);
  int getPcCommadToSetWorkingMode();
  static bool usbAvailable;
  int getworkingMode();
  std::vector<std::string> getParameters();

private:
  static std::string mUsbReceiveString;
  static void mUartRcvLoop(void *unused);

  TaskHandle_t mTaskHandle;
  bool mStarted;
  std::vector<std::string> mParametersVector;

  int mWorkingMode;
};

#endif