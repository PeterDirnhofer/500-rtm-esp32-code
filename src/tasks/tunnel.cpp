#include "UsbPcInterface.h"
#include "helper_functions.h"
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
    ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
    int16_t errorAdc = targetTunnelAdc - adcValue;

    if (abs(errorAdc) <= toleranceTunnelAdc) {
      gpio_set_level(IO_17_DAC_NULL, 0);
      DataElement dataElement(LIMIT, adcValue, currentZDac);
      if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("Queue", "Failed to send to queue");
      }
    } else {
      gpio_set_level(IO_17_DAC_NULL, 1);
      DataElement dataElement(OFF_LIMITS, adcValue, currentZDac);
      if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("Queue", "Failed to send to queue");
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

  DataElement dataElement(DATA_COMPLETE, 0, 0);
  if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
    ESP_LOGE("Queue", "Failed to send to queue");
  }
  while (uxQueueMessagesWaiting(queueToPc) > 0) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelay(pdMS_TO_TICKS(1));
  esp_restart();
}
