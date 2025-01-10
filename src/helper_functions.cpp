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

static const char *TAG = "helper_functions";

static PIResult piresult;
static double integralError = 0.0; // State variables for PID
const double integralMax = 5000.0; // Maximum value for integral term

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


double constrain(double value, double min, double max)
{
    return (value < min) ? min : (value > max) ? max
                                               : value;
}

uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max)
{
    if (input < min)
    {
        return min;
    }
    if (input > max)
    {
        return max;
    }
    return static_cast<uint16_t>(input);
}

int16_t adcValueDebounced(int16_t adcValue)
{
    // Error if high value negative. Low voltage negative is ok.
    if (adcValue < -2000)
    {
        gpio_set_level(IO_25, 1); // red LED
        gpio_set_level(IO_27, 1); // yellow LED
        gpio_set_level(IO_02, 1); // green LED

        UsbPcInterface::send("ERROR. Wrong polarity at ADC Input. %d\n", adcValue);
        // vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
        return 0;
    }

    // Low voltages may be negative from Opamp
    if (adcValue < 0)
    {
        adcValue = -adcValue;
    }
    // Result: Values 0 ... 32767
    return adcValue;
}

void ledStatus(double currentTunnelnA, double targetTunnelnA, double toleranceTunnelnA, uint16_t dac)
{
    static std::string last_limit = "";
    static const char *TAG = "ledStatus";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    if (abs(targetTunnelnA - currentTunnelnA) <= toleranceTunnelnA)
    {
        
        
        if(last_limit != "LIMIT"){
            gpio_set_level(IO_25, 0); // red LED
            gpio_set_level(IO_27, 1); // yellow LED
            gpio_set_level(IO_02, 0); // green LED
            ESP_LOGI(TAG, "%s -> LIMIT", last_limit.c_str());
            last_limit = "LIMIT";
        }
    }
    else if (currentTunnelnA > targetTunnelnA)
    {
       
        if (last_limit != "HI")
        {
            gpio_set_level(IO_25, 1); // red LED
            gpio_set_level(IO_27, 0); // yellow LED
            gpio_set_level(IO_02, 0); // green LED
            ESP_LOGI(TAG, "%s > HI", last_limit.c_str());
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
            ESP_LOGI(TAG, "%s > LO", last_limit.c_str());
            last_limit = "LO";
        }
    }
}

double calculateTunnelNa(int16_t adcValue)
{
    double currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
    currentTunnelnA = currentTunnelnA * 3; // Voltage Divider ADC Input
    return currentTunnelnA;
}

double clamp(double value, double minValue, double maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

uint16_t computePI(double currentNa, double targetNa)
{
    // Calculate error
    double error = targetNa - currentNa;

    // Update integral term with clamping to prevent windup
    integralError += error;
    integralError = clamp(integralError, -integralMax, integralMax);

    // Compute output using proportional and integral terms
    double output = kP * error + kI * integralError;

    // Clamp output to range [0, maxOutput]
    output = clamp(output, 0.0, DAC_VALUE_MAX);

    uint16_t dacz = static_cast<uint16_t>(std::round(output));

    piresult.targetNa = targetNa;
    piresult.currentNa = currentNa;
    piresult.error = error;
    piresult.dacz = dacz;

    // UsbPcInterface::send("target,%.2f nA,%.2f error,%.2f dac,%u\n", targetNa, currentNa, error, dacz);

    return dacz;
}