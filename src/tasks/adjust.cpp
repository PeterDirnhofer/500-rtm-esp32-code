#include "UsbPcInterface.h"
#include "helper_functions.h"
#include "tasks.h"
#include "tasks_common.h"
#include <esp_log.h>

extern "C" void adjustLoop(void *unused) {
  static const char *TAG = "adjustLoop";
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  ESP_LOGI(TAG, "+++ START ADJUST\n");

  while (adjustIsActive) {
    int16_t adcValue = readAdc();
    currentTunnelnA = calculateTunnelNa(adcValue);
    double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                       (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
    UsbPcInterface::send("ADJUST,%.3f,%.3f,%d\n", adcInVolt, currentTunnelnA,
                         adcValue);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  resetDac();
  vTaskDelete(NULL);
}
