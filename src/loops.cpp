#include "loops.h"
#include "UsbPcInterface.h"
#include "esp_log.h"
#include "globalVariables.h"
#include "helper_functions.h"
#include <cmath>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

extern "C" void adjustLoop(void *unused)
{
    static const char *TAG = "adjustLoop";
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "+++++++++ START");
    while (true)
    {
        vTaskSuspend(NULL); // Sleep, will be retriggered by gptimer
        ESP_LOGI(TAG, "AAAAAAAAAAAAAAAAAAAAAAA");
        int16_t adcValue = readAdc(); // Read voltage from preamplifier
        currentTunnelnA = calculateTunnelNa(adcValue);

        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                           (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        UsbPcInterface::send("ADJUST,%.3f,%.3f,%d\n", adcInVolt, currentTunnelnA,
                             adcValue);
    }
}

extern "C" void measureLoop(void *unused)
{
    static const char *TAG = "measureLoop";
    esp_log_level_set(TAG, ESP_LOG_WARN);
    ESP_LOGI(TAG, "+++++++++++++++++ STARTED\n");
    static double errorTunnelNa = 0.0;
   
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

        int16_t adcValue = readAdc(); // Read voltage from preamplifier
        currentTunnelnA = calculateTunnelNa(adcValue);
        errorTunnelNa = targetTunnelnA - currentTunnelnA;
      
        ledStatus(currentTunnelnA, targetTunnelnA, toleranceTunnelnA, currentZDac);
       
        // Log the values of currentTunnelnA, targetTunnelnA, toleranceTunnelnA, and
        // currentZDac

        // If the error is within the allowed limit, process the result
        if (abs(errorTunnelNa) <= toleranceTunnelnA)
        {
            ESP_LOGI(TAG, "In limit");
            counter = 0;
            DataElement dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(),
                                    currentZDac);
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
                DataElement endSignal(0, 0,
                                      0); // Use a special value to signal completion
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
            ESP_LOGI(TAG, "piresult.targetNa: %.2f, piresult.currentNa: %.2f, piresult.error: %.2f, dacOutput: %u",
                     piresult.targetNa, piresult.currentNa, piresult.error, dacOutput);
            counter++;
            if (counter % 200 == 0)
            {
                if (!is_min && !is_max)
                {
                    UsbPcInterface::send("LOG,tar,%.2f,nA,%.2f,err,%.2f,dac,%u\n",
                                         piresult.targetNa, piresult.currentNa,
                                         piresult.error, dacOutput);
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

    // int16_t adcValueRaw, adcValue = 0;
    // std::string dataBuffer;
    // bool first_loop = true;

    // // Reset all outputs
    // currentXDac = 0;
    // currentYDac = 0;
    // currentZDac = 0;
    // vTaskResume(handleVspiLoop);

    // int counter = 0;
    // bool break_loop = false;

    // int MAXCOUNTS = 300;

    // while (!break_loop)
    // {
    //     vTaskSuspend(NULL); // Sleep until resumed by TUNNEL_TIMER
    //     if (ACTMODE != MODE_TUNNEL_FIND)
    //     {
    //         break_loop = true;
    //         continue;
    //     }

    // }
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
