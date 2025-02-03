#include "helper_functions.h"

#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "project_timer.h"
#include "esp_log.h"
#include <cmath>
#include <string>
#include <queue>
#include <sstream>
#include <iomanip>

const double integralMax = 5000.0; // Maximum value for integral term
static int integralErrorAdc = 0;
static const int integralMaxAdc = 1000;

extern "C" void errorBlink()
{
    while (true)
    {
        gpio_set_level(IO_04, 1);       // Turn on LED
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100 ms
        gpio_set_level(IO_04, 0);       // Turn off LED
        vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100 ms
    }
}

void ledStatusAdc(int16_t adcValue, uint16_t targetAdc, uint16_t toleranceAdc, uint16_t dac)
{
    static std::string last_limit = "INIT";
    static const char *TAG = "ledStatus";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    if (abs(targetAdc - adcValue) <= toleranceAdc)
    {
        if (last_limit != "LIMIT")
        {
            gpio_set_level(IO_25, 0); // red LED
            gpio_set_level(IO_27, 1); // yellow LED
            gpio_set_level(IO_02, 0); // green LED
            last_limit = "LIMIT";
        }
    }
    else if (adcValue > targetAdc)
    {
        if (last_limit != "HI")
        {
            gpio_set_level(IO_25, 1); // red LED
            gpio_set_level(IO_27, 0); // yellow LED
            gpio_set_level(IO_02, 0); // green LED
            last_limit = "HI";
        }
    }
    else
    {
        if (last_limit != "LO")
        {
            gpio_set_level(IO_25, 0); // red LED
            gpio_set_level(IO_27, 0); // yellow LED
            gpio_set_level(IO_02, 1); // green LED
            last_limit = "LO";
        }
    }
}

static std::string last_limit = "INIT";

void setGpioLevels()
{
    static const char *TAG = "setGpioLevels";
    gpio_set_level(IO_25, 0); // Red LED
    gpio_set_level(IO_27, 0); // Yellow LED
    gpio_set_level(IO_02, 1); // Green LED
    ESP_LOGI(TAG, "%s > LO", last_limit.c_str());
    last_limit = "LO";
}

uint16_t calculateAdcFromnA(double targetNa)
{
    // Calculate ADC value based on target current and voltage divider
    double adcValue = (targetNa / ADC_VOLTAGE_DIVIDER) * (ADC_VALUE_MAX / ADC_VOLTAGE_MAX);
    return static_cast<uint16_t>(std::round(adcValue));
}

double calculateTunnelNa(int16_t adcValue)
{
    // Calculate current based on ADC value and voltage divider
    double currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
    currentTunnelnA = currentTunnelnA * ADC_VOLTAGE_DIVIDER; // Apply voltage divider
    return currentTunnelnA;
}

uint16_t computePiDac(int16_t adcValue, int16_t targetAdc)
{
    static const char *TAG = "computePiDac";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Calculate the error
    int error = targetAdc - adcValue;
    integralErrorAdc += error;

    // Proportional term
    int16_t proportional = kP * error;

    // Integral term
    int16_t integral = kI * integralErrorAdc;

    // Compute the DAC value
    int16_t dacValue = proportional + integral;

    // Clamp the DAC value to the valid range
    if (dacValue < 0)
    {
        dacValue = 0;
    }
    else if (dacValue > DAC_VALUE_MAX)
    {
        dacValue = DAC_VALUE_MAX;
    }

    ESP_LOGI(TAG, "Computed DAC value: %d", dacValue);
    return static_cast<uint16_t>(dacValue);
}
