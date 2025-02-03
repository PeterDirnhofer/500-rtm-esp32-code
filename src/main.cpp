#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/igmp.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "soc/rtc_periph.h"
#include "driver/spi_slave.h"
#include "esp_log.h"
#include "spi_flash_mmap.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "globalVariables.h"
#include "dac_spi.h"
#include "adc_i2c.h"
#include "controller.h"
#include "dataStoring.h"
#include "UsbPcInterface.h"
#include "ParameterSetter.h"
#include "esp_log.h"
#include <esp_log.h>
#include "helper_functions.h"

using namespace std;

extern "C" void app_main(void)
{
    static const char *TAG = "app_main";
    esp_log_level_set("*", ESP_LOG_NONE);
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGI(TAG, "+++++++++++++++++ STARTED");

    // GPIO configuration for Monitoring on Jumper J3 GPIO_RESERVE
    gpio_set_direction(IO_17, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_04, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_25, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_27, GPIO_MODE_OUTPUT);
    gpio_set_direction(IO_02, GPIO_MODE_OUTPUT);

    // Initialize GPIO levels
    gpio_set_level(IO_17, 0); // white LED
    gpio_set_level(IO_04, 0); // blue LED
    gpio_set_level(IO_25, 0); // red LED
    gpio_set_level(IO_27, 0); // yellow LED
    gpio_set_level(IO_02, 0); // green LED

    // USB Interface initialization
    UsbPcInterface usb;
    usb.start();
    // Start read from PC and Start Dispatcher
    dispatcherTaskStart();

    initAdcDac();

    // Parameter setting
    ParameterSetting parameterSetter;
    parameterSetter.getParametersFromFlash();
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
