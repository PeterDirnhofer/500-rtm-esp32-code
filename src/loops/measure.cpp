#include "UsbPcInterface.h"
#include "helper_functions.h"
#include "loops.h"
#include "loops_common.h"
#include "project_timer.h"
#include <esp_log.h>
#include <string>


extern "C" void measureLoop(void *unused) {
  static const char *TAG = "measureLoop";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "+++ STARTED");

  uint16_t newDacZ = 0;
  std::string dataBuffer;

  setPrefix("FIND");

  while (measureIsActive) {
    isLoopExecution = false;
    vTaskSuspend(NULL);

    if (isLoopExecution) {
      ESP_LOGE(TAG,
               "ERROR: measureLoop retriggered before last run was finished");
      errorBlink();
    }
    isLoopExecution = true;

    int16_t adcValue = readAdc();
    ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
    int16_t errorAdc = targetTunnelAdc - adcValue;

    if (abs(errorAdc) <= toleranceTunnelAdc) {
      if (!tunnel_found) {
        tunnel_found = true;
        queueToPcClear();
        setPrefix("DATA");
      }

      DataElement dataElement(rtmGrid.getCurrentX(), rtmGrid.getCurrentY(),
                              currentZDac);
      if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("Queue", "Failed to send to queue");
      }

      if (!rtmGrid.moveOn()) {
        vTaskResume(handleVspiLoop);
      } else {
        DataElement endSignal(DATA_COMPLETE, 0, 0);
        if (xQueueSend(queueToPc, &endSignal, portMAX_DELAY) != pdPASS) {
          ESP_LOGE("Queue", "Failed to send to queue");
        }

        timer_stop();
        while (uxQueueMessagesWaiting(queueToPc) > 0) {
          vTaskDelay(pdMS_TO_TICKS(10));
        }

        esp_restart();
      }
    } else {
      newDacZ = computePiDac(adcValue, targetTunnelAdc);
      currentZDac = newDacZ;

      if (!tunnel_found) {
        DataElement dataElement(targetTunnelAdc, adcValue, currentZDac);
        if (xQueueSend(queueToPc, &dataElement, portMAX_DELAY) != pdPASS) {
          ESP_LOGE("Queue", "Failed to send to queue");
        }
      }

      vTaskResume(handleVspiLoop);
    }
  }
  vTaskDelete(NULL);
}
