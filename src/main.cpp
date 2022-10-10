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
#include "UsbPcInterface.h"
#include "ParameterSetter.h"

using namespace std;

extern "C" void app_main(void)
{

    esp_log_level_set("*", ESP_LOG_WARN);
    static const char *TAG = "main";

    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT); // Blue LED as Output
    gpio_set_level(BLUE_LED, 1);

    UsbPcInterface usb;
    usb.start();

    ParameterSetting parameterSetter;

    float testFloat,resultFloat;
    testFloat=4711;
    resultFloat=-1;

    parameterSetter.putFloat("P1",testFloat);
    
    testFloat=-1;
    resultFloat=parameterSetter.getFloat("P1",333.333);    
    
    UsbPcInterface::send("testFloat: %f  resultFloat %f\n",testFloat,resultFloat);
  
   
    

    // SELECT Run Mode
    // Wait for command from PC via USB
    usb.getPcCommadToSetWorkingMode();

    if (usb.getWorkingMode() == MODE_INVALID)
    {
        UsbPcInterface::printErrorMessageAndRestart("Invalid command. \n 'PARAMETER,?' or 'PARAMETER,DEFAULT' or 'PARAMETER,kI,kP,...'\n");
    }

    if (usb.getWorkingMode() == MODE_SETUP)
    {
        displayTunnelCurrent();
    }
    else if (usb.getWorkingMode() == MODE_MEASURE)
    {
        usb.send("MEASURE OK\n");
        controllerStart();
    }

    else if (usb.getPcCommadToSetWorkingMode() == MODE_PARAMETER)
    {

        string p0 = "", p1 = "";
        p0 = usb.getParameters()[0];
        
        if (usb.getParameters().size() > 1)
        {
            p1 = usb.getParameters()[1];
            // PARAMETER,?
            if (p1.compare("?") == 0)
            {

                for (size_t i = 0; i < parameterSetter.getParameters().size(); i++)
                {
                    usb.send(parameterSetter.getParameters()[i].c_str());
                    if (i < parameterSetter.getParameters().size() - 1)
                    {
                        usb.send(",");
                    }
                }
                usb.send("\n");

                esp_restart();
            }
            // PARAMETER,DEFAULT
            else if (p1.compare("DEFAULT") == 0)
            {
                esp_err_t err = parameterSetter.putDefaultParameters();
                if (err == ESP_OK)
                {
                    usb.send("DEFAULT PARAMETER set OK\n");
                    esp_restart();
                }
                else
                {
                    UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,float,float,....");
                }
            }
            // PARAMETER, p1, p2,.., p9
            else
            {
                esp_err_t err = parameterSetter.putParameters(usb.getParameters());
                if (err == ESP_OK)
                {
                    usb.send("PARAMETER set OK\n");
                    esp_restart();
                }
                else
                {
                    UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,p1,p2,...,p9");
                }
            }
        }

        ESP_LOGI(TAG, "--- delete main task\n");
        vTaskDelete(NULL);
    }
}
