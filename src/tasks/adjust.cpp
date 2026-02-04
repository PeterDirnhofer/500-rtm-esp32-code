#include "UsbPcInterface.h"
#include "helper_functions.h"
#include "project_timer.h"
#include "tasks.h"
#include "tasks_common.h"
#include <cstdlib>
#include <esp_log.h>
#include <freertos/queue.h>
#include <sstream>
#include <string>
#include <vector>

// TIP queue provided by dispatcher
extern QueueHandle_t tipQueue;

extern "C" void adjustLoop(void *unused) {
  static const char *TAG = "adjustLoop";
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  ESP_LOGI(TAG, "+++ START ADJUST\n");

  while (adjustIsActive) {
    /* Check for TIP updates forwarded by the dispatcher. These allow a
     * remote client to adjust the DAC tip values while the adjust loop is
     * running without calling into USB layer parsing directly. */
    if (tipQueue != NULL) {
      char tipbuf[256];
      if (xQueueReceive(tipQueue, tipbuf, 0) == pdPASS) {
        std::string s(tipbuf);
        // expect format TIP,x,y,z
        if (s.rfind("TIP,", 0) == 0) {
          std::string rest = s.substr(4);
          std::vector<std::string> tokens;
          std::stringstream ss(rest);
          std::string token;
          while (std::getline(ss, token, ','))
            tokens.push_back(token);
          if (tokens.size() == 3) {
            char *endPtr;
            long xl = strtol(tokens[0].c_str(), &endPtr, 10);
            if (*endPtr == '\0') {
              long yl = strtol(tokens[1].c_str(), &endPtr, 10);
              if (*endPtr == '\0') {
                long zl = strtol(tokens[2].c_str(), &endPtr, 10);
                if (*endPtr == '\0') {
                  // clamp values
                  if (xl < 0)
                    xl = 0;
                  if (xl > DAC_VALUE_MAX)
                    xl = DAC_VALUE_MAX;
                  if (yl < 0)
                    yl = 0;
                  if (yl > DAC_VALUE_MAX)
                    yl = DAC_VALUE_MAX;
                  if (zl < 0)
                    zl = 0;
                  if (zl > DAC_VALUE_MAX)
                    zl = DAC_VALUE_MAX;
                  currentXDac = (uint16_t)xl;
                  currentYDac = (uint16_t)yl;
                  currentZDac = (uint16_t)zl;
                  UsbPcInterface::send("TIP,OK\n");
                  // wake vspi loop to apply values
                  if (handleVspiLoop != NULL)
                    vTaskResume(handleVspiLoop);
                } else {
                  UsbPcInterface::send("ERROR: TIP Z not integer\n");
                }
              } else {
                UsbPcInterface::send("ERROR: TIP Y not integer\n");
              }
            } else {
              UsbPcInterface::send("ERROR: TIP X not integer\n");
            }
          } else {
            UsbPcInterface::send("ERROR: TIP needs 3 values\n");
          }
        }
      }
    }
    int16_t adcValue = readAdc();
    currentTunnelnA = calculateTunnelNa(adcValue);
    double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                       (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
    UsbPcInterface::send("ADJUST,%.3f,%.3f,%d\n", adcInVolt, currentTunnelnA,
                         adcValue);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  resetDac();
  // Free timer in case adjust used the shared timer
  timer_deinitialize();
  vTaskDelete(NULL);
}
