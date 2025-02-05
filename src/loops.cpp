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
#include "project_timer.h"

static bool isRunning = false;

// Adjust loop task
extern "C" void adjustLoop(void *unused)
{
    static const char *TAG = "adjustLoop";
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "+++++++++++++++++ START ADJUST\n");
   

    while (adjustIsActive)
    {
        // Read voltage from preamplifier
        int16_t adcValue = readAdc();
        currentTunnelnA = calculateTunnelNa(adcValue);

        // Calculate ADC voltage in volts
        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) / (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);

        // Send data via USB interface
        UsbPcInterface::send("ADJUST,%.3f,%.3f,%d\n", adcInVolt, currentTunnelnA, adcValue);
        // Delay for a specified period (e.g., 1000 ms)
    
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
   
    vTaskDelete(NULL);
}

// Measurement loop task
extern "C" void measureLoop(void *unused)
{
    static const char *TAG = "measureLoop";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "+++++++++++++++++ STARTED\n");

    uint16_t newDacZ = 0;
    std::string dataBuffer;

    while (measureIsActive)
    {
        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        // Check if the task is already running
        if (isRunning)
        {
            ESP_LOGE(TAG, "ERROR: measureLoop retriggered before last run was finished");
            errorBlink();
        }

        isRunning = true; // Set the flag to indicate the task is running

        // Check IO_04 level and set blue LED
        if (gpio_get_level(IO_04) == 1)
        {
            ESP_LOGE(TAG, "ERROR: IO_04 level is not 0 before setting it to 1");
            errorBlink();
        }
        // To signal that measure task is active 
        gpio_set_level(IO_04, 1); // blue LED
        

        // Read voltage from preamplifier
        int16_t adcValue = readAdc();
        ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);

        // Calculate error
        int16_t errorAdc = targetTunnelAdc - adcValue;

        // Check if current is within limit
        if (abs(errorAdc) <= toleranceTunnelAdc)
        {
            if (rtmGrid.getCurrentX() == 0)
            {

                ESP_LOGI(TAG, "Current Y position: %d", rtmGrid.getCurrentY());
            }

            // Create data element and send to queue
            DataElement dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac);
            if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS)
            {
                ESP_LOGE("Queue", "Failed to send to queue");
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
                if (xQueueSend(queueToPc, &endSignal, portMAX_DELAY) != pdPASS)
                {
                    ESP_LOGE("Queue", "Failed to send to queue");
                }

                
                timer_stop();
                // Wait until the queue is empty
                while (uxQueueMessagesWaiting(queueToPc) > 0)
                {
                    vTaskDelay(pdMS_TO_TICKS(10)); // Delay for a short period
                }

                esp_restart(); // Restart once all XY positions are complete
            }
        }
        else
        {
            // Compute new DAC value and resume VSPI loop
            newDacZ = computePiDac(adcValue, targetTunnelAdc);
            currentZDac = newDacZ;
            vTaskResume(handleVspiLoop);
        }

        gpio_set_level(IO_04, 0); // signal, that measure task had finished this iteration
        isRunning = false;
    }
    vTaskDelete(NULL);
}

// Data transmission loop task
extern "C" void dataTransmissionLoop(void *unused)
{
    static const char *TAG = "dataTransmissionTask";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    bool break_loop = false;

    while (!break_loop)
    {
        DataElement element;
        // Receive data from the queue
        if (xQueueReceive(queueToPc, &element, portMAX_DELAY) == pdPASS)
        {
            uint16_t X = element.getDataX();
            uint16_t Y = element.getDataY();
            uint16_t Z = element.getDataZ();

            if (X == 0 && Y == 0 && Z == 0)
            {
                UsbPcInterface::send("DATA,DONE\n");
                break_loop = true;
            }
            else
            {
                // Process the data element
                std::ostringstream oss;
                oss << "DATA," << X << "," << Y << "," << Z << "\n";
                UsbPcInterface::send(oss.str().c_str());
                // Add a delay to wait until send is done
                vTaskDelay(pdMS_TO_TICKS(1)); // 1 millisecond delay
            }
        }
        else
        {
            ESP_LOGE(TAG, "Failed to receive from queue");
        }
    }
}
