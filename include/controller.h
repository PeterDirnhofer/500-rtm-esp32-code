// controller.h
#ifndef CONTROLLER
#define CONTROLLER

#include <stdint.h>
#include "esp_log.h"
#include <stdarg.h>
#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "adc_i2c.h"
#include "dac_spi.h"
#include "dataStoring.h"

// Function prototypes

extern "C" esp_err_t initAdcDac();

extern "C" void commandDispatcherTask(void *unused);

extern "C" void dispatcherTaskStart();

extern "C" void measureStart();

extern "C" void measureLoop(void *unused);

extern "C" void adjustStart();

extern "C" void adjustLoop(void *unused);

double calculateTunnelNa(int16_t adcValue);

// Declare the queue handle
extern QueueHandle_t queueFromPc;

#endif // CONTROLLER
