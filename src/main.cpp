#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/igmp.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <esp_event.h>
#include <soc/rtc_periph.h>
#include <driver/spi_slave.h>
#include <esp_log.h>
#include <spi_flash_mmap.h>
#include <driver/gpio.h>
#include <nvs_flash.h>
#include <nvs.h>

#include "globalVariables.h"
#include "dac_spi.h"
#include "adc_i2c.h"
#include "controller.h"
#include "dataStoring.h"
#include "UsbPcInterface.h"
#include "ParameterSetter.h"
#include "helper_functions.h"

extern "C" void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("efuse", ESP_LOG_WARN);     // Suppress efuse logs
    esp_log_level_set("heap_init", ESP_LOG_WARN); // Suppress heap logs

    static const char *TAG = "app_main";
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "++++++++++++++++ MAIN STARTED +++++++++++++++");

    // GPIO configuration for Monitoring on Jumper J3 GPIO_RESERVE
    gpio_set_direction(IO_17_DAC_NULL, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_04_DAC_MAX, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_25_RED, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_27_YELLOW, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_02_GREEN, GPIO_MODE_OUTPUT);

    // Initialize GPIO levels
    gpio_set_level(IO_17_DAC_NULL, 0); // white LED
    gpio_set_level(IO_04_DAC_MAX, 0);  // blue LED
    gpio_set_level(IO_25_RED, 0);      // red LED
    gpio_set_level(IO_27_YELLOW, 0);   // yellow LED
    gpio_set_level(IO_02_GREEN, 0);    // green LED

    // USB Interface initialization
    UsbPcInterface usb;
    usb.start();

    // initAdc();
    i2cAdcInit();
    vspiDacStart();

    // Parameter setting
    ParameterSetting parameterSetter;
    // Start read from PC and Start Dispatcher
    dispatcherTaskStart();

    // Send "IDLE" message
    usb.send("IDLE\n");

    while (true)
    {
        if (!measureIsActive)
        {
            // Read voltage from preamplifier
            int16_t adcValue = readAdc();
            ledStatusAdc(adcValue, targetTunnelAdc, toleranceTunnelAdc, currentZDac);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
