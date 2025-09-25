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
#include "globalVariables.h"

// Constants
static int integralError = 0;
static const int32_t MAX_PROPORTIONAL = DAC_VALUE_MAX / 10;
static const int32_t MAX_INTEGRAL = DAC_VALUE_MAX / 10;
static const char *TAG = "helper_functions.cpp";

// Function to set up error message loop
extern "C" void setupError(const char *errormessage)
{
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGE(TAG, "%s", errormessage);
    UsbPcInterface::send(errormessage);
    errorBlink();
}

// Error blink function
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

// LED status function based on ADC value
void ledStatusAdc(int16_t adcValue, uint16_t targetAdc, uint16_t toleranceAdc, uint16_t dac)
{
    static std::string last_limit = "INIT";
    static const char *TAG = "ledStatus";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    if (abs(targetAdc - adcValue) <= toleranceAdc)
    {
        if (last_limit != "LIMIT")
        {
            gpio_set_level(IO_25_RED, 0);    // red LED
            gpio_set_level(IO_27_YELLOW, 1); // yellow LED
            gpio_set_level(IO_02_GREEN, 0);  // green LED
            last_limit = "LIMIT";
        }
    }
    else if (adcValue > targetAdc)
    {
        if (last_limit != "HI")
        {
            gpio_set_level(IO_25_RED, 1);    // red LED
            gpio_set_level(IO_27_YELLOW, 0); // yellow LED
            gpio_set_level(IO_02_GREEN, 0);  // green LED
            last_limit = "HI";
        }
    }
    else
    {
        if (last_limit != "LO")
        {
            gpio_set_level(IO_25_RED, 0);    // red LED
            gpio_set_level(IO_27_YELLOW, 0); // yellow LED
            gpio_set_level(IO_02_GREEN, 1);  // green LED
            last_limit = "LO";
        }
    }
}

// Calculate ADC value from nA
uint16_t calculateAdcFromnA(double targetNa)
{

    static const char *TAG = "calculateAdcFromnA";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // Calculate ADC value based on target current and voltage divider
    double adcValue = (targetNa / ADC_VOLTAGE_DIVIDER) * (ADC_VALUE_MAX / ADC_VOLTAGE_MAX);
    ESP_LOGI(TAG, "Target nA: %f, Calculated ADC Value: %f", targetNa, adcValue);
    return static_cast<uint16_t>(std::round(adcValue));
}

// Calculate tunnel current from ADC value
double calculateTunnelNa(int16_t adcValue)
{
    // Calculate current based on ADC value and voltage divider
    double currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
    currentTunnelnA = currentTunnelnA * ADC_VOLTAGE_DIVIDER; // Apply voltage divider
    return currentTunnelnA;
}

// Compute DAC value (Proportional/Integral)
// error = targetAdc - adcValue
// proportional = kP * error
// integralError += error
// integral = kI * integralError
// dacValue = currentZDac - proportional - integral
// ----------------------------
uint16_t computePiDac(int16_t adcValue, int16_t targetAdc)
{
    static const char *TAG = "computePiDac";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // TODO Handle simulation with wired  DAC_Z to ADC inverted

    // Calculate the error
    int error = targetAdc - adcValue;
    error *= INVERT_MODE;

    integralError += error;

    // Proportional term
    int32_t proportional = kP * error; // Use int32_t to handle potential overflow and negative values

    // Clamp the proportional term to a smaller range
    if (proportional < -MAX_PROPORTIONAL)
    {
        proportional = -MAX_PROPORTIONAL;
    }
    if (proportional > MAX_PROPORTIONAL)
    {
        proportional = MAX_PROPORTIONAL;
    }

    // Integral term
    int32_t integral = kI * integralError; // Use int32_t to handle potential overflow

    // Clamp the integral term to a smaller range
    if (integral < -MAX_INTEGRAL)
    {
        integral = -MAX_INTEGRAL;
    }
    if (integral > MAX_INTEGRAL)
    {
        integral = MAX_INTEGRAL;
    }

    // Compute the DAC value
    int32_t dacValue = currentZDac - proportional - integral; // Accumulate the DAC value
    // Log the values
    ESP_LOGD(TAG, "ADC Value: %d, Target ADC: %d, Proportional: %" PRId32 ", Integral: %" PRId32 ", DAC Value: %" PRId32,
             adcValue, targetAdc, proportional, integral, dacValue);

    // Clamp the DAC value to the valid range
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
