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
    parameterSetter.getParametersFromFlash(false);
    

    while (true)
    {

        ESP_LOGI(TAG, "TICK1 main loop. Brauchts das?");
        vTaskDelay(pdMS_TO_TICKS(10000));
        continue;

        // ESP_LOGI(TAG, "Current working mode: %d", usb.getWorkingMode());
        // continue;

        // if (usb.getCommandsFromPC() != ESP_OK)
        // {

        //     ESP_LOGE(TAG, "Invalid command. \n"
        //                   "'PARAMETER,?' or 'PARAMETER,DEFAULT' or 'PARAMETER,kI,kP,...'\n");
        //     UsbPcInterface::printErrorMessageAndRestart("Invalid command. \n"
        //                                                 "'PARAMETER,?' or 'PARAMETER,DEFAULT' or 'PARAMETER,kI,kP,...'\n");

        //     vTaskDelay(pdMS_TO_TICKS(1000));
        //     esp_restart();
        // }
        // ESP_LOGI(TAG, "AAAAA getWorkingMode %d", usb.getWorkingMode());

        // if (usb.getWorkingMode() == MODE_RESTART)
        // {
        //     ESP_LOGI(TAG, "esp_restart MODE_RESTART");
        //     esp_restart();
        // }

        // // Process parameters from PC
        // std::string p0 = "", p1 = "";
        // int parameterCount;

        // // Get the first parameter
        // p0 = usb.getParametersFromPc()[0];

        // // Get the total number of parameters
        // parameterCount = usb.getParametersFromPc().size();

        // // If there are two parameters, get the second one
        // if (parameterCount == 2)
        // {
        //     p1 = usb.getParametersFromPc()[1];
        // }
        // ESP_LOGI(TAG, "Parameter p0: %s", p0.c_str());
        // ESP_LOGI(TAG, "Parameter p1: %s", p1.c_str());

        // // Handle different working modes
        // if (usb.getWorkingMode() == MODE_ADJUST_TEST_TIP)
        // {
        //     adjustStart();
        //     vTaskDelete(NULL);
        // }
        // else if (usb.getWorkingMode() == MODE_MEASURE)
        // {
        //     parameterSetter.getParametersFromFlash(false); // Get measurement parameters from NVS
        //     measureStart();
        //     vTaskDelete(NULL);
        // }
        // else if (usb.getWorkingMode() != MODE_PARAMETER)
        // {
        //     UsbPcInterface::printErrorMessageAndRestart("INVALID Command MODE");
        // }

        // // PARAMETER,?
        // if (p1.compare("?") == 0)
        // {

        //     ESP_LOGI(TAG, "AAAAAAA getParametersFromFlash");
        //     parameterSetter.getParametersFromFlash(false);
        //     ESP_LOGI(TAG, "Stored Parameters:");
        //     string storedParameters = parameterSetter.getParameters();
        //     ESP_LOGI(TAG, "%s", storedParameters.c_str());

        //     ESP_LOGI(TAG, "AAAAAAA start acknowledge");
        //     vTaskDelay(pdMS_TO_TICKS(200));

        //     continue;
        //     // esp_restart();
        //     // UsbPcInterface::printErrorMessageAndRestart("");
        // }
        // // PARAMETER,DEFAULT
        // else if (p1.compare("DEFAULT") == 0)
        // {
        //     esp_err_t err = parameterSetter.putDefaultParametersToFlash();
        //     if (err == ESP_OK)
        //     {
        //         parameterSetter.getParametersFromFlash(false);
        //         esp_restart();
        //     }
        //     else
        //     {
        //         UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,float,float,...");
        //     }
        // }
        // // TODO = size_keys + 1
        // else if (parameterCount == 13)
        // {
        //     esp_err_t err = parameterSetter.putParametersToFlash(usb.getParametersFromPc());
        //     if (err == ESP_OK)
        //     {
        //         parameterSetter.getParametersFromFlash(true);
        //         esp_restart();
        //     }
        //     else
        //     {
        //         UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,p1,p2,...,p9");
        //     }
        // }
        // else
        // {
        //     UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nInvalid number of parameters");
        // }

        // ESP_LOGI(TAG, "--- delete main task\n");
        // vTaskDelete(NULL);
    }
}
