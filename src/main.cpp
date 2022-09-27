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
//#include "nvs_flash.h"
//#include "nvs.h"
#include "soc/rtc_periph.h"
#include "driver/spi_slave.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "globalVariables.h"
#include "spi.h"
#include "adc.h"
#include "controller.h"
#include "dataStoring.h"
#include "timer.h"

#include "uartLocal.h"
#include "parameter.h"

#include "nvs_flash.h"
#include "nvs.h"

#include <UartClass.h>

// static members of UartClass are declared in UArtClass.h
// Need to be initialized from outside the class
int UartClass::staticIntVar=123;
std::string UartClass::usbReceive="";



/// @brief Startet tasks und beendet sich dann selbst
/// @param



extern "C" void app_main(void)
{
    static const char *TAG = "main";

   
    ESP_LOGI(TAG,"\n+++ START ++++++++++++\n");
  
    // modeWorking Betriebsart wählen
    // MODE_MEASURE : Normalbetrieb
    // MODE_MONITOR_TUNNEL_CURRENT : Zeigt Tunnel ADC im Sekundentakt
    modeWorking = MODE_MONITOR_TUNNEL_CURRENT;
    //UartClass uc;

    UartClass uk;
    
    uk.start();
    
   
    
    return;

    

    //modeWorking = MODE_MEASURE;
    uartStart();
   

    









    
    uartSend("Mit ESC in Tunnel current Monitoring Modus springen\n");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    
    //modeWorking = MODE_MONITOR_TUNNEL_CURRENT;

    if (modeWorking==MODE_MONITOR_TUNNEL_CURRENT)
    {
        displayTunnelCurrent();
    }
    else
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
