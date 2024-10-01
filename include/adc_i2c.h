#ifndef ADC
#define ADC

uint16_t readAdc();
uint16_t readAdc(bool return_unsigned);
esp_err_t i2cAdcInit();

#endif