// controller.cpp

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "ParameterSetter.h"
#include "UsbPcInterface.h"
#include "controller.h"
#include "esp_heap_caps.h"
#include "globalVariables.h"
#include "helper_functions.h"
#include "project_timer.h"
#include "tasks.h"
#include "tasks_common.h"


static const char *TAG = "controller";

// Queue for TIP commands forwarded to the running adjust loop
QueueHandle_t tipQueue = NULL;

extern "C" void dispatcherTask(void *unused) {
  static const char *TAG = "dispatcherTask";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "STARTED");
  char rcvChars[255];

  if (queueFromPc == NULL) {
    ESP_LOGE(TAG, "queueFromPc not initialized");
    vTaskDelete(NULL); // Delete the task if queue is not initialized
  }
  while (1) {
    std::string receive; // Declare receive outside the if-block
    // Wait for data to be available in the queue with a timeout of 100 ms
    if (xQueueReceive(queueFromPc, &rcvChars, pdMS_TO_TICKS(100)) == pdPASS) {
      receive = std::string(rcvChars);
      // Remove extra characters (whitespace, newline, etc.)
      receive.erase(std::remove_if(receive.begin(), receive.end(), ::isspace),
                    receive.end());

      // Only proceed if we received a complete string (not empty)
      if (receive.empty()) {

        continue; // Skip processing and check queue again
      }

      ESP_LOGI(TAG, "Processing command: %s", receive.c_str());

      if (receive == "STOP") {

        // Stop running loops so the system becomes idle but keep the
        // receive (UART/WebSocket) loop alive to accept further commands.
        adjustIsActive = false;

        // Stop measure loop and wake it so it can exit cleanly if suspended
        measureIsActive = false;
        if (handleMeasureLoop != NULL) {
          vTaskResume(handleMeasureLoop);
        }

        // Stop any running tunnel loop and wake it
        tunnelIsActive = false;
        if (handleTunnelLoop != NULL) {
          vTaskResume(handleTunnelLoop);
        }

        // Stop sinus (test) loop
        sinusIsActive = false;

        // Stop data transmission loop: send DATA_COMPLETE to unblock it
        dataTransmissionIsActive = false;
        if (queueToPc != NULL) {
          DataElement endSignal(DATA_COMPLETE, 0, 0);
          xQueueSend(queueToPc, &endSignal, pdMS_TO_TICKS(50));
        }

        // Ensure timer is stopped and freed when stopping measurement/tunnel
        timer_deinitialize();

        // Notify controller/clients that loops were stopped
        UsbPcInterface::send("STOPPED\n");
        ESP_LOGI(TAG, "Stopped");

        continue;
      }

      if (receive.rfind("MEASURE", 0) == 0) {

        // Set simulation mode when command contains SIMULATE
        SIMULATION_MODE = (receive.find("SIMULATE") != std::string::npos);

        measureStart();
        continue;
      }

      if (receive.rfind("TUNNEL", 0) == 0) {
        // Set simulation mode when command contains SIMULATE
        SIMULATION_MODE = (receive.find("SIMULATE") != std::string::npos);

        // Parse the number of loops from the command
        std::string loops_str = "1000"; // Default value

        // Look for comma to extract loops parameter
        size_t commaPos = receive.find(',');
        if (commaPos != std::string::npos) {
          // Use string after the comma as loops_str
          loops_str = receive.substr(commaPos + 1);
          ESP_LOGI(TAG, "Using loops from comma: %s", loops_str.c_str());
        }
        tunnelStart(loops_str);
        continue;
      }

      // Adjust and TIP
      if (receive == "ADJUST") {
        adjustStart();
        continue;
      }

      // Start testLoop with parameter X, Y, Z
      if (receive == "SINUS") {
        // Create the testLoop task
        if (handleSinusLoop != NULL) {
          vTaskDelete(handleSinusLoop);
          handleSinusLoop = NULL;
        }
        sinusStart();
        continue;
      }

      if (receive.rfind("TIP,", 0) == 0) {
        if (adjustIsActive) {
          if (tipQueue == NULL) {
            tipQueue = xQueueCreate(8, 256);
            if (tipQueue == NULL) {
              ESP_LOGW(TAG, "Failed to create tipQueue");
              UsbPcInterface::send("ERROR: TIP handling unavailable\n");
              continue;
            }
          }
          BaseType_t q =
              xQueueSend(tipQueue, receive.c_str(), pdMS_TO_TICKS(50));
          if (q != pdPASS) {
            ESP_LOGW(TAG, "TIP queue full — dropping TIP command");
            UsbPcInterface::send("ERROR: TIP queue full\n");
          } else {
            ESP_LOGI(TAG, "TIP enqueued for adjust loop");
          }
          continue;
        } else {
          ESP_LOGW(TAG,
                   "ERROR: Received TIP command while adjust is not active");
          UsbPcInterface::send(
              "Received TIP command while adjust is not active\n");
          continue;
        }
      }

      // Parameter
      if (receive == "PARAMETER,?") {
        ParameterSetting parameterSetter;
        parameterSetter.getParametersFromFlash();
        std::string storedParameters = parameterSetter.getParameters();
        std::istringstream stream(storedParameters);
        std::string line;
        while (std::getline(stream, line)) {
          line = "PARAMETER," + line;
          line += "\n";
          UsbPcInterface::send(line.c_str());
          vTaskDelay(pdMS_TO_TICKS(10));
        }
        continue;
      }

      if (receive == "PARAMETER,DEFAULT") {
        ESP_LOGI(TAG, "parameter,default ++++");
        ParameterSetting parameterSetter;
        parameterSetter.putDefaultParametersToFlash();
        // putDefaultParametersToFlash
        // PARAMETER,0.100000,0.010000,0.001000,1.000000,0.300000,0,0,1,0,199,199,100
        parameterSetter.getParametersFromFlash();
        continue;
      }

      if (receive.rfind("PARAMETER,", 0) == 0) {
        ParameterSetting parameterSetter;
        parameterSetter.putParametersToFlashFromString(receive);
        parameterSetter.getParametersFromFlash();
        continue;
      }

      // If none of the commands matched, send an error message
      std::string errorMsg = "ERROR: Unknown command: " + receive + "\n";
      UsbPcInterface::send(errorMsg.c_str());
    }
    // If no data received (timeout), just continue the loop without error
    // message
  }
}

extern "C" void dispatcherTaskStart() {
  // // Initialize the queue if it hasn't been initialized
  // // TODO
  esp_log_level_set(TAG, ESP_LOG_INFO);

  // Create the dispatcher task
  xTaskCreatePinnedToCore(dispatcherTask, "dispatcherTask", 10000, NULL, 4,
                          NULL, 0);
}

extern "C" void adjustStart() {
  if (!adjustIsActive) {
    adjustIsActive = true;
    xTaskCreatePinnedToCore(adjustLoop, "adjustLoop", 10000, NULL, 2,
                            &handleAdjustLoop, 1);
  }
}

extern "C" void sinusStart() {
  static const char *TAG = "sinusStart";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "sinusStart initiated");
  if (!sinusIsActive) {
    sinusIsActive = true;
    xTaskCreatePinnedToCore(sinusLoop, "sinusLoop", 10000, NULL, 2,
                            &handleSinusLoop, 1);
  }
}

extern "C" void measureStart() {
  measureIsActive = true;
  static const char *TAG = "measureStart";
  esp_log_level_set(TAG, ESP_LOG_INFO);

  // Ensure grid current position is reset to configured start indices
  rtmGrid.setStartX(startX);
  rtmGrid.setStartY(startY);

  // Create queue only if the data transmission task is not yet running.
  // If the task already exists, reuse the existing queue and clear any
  // leftover messages so the new run starts with a clean queue.
  if (handleDataTransmissionLoop == NULL) {
    queueToPc = xQueueCreate(1000, sizeof(DataElement));
    if (queueToPc == NULL) {
      ESP_LOGE("Queue", "Failed to create queue");
      vTaskDelay(pdMS_TO_TICKS(100));
      esp_restart();
    }

    xTaskCreatePinnedToCore(dataTransmissionLoop, "dataTransmissionTask", 10000,
                            NULL, 1, &handleDataTransmissionLoop, 0);
  } else {
    // Clear existing queue contents before reusing it for this run.
    queueToPcClear();
  }

  setPrefix("DATA");
  xTaskCreatePinnedToCore(measureLoop, "measurementLoop", 10000, NULL, 2,
                          &handleMeasureLoop, 1);
  timer_initialize();
}

extern "C" void tunnelStart(const std::string &loops_str) {
  tunnelIsActive = true;
  static const char *TAG = "tunnelStart";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  // ESP_LOGI(TAG, "tunnelStart initiated with %s loops", loops_str.c_str());

  ESP_LOGI(TAG, "FOO1 started");

  // Create queue only if the data transmission task is not yet running.
  // If it already exists, reuse the existing queue and clear any pending
  // messages to avoid sending stale data.
  if (handleDataTransmissionLoop == NULL) {
    queueToPc = xQueueCreate(1000, sizeof(DataElement));

    if (queueToPc == NULL) {
      ESP_LOGE("Queue", "Failed to create queue");
      vTaskDelay(pdMS_TO_TICKS(100));
      esp_restart();
    }

    xTaskCreatePinnedToCore(dataTransmissionLoop, "dataTransmissionTask", 10000,
                            NULL, 1, &handleDataTransmissionLoop, 0);
  } else {
    queueToPcClear();
  }

  // Ensure prefix is set for the data transmission task so outputs are
  // formatted as TUNNEL.
  setPrefix("TUNNEL");
  ESP_LOGI(TAG, "FOO2 ");
  // Convert the string to an integer
  int maxLoops = 1000; // Default value
  if (!loops_str.empty() &&
      std::all_of(loops_str.begin(), loops_str.end(), ::isdigit)) {
    maxLoops = std::stoi(loops_str);
  } else {
    ESP_LOGE(TAG, "Invalid number of loops, using default value");
  }

  // Ensure the tunnel command queue exists
  if (queueTunnelCmd == NULL) {
    queueTunnelCmd = xQueueCreate(4, sizeof(int));
    if (queueTunnelCmd == NULL) {
      ESP_LOGE(TAG, "Failed to create queueTunnelCmd");
      tunnelIsActive = false;
      return;
    }
  }

  // Create the persistent tunnel task if it doesn't exist
  if (handleTunnelLoop == NULL) {
    BaseType_t createResult = xTaskCreatePinnedToCore(
        tunnelLoop, "tunnelLoop", 4096, NULL, 2, &handleTunnelLoop, 1);
    if (createResult != pdPASS) {
      ESP_LOGE(TAG, "Unable to create persistent tunnelLoop task");
      tunnelIsActive = false;
      return;
    }
  }

  // Send the requested loop count to the persistent task
  BaseType_t q = xQueueSend(queueTunnelCmd, &maxLoops, pdMS_TO_TICKS(50));
  if (q != pdPASS) {
    ESP_LOGW(TAG, "Failed to enqueue tunnel command");
    tunnelIsActive = false;
    return;
  }

  // Ensure the task is resumed so it can read the command
  if (handleTunnelLoop != NULL) {
    vTaskResume(handleTunnelLoop);
  }

  ESP_LOGI(TAG, "FOO3 timer_initialize");
  timer_initialize();
  ESP_LOGI(TAG, "FOO4 timer_initialize after");
}
