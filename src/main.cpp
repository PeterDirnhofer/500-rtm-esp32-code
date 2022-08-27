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
#include "parameter.h"

#include "nvs_flash.h"
#include "nvs.h"


/// @brief Startet tasks und beendet sich dann selbst
/// @param
extern "C" void app_main(void)
{

    printf("+++ START\n");

       
    hspiStart();

    // Bei uartStart wird nur vspi gebraucht
    // hspiStart();

    // Nur ohne Raspberry. Kommunikation übe UART

    bool UART_ACTIVE = true;
    if (UART_ACTIVE)
    {
        uartStart();
        controllerStart();
    }

    while (1)
    {
        printf("--- delete app_main\n");

        // https://esp32.com/viewtopic.php?t=10411
        // Delete task to omit task_wdt timeout error
        vTaskDelete(NULL);
    }
}
