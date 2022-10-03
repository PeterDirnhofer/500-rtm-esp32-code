#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "timer.h"
#include <string>
#include <cstring>

// private includes
#include "globalVariables.h"
#include "spi.h"
#include "adc.h"
#include "controller.h"
#include "dataStoring.h"

// private classes
#include "UsbPcClass.h"
#include "ParameterClass.h"

// static members of UsbPcClass are declared in UsbPcClass.h
// Need to be initialized from outside the class
std::string UsbPcClass::mUsbReceiveString = "";
bool UsbPcClass::usbAvailable = false;

extern "C" void app_main(void)
{

    esp_log_level_set("*",ESP_LOG_WARN);
    static const char *TAG = "main";
    ESP_LOGI(TAG, "\n+++ START ++++++++++++\n");

    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT); // Blue LED as Output
    gpio_set_level(BLUE_LED, 1);

    UsbPcClass usb;
    usb.start();

    ParameterClass parameterClass;

    // SELECT Run Mode
    // Wait for command from PC via USB
    usb.getPcCommadToSetWorkingMode();

    if (usb.getworkingMode() == MODE_SETUP)
    {
        usb.send("START SETUP\n");
        displayTunnelCurrent();
    }
    else if (usb.getworkingMode() == MODE_MEASURE)
    {
        usb.send("START MEASURE\n");
        controllerStart();
    }
    else if (usb.getPcCommadToSetWorkingMode() == MODE_PARAMETER)
    {
        usb.send("PARAMETER STARTED\n");

        std::string p0 = "", p1 = "";
        p0 = usb.getParameters()[0];
        if (usb.getParameters().size() > 1)
            p1 = usb.getParameters()[1];

        if (usb.getParameters().size() == 2)
        {
            // ################## PARAMETER,?
            if (p1.compare("?") == 0)
            {
                usb.send("PARAMETER READ\n");

                for (size_t i = 0; i < parameterClass.getParameters().size(); i++)
                {
                    usb.send(parameterClass.getParameters()[i].c_str());
                    if (i < parameterClass.getParameters().size() - 1)
                    {
                        usb.send(",");
                    }
                }
                usb.send("\n");

                esp_restart();
            }
            // ################## PARAMETER,DEFAULT
            else if (p1.compare("DEFAULT") == 0)
            {
                usb.send("SET DEFAULT PARAMETER TBD\n");
                ESP_LOGI(TAG, "SET DEFAULT\n");
                esp_restart();
            }
        }
        else
        {
            usb.send("SET PARAMETER\n");

            // ################## PARAMETER,100,1000,10.0,0.01,0,0,0,199,199

            esp_err_t err = parameterClass.setParameters(usb.getParameters());
            if (err == ESP_OK)
            {
                usb.send("PARAMETER set OK\n");
                esp_restart();
            }
            else
            {
                usb.send("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,float,float,....\n");
                esp_restart();
            }
        }
    }
    else
    {
        ESP_LOGI(TAG, "Invalid command \n");
        usb.send("INVALID COMMAND\n");
        esp_restart();
    }

    ESP_LOGI(TAG, "--- delete main task\n");
    vTaskDelete(NULL);
}