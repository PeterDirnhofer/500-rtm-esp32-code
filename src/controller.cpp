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

static const char *TAG = "controller";

// Declare the queue handle
extern QueueHandle_t queueFromPc;

extern "C" void commandDispatcherTask(void *unused)
{

    static const char *TAG = "dispatcherTask";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "++ STARTED");
    std::string rcvString;

    while (1)
    {
        // Wait for data to be available in the queue with a timeout of 100 ms
        if (xQueueReceive(queueFromPc, &rcvString, pdMS_TO_TICKS(100)) == pdPASS)
        {
            if (rcvString == "STOP")
            {
                adjustIsActive = false;
                measureIsActive = false;
                ESP_LOGI(TAG, "adjustIsActive set to false");
                continue;
            }

            if (rcvString == "MEASURE")
            {
                measureStart();
                continue;
            }

            if (rcvString == "ADJUST")
            {
                adjustStart();
                continue;
            }

            if (adjustIsActive && rcvString.find("TIP") != std::string::npos)
            {
                UsbPcInterface::mUpdateTip(rcvString);
                continue;
            }
            if (!adjustIsActive && rcvString.find("TIP") != std::string::npos)
            {
                ESP_LOGI(TAG, "ERROR: Received TIP command while adjust is not active");
                UsbPcInterface::send("Received TIP command while adjust is not active");
                continue;
            }

            if (rcvString == "PARAMETER,?")
            {
                ESP_LOGI(TAG, "PARAMETER,? detected");
                ParameterSetting parameterSetter;
                parameterSetter.getParametersFromFlash();
                std::string storedParameters = parameterSetter.getParameters();
                ESP_LOGI(TAG, "Stored Parameters: %s", storedParameters.c_str());
                std::istringstream stream(storedParameters);
                std::string line;
                while (std::getline(stream, line))
                {
                    line += "\n";
                    UsbPcInterface::send(line.c_str());
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
                continue;
            }
        }
        else
        {
            // No data received, continue to the next iteration
            continue;
        }
    }
}

extern "C" void dispatcherTaskStart()
{
    // Initialize the queue if it hasn't been initialized
    if (queueFromPc == NULL)
    {
        queueFromPc = xQueueCreate(10, sizeof(std::string)); // Adjust the size and length as needed
    }

    // Create the dispatcher task
    xTaskCreatePinnedToCore(commandDispatcherTask, "dispatcherTask", 10000, NULL, 4, NULL, 0);
}

extern "C" esp_err_t initAdcDac()
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

    xTaskCreatePinnedToCore(measureLoop, "measurementLoop", 10000, NULL, 2, &handleControllerLoop, 1);
    timer_initialize(MODE_MEASURE);
}
