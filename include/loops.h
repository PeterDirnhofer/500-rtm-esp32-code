#ifndef LOOPS_H
#define LOOPS_H

#include "UsbPcInterface.h"
#include "adc_i2c.h"
#include "dac_spi.h"
#include "dataStoring.h"
#include "esp_log.h"
#include "globalVariables.h"
#include <stdarg.h>
#include <stdint.h>


extern "C" void dataTransmissionLoop(void *params);
extern "C" void adjustLoop(void *unused);
extern "C" void sinusLoop(void *unused);
extern "C" void measureLoop(void *unused);
extern "C" void tunnelLoop(void *params);
extern "C" void setPrefix(const char *prefix);
extern "C" void queueToPcClear();
extern "C" void resetDac();
#endif // LOOPS_H
