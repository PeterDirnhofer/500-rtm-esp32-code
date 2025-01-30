// Header DECLARATION
// https://marketplace.visualstudio.com/items?itemName=FleeXo.cpp-class-creator
#ifndef USBPCINTERFACE_H
#define USBPCINTERFACE_H

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


// static const int RX_BUF_SIZE = 200;
using namespace std;
/**
 * @brief Communication with PC via USB
 *
 */
class UsbPcInterface
{
public:
  UsbPcInterface();  // default-construktor
  ~UsbPcInterface(); // destructor

  void start();
  static int send(const char *fmt, ...);
  static int sendParameter(const char *fmt, ...);
  static void printErrorMessageAndRestart(string error_string);
  static void printMessageAndRestart(string msg);
  static esp_err_t sendData();
  static const int RX_BUF_SIZE = 200;
  inline static bool adjustIsActive = false;
  inline static string mUsbReceiveString = "";
  inline static bool mUsbAvailable = false;
  static esp_err_t mUpdateTip(string);

private:
  static void mUartRcvLoop(void *unused);
  TaskHandle_t mTaskHandle;
  bool mStarted;
  vector<string> mParametersVector;
  int numberOfValues = 1;
  inline static int m_workingmode = MODE_IDLE;

  static void dispatcherTask(void *unused);
};

#endif // USBPCINTERFACE_H
