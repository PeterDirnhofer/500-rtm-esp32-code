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
extern "C" void testio(gpio_num_t io, int cycles);
extern "C" void adjustStart();
extern "C" void adjustLoop(void* unused);

extern "C" uint16_t m_saturate16bit(uint32_t input, uint16_t min, uint16_t max);
extern "C" int sendPaketWithData(bool terminate = false);


#endif