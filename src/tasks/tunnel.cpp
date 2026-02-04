#include "UsbPcInterface.h"
#include "helper_functions.h"
#include "project_timer.h"
#include "tasks.h"
#include "tasks_common.h"
#include <esp_log.h>

static inline bool enqueueData(const DataElement &el) {
  if (xQueueSend(queueToPc, &el, portMAX_DELAY) != pdPASS) {
    ESP_LOGE("tunnelLoop/main", "Failed to enqueue data element");
    return false;
  }
  return true;
}

static inline void handleOffLimitsAndResume(int16_t adcValue) {
  uint16_t newDacZ = computePiDac(adcValue, targetTunnelAdc);
  currentZDac = newDacZ;
  // Resume VSPI worker to apply new Z value
  if (handleVspiLoop != NULL) {
    vTaskResume(handleVspiLoop);
  }
}

extern "C" void tunnelLoop(void *params) {
  uint16_t newDacZ = 0;
  resetDac();
  static const char *TAG = "tunnelLoop/main";
  esp_log_level_set(TAG, ESP_LOG_INFO);

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

    int counter = 0;
    resetDac();
    tunnelIsActive = true;
    ESP_LOGI(TAG, "+++ Start TUNNEL (maxLoops=%d)", maxLoops);

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
    }
    while (uxQueueMessagesWaiting(queueToPc) > 0) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDelay(pdMS_TO_TICKS(1));

    tunnelIsActive = false;
    // Free timer resource used by tunnel/measure
    timer_deinitialize();
    // Run cleanup complete
    // Loop back and wait for next command without deleting the task
  }
}
