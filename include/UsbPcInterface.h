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

using namespace std;
/**
 * @brief Communication with PC via USB
 * 
 */
class UsbPcInterface
{
public:
  UsbPcInterface();  // der Default-Konstruktor
  ~UsbPcInterface(); // Destructor

  void start();
  static int send(const char *fmt, ...);
  int getPcCommadToSetWorkingMode();
  inline static bool usbAvailable=false;
  int getWorkingMode();
  vector<string> getParameters();

private:
  inline static string mUsbReceiveString="";
  static void mUartRcvLoop(void *unused);

  TaskHandle_t mTaskHandle;
  bool mStarted;
  vector<string> mParametersVector;

  int mWorkingMode;
};

#endif