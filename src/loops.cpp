#include "loops.h"
#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "esp_log.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <mutex>
#include "helper_functions.h"

static std::queue<std::string> tunnelQueue;
static const char *TAG = "controller";
static PIResult piresult;
static double integralError = 0.0;

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
    static const char *TAG = "measureLoop";
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "+++++++++++++++++ STARTED\n");

    static double errorTunnelNa = 0.0;
    int16_t adcValueRaw, adcValue = 0;
    std::string dataBuffer;

    static int counter = 0;
    static bool is_max = false;
    static bool is_min = false;
    while (true)
    {

        vTaskSuspend(NULL); // Sleep, will be restarted by the timer
        if (gpio_get_level(IO_04) == 1)
        {
            ESP_LOGE(TAG, "ERROR: IO_04 level is not 0 before setting it to 1");
            errorBlink();
        }
        else
        {
            gpio_set_level(IO_04, 1); // blue LED
        }
        adcValueRaw = readAdc(); // Read voltage from preamplifier
        adcValue = adcValueDebounced(adcValueRaw);
        currentTunnelnA = calculateTunnelNa(adcValue);

        errorTunnelNa = targetTunnelnA - currentTunnelnA;
        ledStatus(currentTunnelnA, targetTunnelnA, toleranceTunnelnA, currentZDac);

        // If the error is within the allowed limit, process the result
        if (abs(errorTunnelNa) <= toleranceTunnelnA)
        {
            counter = 0;
            DataElement dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac);
            {
                std::lock_guard<std::mutex> lock(dataQueueMutex);
                dataQueue.push(dataElement);
            }

            // Calculate next XY position
            if (!rtmGrid.moveOn())
            {
                vTaskResume(handleVspiLoop); // Process new XY position
            }
            else
            {
                // Signal completion and restart
                DataElement endSignal(0, 0, 0); // Use a special value to signal completion
                {
                    std::lock_guard<std::mutex> lock(dataQueueMutex);
                    dataQueue.push(endSignal);
                }
                esp_restart(); // Restart once all XY positions are complete
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
                if (!is_min && !is_max)
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
            gpio_set_level(IO_04, 0); // blue LED
        }

     
    }
}

extern "C" void findTunnelLoop(void *unused)
{
    int16_t adcValueRaw, adcValue = 0;
    std::string dataBuffer;
    bool first_loop = true;

    // Reset all outputs
    currentXDac = 0;
    currentYDac = 0;
    currentZDac = 0;
    vTaskResume(handleVspiLoop);

    int counter = 0;
    bool break_loop = false;

    int MAXCOUNTS = 300;

    while (!break_loop)
    {
        vTaskSuspend(NULL); // Sleep until resumed by TUNNEL_TIMER
        if (ACTMODE != MODE_TUNNEL_FIND)
        {
            break_loop = true;
            continue;
        }

        adcValueRaw = readAdc(); // Read voltage from preamplifier
        adcValue = adcValueDebounced(adcValueRaw);
        currentTunnelnA = calculateTunnelNa(adcValue);
        ledStatus(currentTunnelnA, targetTunnelnA, toleranceTunnelnA, currentZDac);

        if (TUNNEL_REQUEST == 0)
        {
            first_loop = true;
            continue;
        }

        if (first_loop)
        {
            first_loop = false;
            // Reset all outputs
            currentXDac = 0;
            currentYDac = 0;
            currentZDac = 0;
            vTaskResume(handleVspiLoop);
            integralError = 0;
            counter = 0;
            continue;
        }

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

        if (currentZDac == 0xFFFF)
        {
            tunnelQueue.emplace("TUNNEL,END,NO CURRENT at DAC Z = max\n");
            sendTunnelPaket();
            TUNNEL_REQUEST = 0;
        }
        else if (currentZDac == 0)
        {
            tunnelQueue.emplace("TUNNEL,END,SHORTCUT at DAC Z = 0\n");
            sendTunnelPaket();
            TUNNEL_REQUEST = 0;
        }
        if (counter == MAXCOUNTS)
        {
            tunnelQueue.emplace("TUNNEL,END,SUCCESS,%d\n", counter);
            sendTunnelPaket();
            TUNNEL_REQUEST = 0;
        }

        vTaskResume(handleVspiLoop);
    }
}

extern "C" void dataTransmissionLoop(void *unused)
{
    static const char *TAG = "dataTransmissionTask";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "+++++++++++++++++ STARTED\n");
    bool break_loop = false;

    while (!break_loop)
    {
        {
            std::lock_guard<std::mutex> lock(dataQueueMutex);
            if (!dataQueue.empty())
            {
                uint16_t X = dataQueue.front().getDataX();
                uint16_t Y = dataQueue.front().getDataY();
                uint16_t Z = dataQueue.front().getDataZ();

                if (X == 0 && Y == 0 && Z == 0)
                {
                    UsbPcInterface::send("DATA,DONE\n");
                    break_loop = true;
                }
                else
                {
                    UsbPcInterface::send("DATA,%d,%d,%d\n", X, Y, Z);
                }

                dataQueue.pop(); // Remove from queue
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }
        }
    }
}
