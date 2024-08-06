#include <stdint.h>

#include "globalVariables.h"
#include "freertos/FreeRTOS.h"

uint16_t readAdc()
{

    uint8_t dataH=0, dataL = 0;

    uint16_t temp = dataH << 8 | dataL;

    return temp;
}

esp_err_t i2cAdcInit()
{

    // INIT ADC
    //  Config ADS 1115 Single
    //  Bits 15 14 13 12  11 10 09 08
    //        0  1  1  1   0  1  0  0
    //       15 Single Conversion
    //           1  1  1  Measure Comparator A3 against GND
    //                     0  1  0  gain +- 2.048 V
    //                              0 Continuous mode, no reset after conversion

    // Bits 07 06 05 04  03 02 01 00
    //       1  1  1  0   0  0  1  1
    //       1  1  1  Datarate 860 samples per second  (ACHTUNG passt das in ms takt?)
    //                0 Comparator traditional (unwichtig, nur für Threshold)
    //                    0 Alert/Rdy Active low
    //                        0  No latch comparator
    //                           1  1  Disable coparator
    esp_err_t err;
    err = ESP_OK;

    return err;
}