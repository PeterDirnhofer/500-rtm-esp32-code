#ifndef ADC
#define ADC

#include <stdint.h>

#include "driver/i2c.h"

uint16_t readAdc();
esp_err_t i2cInit();

#endif