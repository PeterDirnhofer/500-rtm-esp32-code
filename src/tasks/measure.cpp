#include "UsbPcInterface.h"
#include "helper_functions.h"
#include "project_timer.h"
#include "tasks.h"
#include "tasks_common.h"
#include <esp_log.h>
#include <string>

extern "C" void measureLoop(void *unused) {
  static const char *TAG = "measureLoop";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "+++ STARTED");

  uint16_t newDacZ = 0;
  static uint32_t reportCounter =
      0; // counts measurements for throttled reporting

  setPrefix("FIND");

  while (measureIsActive) {
    // Wait until resumed by controller/dispatcher.
    vTaskSuspend(NULL);

    // Protect against overlapping runs: if still set, skip this trigger.
    if (isLoopExecution) {
      ESP_LOGE(TAG,
               "ERROR: measureLoop retriggered before last run was finished");
      errorBlink();
      continue;
    }
    isLoopExecution = true; // mark this iteration as running

    // Read ADC and compute error relative to target.
    int16_t adcValue = readAdc();
    ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
    int16_t errorAdc = targetTunnelAdc - adcValue;

    // Case: within tolerance -> record measurement and advance grid.
    if (abs(errorAdc) <= toleranceTunnelAdc) {
      if (!tunnel_found) {
        tunnel_found = true;
        queueToPcClear();
        setPrefix("DATA");
      }

      // measurement inside tolerance
      const DataElement measurement{rtmGrid.getCurrentX(),
                                    rtmGrid.getCurrentY(), currentZDac};
      if (xQueueSend(queueToPc, &measurement, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("Queue", "Failed to send to queue");
      }

      // If there are more grid points, resume worker to move z/x/y.
      if (!rtmGrid.moveOn()) {
        isLoopExecution = false;
        vTaskResume(handleVspiLoop);
        continue;
      }

      // Grid finished: send end marker, stop timer and restart the board.
      DataElement endSignal(DATA_COMPLETE, 0, 0);
      if (xQueueSend(queueToPc, &endSignal, portMAX_DELAY) != pdPASS) {
        ESP_LOGE("Queue", "Failed to send to queue");
      }

      timer_stop();
      while (uxQueueMessagesWaiting(queueToPc) > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
      }

      ESP_LOGI(TAG, "measureLoop finished, exiting task");
      // Mark loop inactive and clear execution flag, then delete this task
      measureIsActive = false;
      isLoopExecution = false;
      vTaskDelete(NULL);
    }

    // not within tolerance -> compute correction, report and resume
    newDacZ = computePiDac(adcValue, targetTunnelAdc);
    currentZDac = newDacZ;

    if (!tunnel_found) {
      // In simulation mode report every measurement; otherwise throttle to
      // every 100.
      ++reportCounter;
      const bool shouldReport =
          SIMULATION_MODE ? true : (reportCounter % 100 == 0);
      if (shouldReport) {
        const DataElement report{targetTunnelAdc,
                                 static_cast<uint16_t>(adcValue), currentZDac};
        if (xQueueSend(queueToPc, &report, portMAX_DELAY) != pdPASS) {
          ESP_LOGE("Queue", "Failed to send to queue");
        }
      }
    }

    isLoopExecution = false; // finished this iteration before resuming worker
    vTaskResume(handleVspiLoop);
  }

  // Ensure flag is cleared before deleting the task.
  isLoopExecution = false;
  vTaskDelete(NULL);
}
