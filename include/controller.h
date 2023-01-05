// controller.h
#ifndef CONTROLLER
#define CONTROLLER
#include <stdint.h>
#include "esp_log.h"
#include <stdarg.h>
#include "driver/timer.h"

#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "adc.h"
#include "spi.h"
#include "dataStoring.h"
#include "timer.h"

extern "C" void measurementStart();
extern "C" void measurementLoop(void* unused);
extern "C" uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);
extern "C" int sendPaketWithData(bool terminate = false);
extern "C" void displayTunnelCurrentLoop(UsbPcInterface);

#endif