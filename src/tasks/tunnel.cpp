#include "UsbPcInterface.h"
#include "esp_heap_caps.h"
#include "helper_functions.h"
#include "project_timer.h"
#include "tasks.h"
#include "tasks_common.h"
#include <esp_log.h>

extern "C" void tunnelLoop(void *params) {
  uint16_t newDacZ = 0;
  resetDac();
  static const char *TAG = "tunnelLoop/main";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "tunnelLoop persistent task started, waiting for commands");

  while (1) {
    int maxLoops = 0;
    // Wait for a start command (maxLoops) from controller
    if (queueTunnelCmd == NULL) {
      // Queue not yet created; wait and retry
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }
    if (xQueueReceive(queueTunnelCmd, &maxLoops, portMAX_DELAY) != pdPASS) {
      continue;
    }

    // Start run
    int counter = 0;
    resetDac();
    tunnelIsActive = true;
    ESP_LOGI(TAG, "+++ Start Tunnel loop (maxLoops=%d)", maxLoops);

    while (tunnelIsActive && counter < maxLoops) {
      vTaskSuspend(NULL);
      int16_t adcValue = readAdc();

      ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
      int16_t errorAdc = targetTunnelAdc - adcValue;

      if (abs(errorAdc) <= toleranceTunnelAdc) {
        gpio_set_level(IO_17_DAC_NULL, 0);

        DataElement dataElement(LIMIT, adcValue, currentZDac);
        if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
          ESP_LOGE(TAG, "Failed to enqueue LIMIT into queueToPc");
        }
      } else {
        gpio_set_level(IO_17_DAC_NULL, 1);

        DataElement dataElement(OFF_LIMITS, adcValue, currentZDac);
        if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
          ESP_LOGE(TAG, "Failed to enqueue OFF_LIMITS into queueToPc");
        } else {
          ESP_LOGD(TAG, "Enqueued OFF_LIMITS for adc=%d Z=%d", adcValue,
                   currentZDac);
        }

        newDacZ = computePiDac(adcValue, targetTunnelAdc);
        currentZDac = newDacZ;
        vTaskResume(handleVspiLoop);
      }

      counter++;

      if (counter >= maxLoops) {
        tunnelIsActive = false;
      }
    }
    ESP_LOGI(TAG, "tunnelLoop finished, sending DATA_COMPLETE");
    DataElement dataElement(DATA_COMPLETE, 0, 0);
    if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
      ESP_LOGE(TAG, "Failed to enqueue DATA_COMPLETE into queueToPc");
    } else {
      ESP_LOGI(TAG, "Enqueued DATA_COMPLETE");
    }
    while (uxQueueMessagesWaiting(queueToPc) > 0) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(1));
    ESP_LOGI(TAG, "tunnelLoop finished for this run\n");
    tunnelIsActive = false;
    // Free timer resource used by tunnel/measure
    timer_deinitialize();
    // Log heap after run cleanup for diagnostics
    {
      size_t free_after = heap_caps_get_free_size(MALLOC_CAP_8BIT);
      ESP_LOGI(TAG, "Free heap at tunnelLoop run exit: %u", free_after);
    }
    // Loop back and wait for next command without deleting the task
  }
}
