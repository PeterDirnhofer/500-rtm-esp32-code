// controller.cpp
#include "controller.h"
#include "project_timer.h"
#include <cmath>
#include "globalVariables.h"

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

/**
 * @brief Runs an ADC measure cycle, adjusts the Z position, and sends data to the PC.
 *
 * This loop measures the current tunnel current using the ADC, compares it
 * to the target, adjusts the Z position accordingly, and sends the ADC values
 * along with X, Y positions to the PC.
 *
 * @param unused FreeRTOS task parameter, not used in this implementation.
 */
extern "C" void measureLoop(void *unused)
{
    static double e = 0, w = 0, r = 0, z = 0, eOld = 0, zOld = 0;
    uint16_t zSaturate = 0;
    w = targetTunnelnA; // Desired tunnel current

    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        int16_t adcValue = readAdc(); // Read voltage from preamplifier
        // adcValue = abs(adcValue);

        // Convert ADC value to tunnel current (nA)
        currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                          (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        r = currentTunnelnA; // Actual tunnel current
        e = w - r;           // Error = desired - actual

        // If the error is within the allowed limit, process the result
        if (abs(e) <= toleranceTunnelnA)
        {
            gpio_set_level(IO_25, 1);

            if (OPTION_FIND == 0)
            {
                // Store the result in the data queue
                dataQueue.emplace(DataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));
                // Send data to PC
                sendDataPaket();
            }

            // Calculate new XY position
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
            // Calculate new control value
            z = kP * e + kI * eOld + zOld;
            eOld = e;

            // Saturate the control value to fit within DAC boundaries
            zSaturate = saturate16bit((uint32_t)z, 0, DAC_VALUE_MAX);
            currentZDac = zSaturate; // Set new Z height

            // Resume DAC loop to set the new Z position
            vTaskResume(handleVspiLoop);
        }

        if (OPTION_FIND == 1)
        {
            // Store the result in the data queue
            dataQueue.emplace(DataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));
            // Send data to PC
            sendDataPaket();
        }

        zOld = zSaturate; // Store previous control Z-value
    }
}

/**
 * @brief Finds the tunnel current using a PID control loop.
 *
 * This timer-triggered loop continuously adjusts the Z-height using a PID controller to maintain
 * the desired tunneling current. It measures the current tunnel current using
 * the ADC, compares it to the target, and updates the Z-height accordingly.
 * The loop runs until the system stabilizes within a set tolerance for the
 * tunneling current or reaches a maximum iteration count.
 *
 * Key components include:
 *  - Proportional (P), Integral (I), and Derivative (D) terms for PID control.
 *  - Saturation of Z-height to DAC limits to prevent over-control.
 *  - Error delta (`delta`), target tunnel current (`targetTunnelCurrentnA`), and
 *    measured tunnel current (`measuredTunnelCurrentnA`) are used for feedback control.
 *  - Data is sent to the PC for monitoring or logging, and Z-position is updated using SPI.
 *
 * @param unused FreeRTOS task parameter, not used in this implementation.
 */
extern "C" void findTunnelLoop(void *unused)
{

    static const char *TAG = "findTunnelLoop";
    static double e = 0, w = 0, r = 0, z = 0, eOld = 0, zOld = 0;
    uint16_t zSaturate = 0;
    ESP_LOGI(TAG, "STARTED +++ ");

    w = targetTunnelnA; // Desired tunnel current
    bool is_in_tunnel = false;
    uint16_t counter = 0;
    uint16_t currentdac = 0;
    while (counter < 200)
    {

        vTaskSuspend(NULL);           // Sleep until resumed by a timer
        int16_t adcValue = readAdc(); // Read voltage from preamplifier

        // Convert ADC value to tunnel current (nA)
        currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                          (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        r = currentTunnelnA; // Actual tunnel current
        e = w - r;           // Error = desired - actual

        if (abs(e) <= toleranceTunnelnA)
        {
            gpio_set_level(IO_25, 1);
            is_in_tunnel = true;
        }
        else
        {
            is_in_tunnel = false;
            gpio_set_level(IO_25, 0);
            z = kP * e + kI * eOld + zOld;
            eOld = e;

            // Saturate the control value to fit within DAC boundaries
            zSaturate = saturate16bit((uint32_t)z, 0, DAC_VALUE_MAX);
            currentZDac = zSaturate; // Set new Z height
            currentdac = currentZDac;

            // Resume DAC loop to set the new Z position
            vTaskResume(handleVspiLoop);
        }
        zOld = zSaturate;
        tunnelQueue.emplace(DataElementTunnel(currentdac, adcValue, is_in_tunnel, currentTunnelnA));
        counter++;
        // Send data to PC
        // if (counter % 100 == 0)
        // {
        //     sendTunnelPaket();
        // }
    }
    sendTunnelPaket();
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
        //UsbPcInterface::send("TUNNEL;%u,%d,%s,%.2f\n", dacz, adc, tunnelStatus, currentNa);
        // Send data to PC with logging in the desired order
        UsbPcInterface::send("TUNNEL,%s,%.2f,%u\n", tunnelStatus, currentNa, dacz);

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
