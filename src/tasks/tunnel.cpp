#include "UsbPcInterface.h"
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

  int *loopsPtr = static_cast<int *>(params);
  if (loopsPtr == nullptr) {
    ESP_LOGE(TAG, "Invalid parameter pointer");
    vTaskDelete(NULL);
    return;
  }
  int maxLoops = *loopsPtr;

  int counter = 0;

  while (tunnelIsActive && counter < maxLoops) {
    vTaskSuspend(NULL);
    int16_t adcValue = readAdc();
    ESP_LOGI(TAG, "woke: counter=%d/%d, adc=%d, Z=%d", counter, maxLoops,
             adcValue, currentZDac);
    ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
    int16_t errorAdc = targetTunnelAdc - adcValue;

    if (abs(errorAdc) <= toleranceTunnelAdc) {
      gpio_set_level(IO_17_DAC_NULL, 0);
      ESP_LOGI(TAG, "Within tolerance: adc=%d, Z=%d -> enqueue LIMIT", adcValue,
               currentZDac);
      DataElement dataElement(LIMIT, adcValue, currentZDac);
      if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
        ESP_LOGE(TAG, "Failed to enqueue LIMIT into queueToPc");
      } else {
        ESP_LOGD(TAG, "Enqueued LIMIT for X=%d Z=%d", adcValue, currentZDac);
      }
    } else {
      gpio_set_level(IO_17_DAC_NULL, 1);
      ESP_LOGI(TAG, "Off limits: adc=%d, Z=%d -> enqueue OFF_LIMITS", adcValue,
               currentZDac);
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
    ESP_LOGD(TAG, "Enqueued DATA_COMPLETE");
  }
  while (uxQueueMessagesWaiting(queueToPc) > 0) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelay(pdMS_TO_TICKS(1));
  ESP_LOGI(TAG, "tunnelLoop finished, exiting task");
  tunnelIsActive = false;
  // Free timer resource used by tunnel/measure
  timer_deinitialize();
  vTaskDelete(NULL);
}
