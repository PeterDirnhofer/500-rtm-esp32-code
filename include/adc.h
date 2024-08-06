#ifndef ADC
#define ADC

#include <stdint.h>


uint16_t readAdc();
esp_err_t i2cAdcInit();

#endif