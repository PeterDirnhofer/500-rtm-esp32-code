// controller.cpp

#include "controller.h"
#include "project_timer.h"
#include "globalVariables.h"
#include "UsbPcInterface.h"
#include "esp_log.h"
#include "loops.h"
#include "helper_functions.h"

static const char *TAG = "controller";

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
    xTaskCreatePinnedToCore(adjustLoop, "adjustLoop", 10000, NULL, 2, &handleAdjustLoop, 1);
    timer_initialize(MODE_ADJUST_TEST_TIP);
}

extern "C" void measureStart()
{
    static const char *TAG = "measureStart";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    queueRtos = xQueueCreate(1000, sizeof(DataElement)); if (queueRtos == NULL) {
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

