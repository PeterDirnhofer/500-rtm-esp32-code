// controller.cpp
#include "controller.h"
#include "project_timer.h"
#include <cmath>
#include "globalVariables.h"
#include <string>

static const char *TAG = "controller";

// ---- Hardware Initialization ----

/**
 * @brief Initializes I2C for ADC and SPI for DACs, and starts the DAC.
 *
 * This function sets up the I2C interface for the ADC and the SPI interface
 * for the DAC. If the initialization fails, the ESP device will restart.
 *
 * @return esp_err_t ESP_OK on success, or an error code if initialization fails.
 */
extern "C" esp_err_t initHardware()
{
    esp_err_t errTemp = i2cAdcInit(); // Init I2C for ADC

    if (errTemp != ESP_OK)
    {
        ESP_LOGE(TAG, "ERROR: Cannot init I2C. Return code: %d\n", errTemp);
        esp_restart(); // Restart on failure
    }

    // Init DACs
    vspiDacStart(); // Init and loop for DACs

    return ESP_OK;
}

// ---- Loop Start Functions ----

/**
 * @brief Starts the adjustment loop task and initializes the timer for adjustment mode.
 *
 * This function creates a FreeRTOS task to execute the adjustment loop and
 * initializes the corresponding timer.
 */
extern "C" void adjustStart()
{
    xTaskCreatePinnedToCore(adjustLoop, "adjustLoop", 10000, NULL, 2, &handleAdjustLoop, 1);
    timer_initialize(MODE_ADJUST_TEST_TIP);
}

/**
 * @brief Starts the measurement loop task and initializes the timer for measurement mode.
 *
 * This function creates a FreeRTOS task to execute the measurement loop and
 * initializes the corresponding timer.
 */
extern "C" void measureStart()
{
    xTaskCreatePinnedToCore(measureLoop, "measurementLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_initialize(MODE_MEASURE);
}

/**
 * @brief Starts the tunnel finding loop task.
 *
 * This function creates a task that runs the tunnel finding loop on core 1
 * of the processor with a stack size of 10 KB and initializes the timer
 * for tunnel finding mode.
 */
extern "C" void findTunnelStart()
{
    // Create the tunnel finding loop task pinned to core 1 with 10 KB stack
    xTaskCreatePinnedToCore(findTunnelLoop, "findTunnelLoop", 10000, NULL, 2, &handleTunnelLoop, 1);

    // Initialize the timer for the tunnel finding mode
    timer_initialize(MODE_TUNNEL_FIND);
}

// ---- Main Loops ----

/**
 * @brief Reads ADC values, converts them to voltages, and sends the current.
 *
 * This loop is triggered by the GPTimer tick and processes the ADC data to
 * calculate the tunnel current, which is then sent to the PC for monitoring.
 *
 * @param unused FreeRTOS task parameter, not used in this implementation.
 */
extern "C" void adjustLoop(void *unused)
{
    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be retriggered by gptimer

        int16_t adcValue = readAdc(); // Read voltage from preamplifier

        currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                          (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);

        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                           (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        UsbPcInterface::send("ADJUST,%f,%f,%d\n", adcInVolt, currentTunnelnA, adcValue);
    }
}

extern "C" void measureLoop(void *unused)
{
    static double errorTunnelNa = 0.0;
    int16_t adcValueRaw, adcValue = 0;
    string dataBuffer;

    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        adcValueRaw = readAdc(); // Read voltage from preamplifier
        adcValue = adcValueDebounced(adcValueRaw);

        currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
        currentTunnelnA = currentTunnelnA * 3; // Voltage Divider ADC Input

        if (currentTunnelnA > targetTunnelnA + toleranceTunnelnA)
        {
            gpio_set_level(IO_27, 1);
        }
        else
        {
            gpio_set_level(IO_27, 0);
        }

        errorTunnelNa = targetTunnelnA - currentTunnelnA;

        // If the error is within the allowed limit, process the result
        if (abs(errorTunnelNa) <= toleranceTunnelnA)
        {

            gpio_set_level(IO_25, 1);

            dataQueue.emplace(DataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));
            sendDataPaket();

            // Calculate next XY position
            if (!rtmGrid.moveOn())
            {
                vTaskResume(handleVspiLoop); // Process new XY position
            }
            else
            {
                sendDataPaket(1); // Send rest of data and signal completion
                esp_restart();    // Restart once all XY positions are complete
            }
        }
        // If the error is too large, adjust the Z position
        else
        {
            gpio_set_level(IO_25, 0);
            // Calculate new Z Value with PI controller
            uint16_t dacOutput = computePI(currentTunnelnA, targetTunnelnA);

            currentZDac = dacOutput;
            vTaskResume(handleVspiLoop);
        }
    }
}

// State variables for PID

double integralError = 0.0;
const double integralMax = 5000.0; // Maximum value for integral term

// Clamp function to constrain values
double clamp(double value, double minValue, double maxValue)
{
    if (value < minValue)
        return minValue;
    if (value > maxValue)
        return maxValue;
    return value;
}

// PID function to compute output every millisecond
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

    UsbPcInterface::send("target,%.2f nA,%.2f error,%.2f dac,%u\n", targetNa, currentNa, error, dacz);

    return dacz;
}

extern "C" void findTunnelLoop(void *unused)
{

    int16_t adcValueRaw, adcValue = 0;
    string dataBuffer;

    // Reset vall outputs sendTunnelPaket();
    currentXDac = 0;
    currentYDac = 0;
    currentZDac = 0;
    vTaskResume(handleVspiLoop);

    int counter = 0;

    while (counter < 50)
    {
        vTaskSuspend(NULL); // Sleep until resumed by TUNNEL_TIMER

        adcValueRaw = readAdc(); // Read voltage from preamplifier
        adcValue = adcValueDebounced(adcValueRaw);

        currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
        currentTunnelnA = currentTunnelnA * 3; // Voltage Divider ADC Input

        // Calculate new Z Value with PI controller
        uint16_t dacOutput = computePI(currentTunnelnA, targetTunnelnA);

        currentZDac = dacOutput;
        counter++;
        vTaskResume(handleVspiLoop);
    }
    // sendTunnelPaket();
    currentXDac = 0;
    currentYDac = 0;
    currentZDac = 0;
    vTaskResume(handleVspiLoop);
}

// ---- Helper Functions ----
/**
 * @brief Sends the tunnel data packet to the PC.
 *
 * This function processes the tunnel queue and sends the stored data
 * to the PC. If the terminate flag is set, it signals completion
 * after sending all data.
 *
 * @return int Returns -1 if the queue is empty; otherwise returns 0.
 */
extern "C" int sendTunnelPaket()
{
    if (tunnelQueue.empty())
    {
        ESP_LOGW(TAG, "Tunnel queue is empty. No data to send.");
        return -1; // Return early if there's no data
    }

    timer_stop(); // Pause timer to avoid timing issues during send
    // DataElementTunnel::DataElementTunnel(uint16_t dacz, uint16_t adc, bool isTunnel)

    // DataElementTunnel::DataElementTunnel(uint32_t dacz, int16_t adc, bool isTunnel, float currentNa)
    //     : dacz(dacz), adc(adc), isTunnel(isTunnel), currentNa(currentNa)

    while (!tunnelQueue.empty())
    {
        // Getter methods for queue elements
        uint32_t dacz = tunnelQueue.front().getDacZ();
        int16_t adc = tunnelQueue.front().getAdc();
        bool istunnel = tunnelQueue.front().getIsTunnel();
        float currentNa = tunnelQueue.front().getCurrentNa();
        // Conditional logic to send "tunnel" or "no"

        const char *tunnelStatus = (istunnel) ? "tunnel" : "no";

        // Send data to PC with logging
        // UsbPcInterface::send("TUNNEL;%u,%d,%s,%.2f\n", dacz, adc, tunnelStatus, currentNa);
        // Send data to PC with logging in the desired order
        UsbPcInterface::send("TUNNEL,%s,%u,%d,%.2f\n", tunnelStatus, dacz, adc, currentNa);

        tunnelQueue.pop(); // Remove processed element from queue
    }

    timer_start(); // Resume the timer after sending

    return 0; // Indicate success
}

/**
 * @brief Sends the data packet to the PC.
 *
 * This function processes the data queue and sends the stored data
 * to the PC. If the terminate flag is set, it signals completion
 * after sending all data.
 *
 * @param terminate Indicates if this is the final packet to send.
 * @return int Always returns 0 to indicate success.
 */
extern "C" int sendDataPaket(bool terminate)
{
    bool timer_was_stopped = false;
    size_t numElements = dataQueue.size();

    // Stop the timer if the queue overflows
    if (numElements > 100)
    {
        timer_was_stopped = true;
        ESP_LOGW(TAG, "Stop timer. Size of dataQueue > 100\n");
        timer_stop();
    }

    // Process and send each element in the queue
    while (!dataQueue.empty())
    {
        uint16_t X = dataQueue.front().getDataX();
        uint16_t Y = dataQueue.front().getDataY();
        uint16_t Z = dataQueue.front().getDataZ();

        UsbPcInterface::send("DATA,%d,%d,%d\n", X, Y, Z);
        dataQueue.pop(); // Remove from queue
    }

    // Send completion signal if needed
    if (terminate)
    {
        UsbPcInterface::send("DATA,DONE\n");
    }

    // Restart the timer if it was stopped
    if (timer_was_stopped)
    {
        timer_start();
    }

    return 0;
}

/**
 * @brief Constrains a value within a specified range.
 *
 * This helper function ensures that the input value is bounded
 * within the defined minimum and maximum limits.
 *
 * @param value The value to constrain.
 * @param min The minimum limit.
 * @param max The maximum limit.
 * @return double The constrained value.
 */
double constrain(double value, double min, double max)
{
    return (value < min) ? min : (value > max) ? max
                                               : value;
}

/**
 * @brief Saturates a 16-bit input value to a specified range.
 *
 * This function ensures that the input value does not exceed
 * the specified minimum and maximum boundaries.
 *
 * @param input The input value to saturate.
 * @param min The minimum limit.
 * @param max The maximum limit.
 * @return uint16_t The saturated value.
 */
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
    if (adcValue < -1000)
    {
        UsbPcInterface::send("ERROR. Wrong polarity at ADC Input. %d\n", adcValue);
        return 0;
    }

    // Low volages may be negative from Opamp
    if (adcValue < 0)
    {
        adcValue = -adcValue;
    }
    // Result: Values 0 ... 32767
    return adcValue;
}