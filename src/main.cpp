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
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "driver/spi_slave.h"
//#include "esp_log.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"

#include "globalVariables.h"
#include "spi.h"
#include "adc.h"
#include "controller.h"
#include "dataStoring.h"
#include "timer.h"

#include "uart.h"

/**
 * @brief Set default data when useing without raspberry
 *
 */
void setupDefaultData()
{

    //printf("Setup defaul data\n");
    // Simulation data configExisting
    configNeeded = false;   
    
    kI = 50;
    kP = 100;
    destinationTunnelCurrentnA = 20.0;
    remainingTunnelCurrentDifferencenA = 0.1;
    rtmGrid.setDirection(false);
    rtmGrid.setMaxX(199);
    rtmGrid.setMaxY(199);


    printf("%f \n", kI);
    printf("%f \n", kP);
    printf("%f \n", destinationTunnelCurrentnA);
    printf("%f \n", remainingTunnelCurrentDifferencenA);
    printf("%d \n", rtmGrid.getCurrentX());
    printf("%d \n", rtmGrid.getCurrentY());
    printf("%d \n", direction);
    printf("%d \n", rtmGrid.getMaxX());
    printf("%d \n", rtmGrid.getMaxY());



}

// Main application
extern "C" void app_main(void)
{
    printf("\n\n+++ app_main START +++\n\n");
    //setupDefaultData();

    // wird nur verwendet, wenn Parameter üer UART übergeben werden sollen
    //uartStart(); 
   
    hspiStart();
    
    //controllerStart();


        
    

    while (1)
    {
        printf("TICK main\n");

        // https://esp32.com/viewtopic.php?t=10411
        // Delete task to omit task_wdt timeout error
        vTaskDelete(NULL);
    }
}
