#include "loops.h"
#include "helper_functions.h"
#include "UsbPcInterface.h"
#include "esp_log.h"
#include "globalVariables.h"
#include <cmath>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include "project_timer.h"

static bool isLoopExecution = false;
static const size_t BUFFER_SIZE = 1024; // Define BUFFER_SIZE

// Function to generate a 1kHz sinusoidal signal at DAC outputs X, Y, and Z
extern "C" void sinusLoop(void *params)
{
    static const char *TAG = "testLoop";
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "+++ START TEST LOOP");

    const uint16_t amplitude = DAC_VALUE_MAX / 2; // Amplitude of the sine wave (half of the DAC range)
    const uint16_t offset = DAC_VALUE_MAX / 2;    // Offset to center the sine wave in the DAC range
    const double frequency = 1000.0;              // Frequency of the sine wave in Hz
    const double increment = (2.0 * M_PI * frequency) / BUFFER_SIZE;
    double phase = 0.0;
    std::unique_ptr<uint16_t[]> buffer(new uint16_t[BUFFER_SIZE]);
    ESP_LOGD(TAG, "Foo 1");
    // Precompute the buffer values
    for (size_t i = 0; i < BUFFER_SIZE; ++i)
    {
        buffer[i] = static_cast<uint16_t>(amplitude * sin(phase) + offset);
        phase += increment;
        if (phase >= 2.0 * M_PI)
        {
            phase -= 2.0 * M_PI;
        }
    }
    ESP_LOGD(TAG, "Foo 2");
    while (sinusIsActive)
    {

        // ESP_LOGI(TAG, ".");
        //  Send the precomputed buffer values to the DAC
        for (size_t i = 0; i < BUFFER_SIZE; ++i)
        {
            if (!sinusIsActive)
                break;
            vspiSendDac(buffer[i], buffer.get(), handleDacX);
            vspiSendDac(buffer[i], buffer.get(), handleDacY);
            vspiSendDac(buffer[i], buffer.get(), handleDacZ);

            // Delay to achieve the desired sample rate
            vTaskDelay(pdMS_TO_TICKS(1)); // Adjust delay as needed for the desired sample rate
        }
    }
    ESP_LOGD(TAG, "stop and vTaskDelete");
    vTaskDelete(NULL);
}

// Adjust loop task
extern "C" void adjustLoop(void *unused)
{
    static const char *TAG = "adjustLoop";
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGI(TAG, "+++ START ADJUST\n");

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
    ESP_LOGI(TAG, "+++ STARTED");

    uint16_t newDacZ = 0;
    std::string dataBuffer;

    while (measureIsActive)
    {
        isLoopExecution = false;
        vTaskSuspend(NULL); // Sleep, will be restarted by the timer

        // Check if the task is already running
        if (isLoopExecution)
        {
            ESP_LOGE(TAG, "ERROR: measureLoop retriggered before last run was finished");
            errorBlink();
        }
        isLoopExecution = true; // Set the flag to indicate the task is running

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
                // Wait until complete queue was sent to PC
                while (uxQueueMessagesWaiting(queueToPc) > 0)
                {
                    vTaskDelay(pdMS_TO_TICKS(10)); // Delay for a short period
                }

                esp_restart(); // Restart
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
