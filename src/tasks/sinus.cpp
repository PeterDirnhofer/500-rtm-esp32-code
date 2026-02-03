#include "UsbPcInterface.h"
#include "globalVariables.h"
#include "helper_functions.h"
#include "tasks.h"
#include "tasks_common.h"
#include <cmath>
#include <esp_log.h>
#include <memory>

extern "C" void sinusLoop(void *params) {
  static const char *TAG = "testLoop";
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  ESP_LOGI(TAG, "+++ START TEST LOOP");

  std::unique_ptr<uint16_t[]> buffer(new uint16_t[BUFFER_SIZE]);

  for (size_t i = 0; i < BUFFER_SIZE; ++i) {
    buffer[i] = static_cast<uint16_t>((DAC_VALUE_MAX / 2) *
                                      (1 + sin(2.0 * M_PI * i / BUFFER_SIZE)));
  }

  while (sinusIsActive) {
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
      if (!sinusIsActive)
        break;
      vspiSendDac(buffer[i], buffer.get(), handleDacX);
      vspiSendDac(buffer[i], buffer.get(), handleDacY);
      vspiSendDac(buffer[i], buffer.get(), handleDacZ);
      vTaskDelay(pdMS_TO_TICKS(1));
    }
  }
  resetDac();
  ESP_LOGI(TAG, "stop sinus loop");
  vTaskDelete(NULL);
}
