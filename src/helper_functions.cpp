#include <esp_log.h>
#include <cmath>
#include <string>
#include <queue>
#include <sstream>
#include <iomanip>
#include <cstdint>

#include "helper_functions.h"
#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "project_timer.h"

//=============================================================================
// Constants and Static Variables
//=============================================================================
static int integralError = 0;
static const int32_t MAX_PROPORTIONAL = DAC_VALUE_MAX / 10;
static const int32_t MAX_INTEGRAL = DAC_VALUE_MAX / 10;
static const char *TAG = "helper_functions.cpp";

//=============================================================================
// Error Handling Functions
//=============================================================================

/**
 * @brief Set up error message loop and display error
 * @param errormessage Error message to display
 */
extern "C" void setupError(const char *errormessage)
{
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGE(TAG, "%s", errormessage);
    UsbPcInterface::send(errormessage);
    errorBlink();
}

/**
 * @brief Infinite error blink loop - blinks blue LED
 */
extern "C" void errorBlink()
{
    while (true)
    {
        gpio_set_level(IO_04_DAC_MAX, 1); // Turn on blue LED
        vTaskDelay(pdMS_TO_TICKS(100));   // Delay 100 ms
        gpio_set_level(IO_04_DAC_MAX, 0); // Turn off blue LED
        vTaskDelay(pdMS_TO_TICKS(100));   // Delay 100 ms
    }
}

//=============================================================================
// LED Status Functions
//=============================================================================

/**
 * @brief Control LED status based on ADC value relative to target
 * @param adcValue Current ADC reading
 * @param targetAdc Target ADC value
 * @param toleranceAdc Tolerance range for target
 * @param dac Current DAC value (unused but kept for interface compatibility)
 */
void ledStatusAdc(int16_t adcValue, uint16_t targetAdc, uint16_t toleranceAdc, uint16_t dac)
{
    static std::string lastLimit = "INIT";
    static const char *TAG = "ledStatus";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Within tolerance - Yellow LED
    if (abs(targetAdc - adcValue) <= toleranceAdc)
    {
        if (lastLimit != "LIMIT")
        {
            gpio_set_level(IO_25_RED, 0);    // Red LED off
            gpio_set_level(IO_27_YELLOW, 1); // Yellow LED on
            gpio_set_level(IO_02_GREEN, 0);  // Green LED off
            lastLimit = "LIMIT";
        }
    }
    // Above target - Red LED
    else if (adcValue > targetAdc)
    {
        if (lastLimit != "HI")
        {
            gpio_set_level(IO_25_RED, 1);    // Red LED on
            gpio_set_level(IO_27_YELLOW, 0); // Yellow LED off
            gpio_set_level(IO_02_GREEN, 0);  // Green LED off
            lastLimit = "HI";
        }
    }
    // Below target - Green LED
    else
    {
        if (lastLimit != "LO")
        {
            gpio_set_level(IO_25_RED, 0);    // Red LED off
            gpio_set_level(IO_27_YELLOW, 0); // Yellow LED off
            gpio_set_level(IO_02_GREEN, 1);  // Green LED on
            lastLimit = "LO";
        }
    }
}

//=============================================================================
// ADC/Current Calculation Functions
//=============================================================================

/**
 * @brief Calculate ADC value from target current in nanoamperes
 * @param targetNa Target current in nA
 * @return Calculated ADC value (16-bit)
 */
uint16_t calculateAdcFromnA(double targetNa)
{
    static const char *TAG = "calculateAdcFromnA";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Calculate ADC value based on target current and voltage divider
    double adcValueD = (targetNa / ADC_VOLTAGE_DIVIDER) * (ADC_VALUE_MAX / ADC_VOLTAGE_MAX);
    uint16_t adcValue = static_cast<uint16_t>(std::round(adcValueD));

    // ESP_LOGI(TAG, "Target nA: %f, Calculated ADC Value: %d", targetNa, adcValue);
    return adcValue;
}

/**
 * @brief Calculate tunnel current from ADC value
 * @param adcValue ADC reading value
 * @return Current in nanoamperes
 */
double calculateTunnelNa(int16_t adcValue)
{
    // Calculate current based on ADC value and voltage divider
    double currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
    currentTunnelnA = currentTunnelnA * ADC_VOLTAGE_DIVIDER; // Apply voltage divider
    return currentTunnelnA;
}

//=============================================================================
// PID Control Functions
//=============================================================================

/**
 * @brief Compute DAC value using Proportional-Integral control
 *
 * Algorithm:
 * - error = targetAdc - adcValue
 * - proportional = kP * error
 * - integralError += error
 * - integral = kI * integralError
 * - dacValue = currentZDac - proportional - integral
 *
 * @param adcValue Current ADC reading
 * @param targetAdc Target ADC value
 * @return Computed DAC value (16-bit)
 */
uint16_t computePiDac(int16_t adcValue, int16_t targetAdc)
{
    static const char *TAG = "computePiDac";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // TODO: Handle simulation with wired DAC_Z to ADC inverted

    // Calculate the error
    int error = targetAdc - adcValue;
    error *= INVERT_MODE;

    integralError += error;

    // Proportional term with clamping
    int32_t proportional = kP * error;
    if (proportional < -MAX_PROPORTIONAL)
    {
        proportional = -MAX_PROPORTIONAL;
    }
    if (proportional > MAX_PROPORTIONAL)
    {
        proportional = MAX_PROPORTIONAL;
    }

    // Integral term with clamping
    int32_t integral = kI * integralError;
    if (integral < -MAX_INTEGRAL)
    {
        integral = -MAX_INTEGRAL;
    }
    if (integral > MAX_INTEGRAL)
    {
        integral = MAX_INTEGRAL;
    }

    // Compute the DAC value
    int32_t dacValue = currentZDac - proportional - integral;

    // Log the calculation details
    ESP_LOGD(TAG, "ADC Value: %d, Target ADC: %d, Proportional: %" PRId32 ", Integral: %" PRId32 ", DAC Value: %" PRId32,
             adcValue, targetAdc, proportional, integral, dacValue);

    // Clamp the DAC value to valid range
    if (dacValue < 0)
    {
        dacValue = 0;
    }
    if (dacValue > DAC_VALUE_MAX)
    {
        dacValue = DAC_VALUE_MAX;
    }

    return static_cast<uint16_t>(dacValue);
}
