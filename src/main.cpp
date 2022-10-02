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

// private includes
#include "globalVariables.h"
#include "spi.h"
#include "adc.h"
#include "controller.h"
#include "dataStoring.h"
#include "parameter.h"

// private classes
#include "UartClass.h"
#include "preferences.h"


// static members of UartClass are declared in UArtClass.h
// Need to be initialized from outside the class
std::string UartClass::usbReceive="";
bool UartClass::usbAvailable=false;



extern "C" void app_main(void)
{
    static const char *TAG = "main";

    gpio_set_direction(BLUE_LED, GPIO_MODE_OUTPUT); // Blue LED as Output
    gpio_set_level(BLUE_LED,1);
    UartClass usb;
    Preferences prefs;

    

    prefs.begin();
    
    //prefs.putChar("CHAR1",'C');
    

    int8_t rc1=0;
    rc1= prefs.getChar("CHAR1",0);

    float floatresult = 0;

    //floatresult = prefs.putFloat("FLOAT1",1.2334);

    floatresult = prefs.getFloat("FLOAT1",0.0);

    ESP_LOGI(TAG,"%f\n",floatresult);

    
    
   

    return;


    ESP_LOGI(TAG,"\n+++ START ++++++++++++\n");
    usb.start();
    
    // Wait for command from PC via USB
    usb.getPcCommad();
    if (usb.getworkingMode()==MODE_MONITOR_TUNNEL_CURRENT)
    {
        displayTunnelCurrent();
    }
    else if (usb.getworkingMode()==MODE_MEASURE)
    {
        controllerStart();    
    }
    
    
    while (1)
    {
        printf("--- delete  app_main\n");

        // https://esp32.com/viewtopic.php?t=10411
        // Delete task to omit task_wdt timeout error
        vTaskDelete(NULL);
    }
}
