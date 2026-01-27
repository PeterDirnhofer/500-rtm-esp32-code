#include "UsbPcInterface.h"
#include "globalVariables.h"
#include "loops.h"
#include "loops_common.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

// Shared constants
const int LIMIT = 1;
const int OFF_LIMITS = 0;
const int DATA_COMPLETE = 0xFFFF;
const size_t BUFFER_SIZE = 1024;

// Shared state
bool isLoopExecution = false;
bool tunnel_found = false;
const char *prefix = "";

extern "C" void setPrefix(const char *_prefix) {
  if (_prefix != nullptr) {
    prefix = _prefix;
  }
}

extern "C" void queueToPcClear() {
  DataElement element;
  while (xQueueReceive(queueToPc, &element, 0) == pdPASS) {
    // clear
  }
}

extern "C" void resetDac() {
  currentXDac = 0;
  currentYDac = 0;
  currentZDac = DAC_VALUE_MAX / 2;
  vTaskResume(handleVspiLoop);
}
