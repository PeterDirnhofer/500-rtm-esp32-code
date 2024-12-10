// controller.cpp
#include "controller.h"
#include "project_timer.h"
#include <cmath>
#include "globalVariables.h"
#include <string>
#include <queue>
#include <sstream>
#include <iomanip>

static const char *TAG = "controller";
static PIResult piresult;

static queue<string> tunnelQueue;

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
        currentTunnelnA = calculateTunnelNa(adcValue);

        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                           (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        UsbPcInterface::send("ADJUST,%.3f,%.3f,%d\n", adcInVolt, currentTunnelnA, adcValue);
    }
}

extern "C" void measureLoop(void *unused)
{
    static double errorTunnelNa = 0.0;
    int16_t adcValueRaw, adcValue = 0;
    string dataBuffer;

    static int counter = 0;
    static bool is_max = false;
    static bool is_min = false;
    while (true)
    {

        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        adcValueRaw = readAdc(); // Read voltage from preamplifier
        adcValue = adcValueDebounced(adcValueRaw);
        currentTunnelnA = calculateTunnelNa(adcValue);

        errorTunnelNa = targetTunnelnA - currentTunnelnA;
        ledStatus(currentTunnelnA, targetTunnelnA, toleranceTunnelnA, currentZDac);

        // If the error is within the allowed limit, process the result
        if (abs(errorTunnelNa) <= toleranceTunnelnA)
        {
            counter = 0;
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

            // Calculate new Z Value with PI controller
            uint16_t dacOutput = computePI(currentTunnelnA, targetTunnelnA);

            counter++;
            if (counter % 200 == 0)
            {
                if (is_min == false && is_max == false)
                {
                    UsbPcInterface::send("LOG,tar,%.2f,nA,%.2f,err,%.2f,dac,%u\n", piresult.targetNa, piresult.currentNa, piresult.error, dacOutput);
                }

                if (dacOutput == 0xFFFF)
                {
                    is_max = true;
                }
                else
                {
                    is_max = false;
                }
                if (dacOutput == 0)
                {
                    is_min = true;
                }
                else
                {
                    is_min = false;
                }

                if (dacOutput == 0xFFFF)
                {
                    dacOutput = 1;
                }
            }

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

    piresult.targetNa = targetNa;
    piresult.currentNa = currentNa;
    piresult.error = error;
    piresult.dacz = dacz;

    // UsbPcInterface::send("target,%.2f nA,%.2f error,%.2f dac,%u\n", targetNa, currentNa, error, dacz);

    return dacz;
}
extern "C" void findTunnelLoop(void *unused)
{
    int16_t adcValueRaw, adcValue = 0;
    string dataBuffer;

    // Reset all outputs
    currentXDac = 0;
    currentYDac = 0;
    currentZDac = 0;
    vTaskResume(handleVspiLoop);

    int counter = 0;
    bool break_loop = false;

    int MAXCOUNTS = 300;

    while ((counter < MAXCOUNTS) && !break_loop)
    {
        vTaskSuspend(NULL); // Sleep until resumed by TUNNEL_TIMER

        
        adcValueRaw = readAdc(); // Read voltage from preamplifier
        adcValue = adcValueDebounced(adcValueRaw);
        currentTunnelnA = calculateTunnelNa(adcValue);
        ledStatus(currentTunnelnA, targetTunnelnA, toleranceTunnelnA, currentZDac);

        // Calculate new Z Value with PI controller
        uint16_t dacOutput = computePI(currentTunnelnA, targetTunnelnA);

        // Create a stringstream to format message
        std::ostringstream messageStream;
        messageStream << "TUNNEL,tar,"
                      << std::fixed << std::setprecision(2) << piresult.targetNa << ",nA,"
                      << std::fixed << std::setprecision(2) << piresult.currentNa << ",dac," << std::to_string(dacOutput) << "\n";

        // Add message to tunnelQueue
        tunnelQueue.emplace(messageStream.str()); // Add formatted message to queue

        currentZDac = dacOutput;
        counter++;

        


        // // Send packet and debug log every 50 iterations
        // if (counter % 10 == 0)
        // {
        //     // Log before calling sendTunnelPaket
        //     UsbPcInterface::send("Calling sendTunnelPaket, counter: %d\n", counter);
        //     int sendResult = sendTunnelPaket(); // Capture result for debugging
        //     vTaskDelay(10 / portTICK_PERIOD_MS);

        //     if (sendResult != 0)
        //     {
        //         UsbPcInterface::send("Error sending tunnel packet. Result: %d\n", sendResult);
        //     }
        // }

        if (currentZDac == 0xFFFF)
        {

            UsbPcInterface::send("TUNNEL,END,NO CURRENT at DAC Z = max\n");
            vTaskDelay(10 / portTICK_PERIOD_MS);
            break_loop = true;
        }
        else if (currentZDac == 0)
        {
            //tunnelQueue.emplace("TUNNEL,END,SHORTCUT at DAC Z = 0\n");
            UsbPcInterface::send("TUNNEL,END,SHORTCUT at DAC Z = 0\n");
            vTaskDelay(10 / portTICK_PERIOD_MS);
            break_loop = true;
        }
        vTaskResume(handleVspiLoop);
    }

    if (counter == MAXCOUNTS)
    {
        UsbPcInterface::send("TUNNEL,END,COUNTER,%d\n", counter);
    }

   
    sendTunnelPaket(); // Capture final result for debugging
    //vTaskDelay(2000 / portTICK_PERIOD_MS);
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

    while (!tunnelQueue.empty())
    {
        const string &message = tunnelQueue.front();

        if (UsbPcInterface::send(message.c_str()) < 0) // Send the message
        {
            return -2; // Error occurred while sending
        }
        tunnelQueue.pop(); // Remove the message after successful send
    }

    // Reinitialize the queue (clear the queue)
    tunnelQueue = queue<string>(); // Create a new empty queue

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
    if (adcValue < -2000)
    {
        gpio_set_level(IO_25, 1); // red LED
        gpio_set_level(IO_27, 1); // yellow LED
        gpio_set_level(IO_02, 1); // green LED

        UsbPcInterface::send("ERROR. Wrong polarity at ADC Input. %d\n", adcValue);
        //vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
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

void ledStatus(double currentTunnelnA, double targetTunnelnA, double toleranceTunnelnA, uint16_t dac)
{

    if (abs(targetTunnelnA - currentTunnelnA) <= toleranceTunnelnA)
    {
        gpio_set_level(IO_25, 0); // red LED
        gpio_set_level(IO_27, 1); // yellow LED
        gpio_set_level(IO_02, 0); // green LED
    }
    else if (currentTunnelnA > targetTunnelnA)
    {
        gpio_set_level(IO_25, 1); // red LED
        gpio_set_level(IO_27, 0); // yellow LED
        gpio_set_level(IO_02, 0); // green LED
    }
    else
    {
        gpio_set_level(IO_25, 0); // red LED
        gpio_set_level(IO_27, 0); // yellow LED
        gpio_set_level(IO_02, 1); // green LED
    }
}

double calculateTunnelNa(int16_t adcValue)
{
    currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX) / ADC_VALUE_MAX;
    currentTunnelnA = currentTunnelnA * 3; // Voltage Divider ADC Input
    return currentTunnelnA;
}