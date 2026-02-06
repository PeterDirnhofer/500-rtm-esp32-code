#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "nvs_helpers.h"
#include <driver/gpio.h>
#include <driver/spi_slave.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <lwip/dns.h>
#include <lwip/igmp.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <soc/rtc_periph.h>
#include <spi_flash_mmap.h>

#include "ParameterSetter.h"
#include "UsbPcInterface.h"
#include "WifiPcInterface.h"
#include "adc_i2c.h"
#include "controller.h"
#include "dac_spi.h"
#include "dataStoring.h"
#include "globalVariables.h"
#include "helper_functions.h"
// Do not use compile-time credentials

// Background task to start WiFi so app_main isn't blocked
static void wifi_init_task(void *pvParameters) {

  // Delegate credential selection to WifiPcInterface (it will read NVS)
  WifiPcInterface::startStation();
  vTaskDelete(NULL);
}

extern "C" void app_main(void) {
  esp_log_level_set("*", ESP_LOG_WARN);
  esp_log_level_set("efuse", ESP_LOG_WARN);     // Suppress efuse logs
  esp_log_level_set("heap_init", ESP_LOG_WARN); // Suppress heap logs

  static const char *TAG = "app_main";
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "++++++++++++++++ MAIN STARTED +++++++++++++++");

  // Initialize NVS — required by WiFi
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    ESP_LOGW(TAG, "NVS init: erasing NVS partition and retrying");
    nvs_flash_erase();
    ret = nvs_flash_init();
  }
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize NVS (0x%08x)", ret);
  } else {
    ESP_LOGI(TAG, "NVS initialized");
  }

  // GPIO configuration for Monitoring on Jumper J3 GPIO_RESERVE
  gpio_set_direction(LED_RED, GPIO_MODE_OUTPUT);    // LED1
  gpio_set_direction(LED_YELLOW, GPIO_MODE_OUTPUT); // LED2
  gpio_set_direction(LED_GREEN, GPIO_MODE_OUTPUT);  // LED3
  gpio_set_direction(LED_4, GPIO_MODE_OUTPUT);      // LED4
  gpio_set_direction(LED_5, GPIO_MODE_OUTPUT);      // LED5

  // USB Interface initialization
  UsbPcInterface usb;
  usb.start();

  // Start WiFi interface in STA mode using credentials from my_data.h
  // Run WiFi start in a background FreeRTOS task so other init isn't blocked
  xTaskCreate(wifi_init_task, "wifi_init", 4096, NULL, tskIDLE_PRIORITY + 1,
              NULL);

  gpio_set_level(LED_RED, 1);
  // initAdc();
  i2cAdcInit();
  gpio_set_level(LED_YELLOW, 1);

  vspiDacStart();
  // Turn on the green LED to indicate system is ready
  gpio_set_level(LED_GREEN, 1);

  // Parameter settings
  ParameterSetting parameterSetter;

  // Start read from PC and Start Dispatcher
  dispatcherTaskStart();

  vTaskDelay(pdMS_TO_TICKS(100));

  // Send "STOP" message
  usb.send("STOP\n");

  while (true) {
    if (!measureIsActive) {
      // Read voltage from preamplifier
      int16_t adcValue = readAdc();
      ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
