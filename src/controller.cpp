// controller.cpp

#include "controller.h"
#include "project_timer.h"
#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "esp_log.h"
#include "loops.h"
#include "helper_functions.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "ParameterSetter.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

static const char *TAG = "controller";

extern "C" void dispatcherTask(void *unused)
{

    static const char *TAG = "dispatcherTask";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "STARTED");
    char rcvChars[255];

    if (queueFromPc == NULL)
    {
        ESP_LOGE(TAG, "queueFromPc not initialized");
        vTaskDelete(NULL); // Delete the task if queue is not initialized
    }
    while (1)
    {
        // Wait for data to be available in the queue with a timeout of 100 ms
        if (xQueueReceive(queueFromPc, &rcvChars, pdMS_TO_TICKS(100)) == pdPASS)
        {

            std::string receive(rcvChars);
            // Remove extra characters (whitespace, newline, etc.)
            receive.erase(std::remove_if(receive.begin(), receive.end(), ::isspace), receive.end());

            if (receive == "STOP")
            {
                adjustIsActive = false;
                measureIsActive = false;
                UsbPcInterface::send("STOPPED\n");
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            }

            if (receive == "RESTART")
            {
                ESP_LOGI(TAG, "System reset initiated");
                esp_restart();
                continue;
            }

            if (receive == "MEASURE")
            {
                measureStart();
                continue;
            }

            // Adjust and TIP
            if (receive == "ADJUST")
            {
                adjustStart();
                continue;
            }

            if (receive.rfind("TIP,", 0) == 0)
            {
                if (adjustIsActive)
                {
                    esp_err_t updateSuccess = UsbPcInterface::mUpdateTip(receive);
                    if (updateSuccess != ESP_OK)
                    {

                        ESP_LOGW(TAG, "ERROR: Failed to update TIP");
                        UsbPcInterface::send("ERROR: Failed to update TIP");
                    }

                    continue;
                }
                else
                {
                    ESP_LOGW(TAG, "ERROR: Received TIP command while adjust is not active");
                    UsbPcInterface::send("Received TIP command while adjust is not active");
                    continue;
                }
            }

            // Parameter
            if (receive == "PARAMETER,?")
            {
                ParameterSetting parameterSetter;
                parameterSetter.getParametersFromFlash();
                std::string storedParameters = parameterSetter.getParameters();
                std::istringstream stream(storedParameters);
                std::string line;
                while (std::getline(stream, line))
                {

                    line = "PARAMETER," + line;
                    line += "\n";
                    UsbPcInterface::send(line.c_str());
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                continue;
            }

            if (receive == "PARAMETER,DEFAULT")
            {
                ESP_LOGI(TAG, "parameter,default ++++");
                ParameterSetting parameterSetter;
                parameterSetter.putDefaultParametersToFlash();
                // putDefaultParametersToFlash PARAMETER,0.100000,0.010000,0.001000,1.000000,0.300000,0,0,1,0,199,199,100
                parameterSetter.getParametersFromFlash();
                continue;
            }

            if (receive.rfind("PARAMETER,", 0) == 0)
            {
                ParameterSetting parameterSetter;
                parameterSetter.putParametersToFlashFromString(receive);
                parameterSetter.getParametersFromFlash();
                continue;
            }
        }
    }
}

extern "C" void dispatcherTaskStart()
{
    // // Initialize the queue if it hasn't been initialized
    // // TODO
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Create the dispatcher task
    xTaskCreatePinnedToCore(dispatcherTask, "dispatcherTask", 10000, NULL, 4, NULL, 0);
}

extern "C" void adjustStart()
{
    if (!adjustIsActive)
    {
        adjustIsActive = true;
        xTaskCreatePinnedToCore(adjustLoop, "adjustLoop", 10000, NULL, 2, &handleAdjustLoop, 1);
    }
}

extern "C" void measureStart()
{
    measureIsActive = true;
    static const char *TAG = "measureStart";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    queueToPc = xQueueCreate(1000, sizeof(DataElement));
    if (queueToPc == NULL)
    {
        // Handle error
        ESP_LOGE("Queue", "Failed to create queue");
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    }
    // Create the data transmission task
    xTaskCreatePinnedToCore(dataTransmissionLoop, "dataTransmissionTask", 10000, NULL, 1, NULL, 0);

    xTaskCreatePinnedToCore(measureLoop, "measurementLoop", 10000, NULL, 2, &handleMeasureLoop, 1);
    timer_initialize();
}
