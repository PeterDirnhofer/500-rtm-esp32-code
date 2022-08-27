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

    esp_err_t err;
    printf("+++ START\n");

    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = saveStringParameter("vi","1000");
    if (err != ESP_OK) printf("Error (%s) saveStringParameter\n", esp_err_to_name(err));
 
    
    char austausch[10] = {0};

    err = readStringParameter("vi",austausch);

    printf("nachher %s\n",austausch);




    

     return;



    err = saveParameter(1234);
    if (err != ESP_OK) printf("Error (%s) saveParameter\n", esp_err_to_name(err));

    err = print_what_saved();
    if (err != ESP_OK) printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));

    err = saveStringParameter("vi","1000");
    if (err != ESP_OK) printf("Error (%s) saveStringParameter\n", esp_err_to_name(err));
    

    err = print_what_saved();
    if (err != ESP_OK) printf("Error (%s) reading data from NVS!\n", esp_err_to_name(err));

    err = save_restart_counter();
    if (err != ESP_OK)
        printf("Error (%s) saving restart counter to NVS!\n", esp_err_to_name(err));

    
    return;
    
  

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
