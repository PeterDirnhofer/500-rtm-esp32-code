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
    UsbPcInterface::send("+++ START\n");

    ParameterSetting parameterSetter;


/*
    if(!parameterSetter.parametersAreValid()){

        UsbPcInterface::send("Set Default parameters\n");
        
    }
    
*/
    // SELECT Run Mode
    // Wait for command from PC via USB
       
    if (usb.getCommandsFromPC() != ESP_OK)
    {
        UsbPcInterface::printErrorMessageAndRestart("Invalid command. \n 'PARAMETER,?' or 'PARAMETER,DEFAULT' or 'PARAMETER,kI,kP,...'\n");
        esp_restart();
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

    else if (usb.getCommandsFromPC() == MODE_PARAMETER)
    {

        string p0 = "", p1 = "";
        p0 = usb.getParametersFromPc()[0];
        
        if (usb.getParametersFromPc().size() > 1)
        {
            p1 = usb.getParametersFromPc()[1];
            // PARAMETER,?
            if (p1.compare("?") == 0)
            {

                for (size_t i = 0; i < parameterSetter.getParametersFromFlash().size(); i++)
                {
                    usb.send(parameterSetter.getParametersFromFlash()[i].c_str());
                    if (i < parameterSetter.getParametersFromFlash().size() - 1)
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
                esp_err_t err = parameterSetter.putDefaultParametersToFlash();
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
                esp_err_t err = parameterSetter.putParametersToFlash(usb.getParametersFromPc());
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
