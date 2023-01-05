// Header DECLARATION
// https://marketplace.visualstudio.com/items?itemName=FleeXo.cpp-class-creator
#ifndef UARTCLASS_H
#define UARTCLASS_H
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>
#include <vector>
#include <memory>
#include <esp_log.h>
#include "esp_system.h"
#include "driver/uart.h" 
#include "driver/gpio.h"
#include "globalVariables.h"


static const int RX_BUF_SIZE = 200;
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
  static int sendParameter(const char *fmt, ...);
  static void printErrorMessageAndRestart(string error_string);
  static void printMessageAndRestart(string msg);
  static esp_err_t sendData();
  esp_err_t getCommandsFromPC();
  int getWorkingMode();
  vector<string> getParametersFromPc();

private:
  static void mUartRcvLoop(void *unused);
  esp_err_t mUpdateTip();

  inline static string mUsbReceiveString = "";
  inline static bool mUsbAvailable = false;
  TaskHandle_t mTaskHandle;
  bool mStarted;
  vector<string> mParametersVector;
  int mWorkingMode;
  int numberOfValues = 1;
  
};

#endif
