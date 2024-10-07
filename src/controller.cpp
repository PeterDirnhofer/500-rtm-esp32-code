// controller.cpp
#include "controller.h"
#include "project_timer.h"
using namespace std;

static const char *TAG = "controller";

/**
 * @brief Initialize I2C for ADC and SPI for DACs.
 *        Initialize ADC.
 *        Initialize DAC by starting vspiDacLoop once.
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

/**
 * @brief Start the adjust loop task and initialize the timer for adjust mode.
 */
extern "C" void adjustStart()
{

    xTaskCreatePinnedToCore(adjustLoop, "adjustLoop", 10000, NULL, 2, &handleAdjustLoop, 1);
    timer_initialize(MODE_ADJUST_TEST_TIP);
}

/**
 * @brief The adjust loop reads ADC values, converts them to voltages,
 *        and sends the current. Triggered by GPTimer tick.
 */
extern "C" void adjustLoop(void *unused)
{

    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be retriggered by gptimer

        int16_t adcValue = readAdc(); // Read voltage from preamplifier

        actualTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                                (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);

        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                           (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        UsbPcInterface::send("ADJUST,%f,%d\n", adcInVolt, adcValue);
    }
}

/**
 * @brief Start the measurement loop task and initialize the timer for measurement mode.
 */
extern "C" void measurementStart()
{
    xTaskCreatePinnedToCore(measurementLoop, "measurementLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_initialize(MODE_MEASURE);
}

/**
 * @brief The measurement loop runs an ADC measure cycle, adjusts Z position,
 *        and sends ADC values with X, Y positions to the PC.
 */
extern "C" void measurementLoop(void *unused)
{
    static double e = 0, w = 0, r = 0, z = 0, eOld = 0, zOld = 0;
    uint16_t zSaturate = 0;
    w = destinationTunnelCurrentnA; // Desired tunnel current

    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        int16_t adcValue = readAdc(); // Read voltage from preamplifier
        adcValue = abs(adcValue);

        // Convert ADC value to tunnel current (nA)
        actualTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                                (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        r = actualTunnelCurrentnA; // Actual tunnel current
        e = w - r;                 // Error = desired - actual

        // If the error is within the allowed limit, process the result
        if (abs(e) <= remainingTunnelCurrentDifferencenA)
        {

            gpio_set_level(IO_25, 1);
            // Store the result in the data queue
            dataQueue.emplace(DataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac));

            // Send data to PC
            m_sendDataPaket();

            // Calculate new XY position
            if (!rtmGrid.moveOn())
            {
                vTaskResume(handleVspiLoop); // Process new XY position
            }
            else
            {
                m_sendDataPaket(1); // Send rest of data and signal completion
                esp_restart();      // Restart once all XY positions are complete
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
            zSaturate = m_saturate16bit((uint32_t)z, 0, DAC_VALUE_MAX);
            currentZDac = zSaturate; // Set new Z height

            // Resume DAC loop to set the new Z position
            vTaskResume(handleVspiLoop);
        }

        zOld = zSaturate; // Store previous control Z-value
    }
}

// TODO refactor
/*
Refactor in parameter:
destinationTunnelCurrentnA:  targetTunnelCurrentnA
remainingTunnelCurrentDifferencenA : ToleranceTunnelCurrentnA
Add kD
*/

/**
 * @brief Start the tunnel finding loop task.
 */
extern "C" void findTunnelStart()
{
    // Create the tunnel finding loop task pinned to core 1 with 10 KB stack
    xTaskCreatePinnedToCore(findTunnelLoop, "findTunnelLoop", 10000, NULL, 2, &handleTunnelLoop, 1);

    // Initialize the timer for the tunnel finding mode
    timer_initialize(MODE_TUNNEL_FIND);
}

/**
 * @brief The tunnel finding loop function.
 */
extern "C" void findTunnelLoop(void *unused)
{
    static double delta = 0, target = 0, z = 0, deltaSum = 0, deltaOld = 0;

    // TODO: Add to parameter
    double kD = 10.0;

    target = destinationTunnelCurrentnA; // Desired target tunnel current
    int counter = 0;

    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        int16_t adcValue = readAdc(); // Read voltage from preamplifier
        adcValue = abs(adcValue);

        // TODO: Voltage divider and supply voltage OP amp is missing
        // Convert ADC value to tunnel current (nA)
        actualTunnelCurrentnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                                (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        delta = target - actualTunnelCurrentnA; // Error = target - measured

        // If the error is within the allowed limit, process the result
        if (abs(delta) <= remainingTunnelCurrentDifferencenA)
        {
            gpio_set_level(IO_25, 1); // Indicate within tolerance

            // Store the result in the data queue
            tunnelQueue.emplace(DataElementTunnel(currentZDac, actualTunnelCurrentnA, true));
        }
        // If the error is too large, adjust the Z position
        else
        {
            gpio_set_level(IO_25, 0); // Set GPIO to signal out of tolerance

            // Calculate PID components
            double P = kP * delta;              // Proportional term
            double I = kI * deltaSum;           // Integral term (accumulate previous error)
            double D = kD * (delta - deltaOld); // Derivative term (error change rate)

            deltaOld = delta; // Store current delta for next iteration

            // Combine PID terms to adjust Z position
            z = P + I + D;

            // Constrain Z value to the allowed DAC limits
            if (z > DAC_VALUE_MAX)
            {
                z = DAC_VALUE_MAX;
            }
            else if (z < 0)
            {
                z = 0;
            }
            else
            {
                // Update integral term only when Z is not saturated
                deltaSum += delta;
                const double MAX_INTEGRAL = 1000; // Example max value for integral term

                if (deltaSum > MAX_INTEGRAL)
                {
                    deltaSum = MAX_INTEGRAL;
                }
                else if (deltaSum < -MAX_INTEGRAL)
                {
                    deltaSum = -MAX_INTEGRAL;
                }
            }

            currentZDac = z; // Update Z height for the tunnel system

            // Add data to the tunnel queue and send data to the PC
            tunnelQueue.emplace(DataElementTunnel(currentZDac, static_cast<float>(actualTunnelCurrentnA), false));

            // Resume DAC loop to set the new Z position
            vTaskResume(handleVspiLoop);
        }

        counter++;
        if (counter >= TUNNEL_FIMD_MAX_COUNT)
        {
            counter = 0;
            currentZDac = 0; // Reset DAC position
            deltaSum = 0;    // Reset integral term

            // Send data to PC
            m_SendTunnelPaket();
        }
    }
}

/**
 * @brief Saturates an input value within the given 16-bit boundaries.
 *
 * @param input The value to be saturated.
 * @param min The minimum allowed value.
 * @param max The maximum allowed value.
 * @return uint16_t The saturated value.
 */
uint16_t m_saturate16bit(uint32_t input, uint16_t min, uint16_t max)
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
extern "C" int m_SendTunnelPaket(bool terminate)
{
    bool timer_was_stopped = false; // Flag to check if the timer was stopped
    size_t numElements = tunnelQueue.size();

    // Stop the timer if the queue overflows
    if (numElements > 100)
    {
        timer_was_stopped = true;
        ESP_LOGW(TAG, "Stop timer. Size of tunnelQueue > 100\n");
        timer_stop();
    }

    // Process and send each element in the queue
    while (!tunnelQueue.empty())
    {
        // Getter methods for queue elements
        uint16_t adcz = tunnelQueue.front().getDataZ();
        float current = tunnelQueue.front().getCurrent();
        bool istunnel = tunnelQueue.front().getIsTunnel();

        // Send data to PC
        UsbPcInterface::send("TUNNEL,%u,%f,%s\n", adcz, current, istunnel ? "ON" : "OFF");
        tunnelQueue.pop(); // Remove processed element from queue
    }

    // Send completion signal if needed
    if (terminate)
    {
        UsbPcInterface::send("TUNNEL,DONE\n");
    }

    // Restart the timer if it was stopped
    if (timer_was_stopped)
    {
        timer_start();
    }

    return 0; // Indicate success
}

/**
 * @brief Sends the measurement data in the queue to the PC.
 *        Pauses the measurement if the queue is too full.
 *
 * @param terminate Whether to terminate after sending data.
 * @return int 0 on success.
 */
extern "C" int m_sendDataPaket(bool terminate)
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
