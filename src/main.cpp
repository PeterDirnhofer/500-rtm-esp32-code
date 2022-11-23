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


// Wenn spi nicht geht suche in vspiDacInit() PeDi Added
// .rx_flow_ctrl_thresh = 0,  Added in uart_config
// devcfgDacY 
using namespace std;

extern "C" void app_main(void)
{

    esp_log_level_set("*", ESP_LOG_WARN);
    static const char *TAG = "main";

    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT); // Blue LED as Output
    gpio_set_level(BLUE_LED, 1);

    UsbPcInterface usb;
    usb.start();
    UsbPcInterface::send("IDLE\n");

    ParameterSetting parameterSetter;

    // If no parameters in Flash Set Default Parameters
    if (parameterSetter.parametersAreValid() != ESP_OK)
    {
        parameterSetter.putDefaultParametersToFlash();
    }

    // ##############################################################
    // SELECT Run Mode

    // Wait for command from PC via USB
    if (usb.getCommandsFromPC() != ESP_OK)
    {
        UsbPcInterface::printErrorMessageAndRestart("Invalid command. \n 'PARAMETER,?' or 'PARAMETER,DEFAULT' or 'PARAMETER,kI,kP,...'\n");
        esp_restart();
    }

    string p0 = "", p1 = "";
    int parameterCount;
    p0 = usb.getParametersFromPc()[0];
    parameterCount=usb.getParametersFromPc().size();
    if (parameterCount == 2)
        p1 = usb.getParametersFromPc()[1];

    if (usb.getWorkingMode() == MODE_ADJUST_TEST_TIP)
    {
        //UsbPcInterface::send("ADJUST\n");
        displayTunnelCurrent();
        vTaskDelete(NULL);

    }
    else if (usb.getWorkingMode() == MODE_MEASURE)
    {
        //UsbPcInterface::sendStatus("MEASURE\n");
        controllerStart();
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
            UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,float,float,....");
        }
    }

    // PARAMETER,10,1000,10.0,0.01,0,0,0,199,199,10
    else if (parameterCount==11)
    {

        esp_err_t err = parameterSetter.putParametersToFlash(usb.getParametersFromPc());
        if (err == ESP_OK)
        {
            parameterSetter.getParametersFromFlash(true);
            //usb.sendInfo("PARAMETER set OK\n");
            esp_restart();
        }
        else
        {
            UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nRequired Format is \nPARAMETER,p1,p2,...,p9");
        }
    }
    else {
        UsbPcInterface::printErrorMessageAndRestart("PARAMETER SET ERROR\nInvalid number of parameters");
    }

    ESP_LOGI(TAG, "--- delete main task\n");
    vTaskDelete(NULL);
}