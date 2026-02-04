#include "UsbPcInterface.h"
#include "tasks.h"
#include "tasks_common.h"
#include <esp_log.h>
#include <sstream>

extern "C" void dataTransmissionLoop(void *params) {
  static const char *TAG = "dataTransmissionTask";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  // Keep the data transmission task persistent to avoid repeated
  // create/delete cycles which fragment the heap. The task will
  // continuously process queueToPc messages and send them out.
  dataTransmissionIsActive = true;

  while (1) {
    DataElement element;
    if (xQueueReceive(queueToPc, &element, portMAX_DELAY) == pdPASS) {
      uint16_t X = element.getDataX();
      uint16_t Y = element.getDataY();
      uint16_t Z = element.getDataZ();

      if (X == DATA_COMPLETE) {
        ESP_LOGI(TAG, "dataTransmission: DATA_COMPLETE received, sending DONE");
        UsbPcInterface::send("%s,DONE\n", prefix);
        // Do not delete the task; just continue waiting for next run
      } else {
        std::ostringstream oss;
        oss << prefix << "," << X << "," << Y << "," << Z << "\n";
        UsbPcInterface::send(oss.str().c_str());
        vTaskDelay(pdMS_TO_TICKS(1));
      }
    } else {
      ESP_LOGE(TAG, "Failed to receive from queue");
    }
  }
}
