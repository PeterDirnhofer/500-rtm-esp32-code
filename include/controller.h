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

/**
 * @brief Initializes the hardware components including I2C (ADC) and SPI (DACs).
 *        Restarts the ESP on failure.
 *
 * @return esp_err_t ESP_OK on success, error code otherwise.
 */
extern "C" esp_err_t initAdcDac();

/**
 * @brief Starts the measurement task loop and initializes the timer for the measurement mode.
 */

extern "C" void measureStart();

/**
 * @brief Main loop for performing measurements.
 *
 * This loop measures the tunnel current, adjusts the Z-position, and sends data to the PC.
 * Triggered by the GPTimer tick.
 *
 * @param unused FreeRTOS task parameter, not used in this implementation.
 */
extern "C" void measureLoop(void *unused);





/**
 * @brief Starts the adjustment task loop and initializes the timer for adjust mode.
 */
extern "C" void adjustStart();

/**
 * @brief Main loop for performing adjustments.
 *
 * This loop reads ADC values, converts them to voltages, and sends the current.
 * Triggered by GPTimer tick.
 *
 * @param unused FreeRTOS task parameter, not used in this implementation.
 */
extern "C" void adjustLoop(void *unused);

double calculateTunnelNa(int16_t adcValue);

#endif // CONTROLLER
