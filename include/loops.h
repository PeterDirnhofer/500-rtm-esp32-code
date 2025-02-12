#ifndef LOOPS_H
#define LOOPS_H

#include <stdint.h>
#include "esp_log.h"
#include <stdarg.h>
#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "adc_i2c.h"
#include "dac_spi.h"
#include "dataStoring.h"

extern "C" void dataTransmissionLoop(void *params);
extern "C" void adjustLoop(void *unused);
extern "C" void sinusLoop(void *unused);
extern "C" void measureLoop(void *unused);
extern "C" void tunnelLoop(void *params);

#endif // LOOPS_H