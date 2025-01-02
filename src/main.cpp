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

using namespace std;

extern "C" void app_main(void)
{
    static const char *TAG = "app_main";

    // Set log level for the application
    esp_log_level_set("app_main", ESP_LOG_INFO);
    ESP_LOGI(TAG, "STARTED");

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
    // UsbPcInterface::send("IDLE\n");

    // Parameter setting
    ParameterSetting parameterSetter;
    UsbPcInterface::adjustIsActive = false;

    // Check for valid parameters in Flash; set defaults if invalid
    if (parameterSetter.parametersAreValid() != ESP_OK)
    {
        ESP_LOGW("TAG", "No valid Parameters found. Set to default ... ");

        esp_err_t result = parameterSetter.putDefaultParametersToFlash(); // Capture the result
        if (result != ESP_OK)
        {                                                                                            // Check if putting defaults was unsuccessful
            ESP_LOGE(TAG, "Failed to put default parameters to flash: %s", esp_err_to_name(result)); // Log error with message
        }
    }
    parameterSetter.getParametersFromFlash(false);

    initHardware();

    // ##############################################################
    // SELECT Run Mode
    // Wait for command from PC via USB
    if (usb.getCommandsFromPC() != ESP_OK)
    {
        UsbPcInterface::printErrorMessageAndRestart("Invalid command. \n"
                                                    "'PARAMETER,?' or 'PARAMETER,DEFAULT' or 'PARAMETER,kI,kP,...'\n");
        esp_restart();
    }

    if (usb.getWorkingMode() == MODE_RESTART)
    {
        esp_restart();
    }

      

    // Process parameters from PC
    string p0 = "", p1 = "";
    int parameterCount;
    p0 = usb.getParametersFromPc()[0];
    parameterCount = usb.getParametersFromPc().size();
    if (parameterCount == 2)
    {
        p1 = usb.getParametersFromPc()[1];
    }

    // Handle different working modes
    if (usb.getWorkingMode() == MODE_ADJUST_TEST_TIP)
    {
        adjustStart();
        vTaskDelete(NULL);
    }
    else if (usb.getWorkingMode() == MODE_TUNNEL_FIND)
    {
        parameterSetter.getParametersFromFlash(false); // Get measurement parameters from NVS
        findTunnelStart();
        vTaskDelete(NULL);
    }
    else if (usb.getWorkingMode() == MODE_MEASURE)
    {
        parameterSetter.getParametersFromFlash(false); // Get measurement parameters from NVS
        measureStart();
        vTaskDelete(NULL);
    }
    else if (usb.getWorkingMode() != MODE_PARAMETER)
    {
        UsbPcInterface::printErrorMessageAndRestart("INVALID Command MODE");
    }

    // PARAMETER,?
    if (p1.compare("?") == 0)
    {
        parameterSetter.getParametersFromFlash(true);
        esp_restart();
        UsbPcInterface::printErrorMessageAndRestart("");
    }
    // PARAMETER,DEFAULT
    else if (p1.compare("DEFAULT") == 0)
    {
        esp_err_t err = parameterSetter.putDefaultParametersToFlash();
        if (err == ESP_OK)
        {
            parameterSetter.getParametersFromFlash(true);
            esp_restart();
        }
        else
        {
            UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,float,float,...");
        }
    }
    // PARAMETER,10,1000,10.0,0.01,0,0,0,199,199,10
    else if (parameterCount == 13)
    {
        esp_err_t err = parameterSetter.putParametersToFlash(usb.getParametersFromPc());
        if (err == ESP_OK)
        {
            parameterSetter.getParametersFromFlash(true);
            esp_restart();
        }
        else
        {
            UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,p1,p2,...,p9");
        }
    }
    else
    {
        UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nInvalid number of parameters");
    }

    ESP_LOGI(TAG, "--- delete main task\n");
    vTaskDelete(NULL);
}
