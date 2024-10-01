// controller.h
#ifndef CONTROLLER
#define CONTROLLER
#include <stdint.h>
#include "esp_log.h"
#include <stdarg.h>
// #include "driver/timer.h"

#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "adc_i2c.h"
#include "dac_spi.h"
#include "dataStoring.h"
// #include "timer.h"

extern "C" esp_err_t initHardware();
extern "C" void measurementStart();
extern "C" void measurementLoop(void *unused);

extern "C" void findTunnelStart();
extern "C" void findTunnelLoop(void *unused);

extern "C" void adjustStart();
extern "C" void adjustLoop(void *unused);

extern "C" uint16_t m_saturate16bit(uint32_t input, uint16_t min, uint16_t max);
extern "C" int m_sendDataPaket(bool terminate = false);

#endif