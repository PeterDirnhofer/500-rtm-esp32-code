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
extern "C" esp_err_t initHardware();

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
 * @brief Starts the tunnel finding task loop and initializes the timer for the tunnel finding mode.
 */
extern "C" void findTunnelStart();

/**
 * @brief Task loop for finding the tunneling current using PID control.
 *
 * This function controls the Z-height to maintain the desired tunneling current
 * by adjusting the Z-position via PID control. The function runs until the
 * current is within the specified tolerance or a maximum iteration count is reached.
 *
 * @param unused FreeRTOS task parameter, not used in this implementation.
 */
extern "C" void findTunnelLoop(void *unused);

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

/**
 * @brief Saturates the input value within the given 16-bit boundaries.
 *
 * @param input The value to be saturated.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @return uint16_t The saturated value.
 */
extern "C" uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);

/**
 * @brief Sends the measurement data stored in the queue to the PC.
 *        Pauses the measurement if the queue overflows.
 *
 * @param terminate Whether to terminate after sending the data.
 * @return int 0 on success, error code otherwise.
 */
extern "C" int sendDataPaket(bool terminate = false);

/**
 * @brief Sends the tunnel data stored in the queue to the PC.
 *
 * @param terminate Whether to terminate after sending the data.
 * @return int 0 on success, error code otherwise.
 */
extern "C" int sendTunnelPaket();

/**
 * @brief Constrains the value within a specified range.
 *
 * @param value The value to be constrained.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @return double The constrained value.
 */
extern "C" double constrain(double value, double min, double max);

int16_t adcValueDebounced(int16_t adcValue);
double clamp(double value, double minValue, double maxValue);
uint16_t computePI(double currentNa, double targetNa);

#endif // CONTROLLER
