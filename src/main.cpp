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
#include "UartClass.h"
#include "ParameterClass.h"

// static members of UartClass are declared in UArtClass.h
// Need to be initialized from outside the class
std::string UartClass::usbReceive = "";
bool UartClass::usbAvailable = false;

extern "C" void app_main(void)
{
    static const char *TAG = "main";
    ESP_LOGI(TAG, "\n+++ START ++++++++++++\n");

    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT); // Blue LED as Output
    gpio_set_level(BLUE_LED, 1);

    UartClass usb;
    usb.start();

    ParameterClass parameterClass;

    // SELECT Run Mode
    // Wait for command from PC via USB
    usb.getPcCommad();
    if (usb.getworkingMode() == MODE_MONITOR_TUNNEL_CURRENT)
    {
        displayTunnelCurrent();
    }
    else if (usb.getworkingMode() == MODE_MEASURE)
    {
        controllerStart();
    }
    else if (usb.getPcCommad() == MODE_PARAMETER)
    {
        usb.send("PARAMETER STARTED\n");

        std::string p0="", p1="";
        p0=usb.getParameters()[0];
        if (usb.getParameters().size() >1)
            p1=usb.getParameters()[1];


        // PARAMETER,?
        if (usb.getParameters().size() == 2)
        {
            if (p1.compare("?")==0)
            {
                usb.send("PARAMETER READ\n");
                for (size_t i = 0; i < usb.getParameters().size(); i++)
                {
                    usb.send(usb.getParameters()[i].c_str());
                    if (i<usb.getParameters().size()-1)
                    {
                        usb.send(",");
                    }
                    
                }
                
            }
            else if (p1.compare("DEFAULT")==0)
            {
                usb.send("SET DEFAULT values\n");
                ESP_LOGI(TAG, "SET DEFAULT\n");
            }
        }
        else
        {

            esp_err_t err = parameterClass.setParameter(usb.getParameters());
            if (err != ESP_OK)
            {
                usb.send("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,float,float,....\n");
                esp_restart();
            }

            else
            {
                ESP_LOGI(TAG, "Invalid command \n");
                usb.send("INVALID COMMAND\n");
                esp_restart();
            }
        }

        ESP_LOGI(TAG, "--- delete main task\n");
        vTaskDelete(NULL);
    }
}
