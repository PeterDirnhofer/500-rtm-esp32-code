
// https://github.com/espressif/esp-idf/blob/master/examples/storage/nvs_rw_blob/main/nvs_blob_example_main.c
// Es gibt NVS key-value store
// und SPIFFS für grössere files

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/gpio.h"

#include "globalVariables.h"
#include "parameter.h"
#include "strings.h"
#include "string.h"
#include "ctype.h"

#define STORAGE_NAMESPACE "parameters"
nvs_handle_t my_handle;
esp_err_t err;


/* Save the number of module restarts in NVS
   by first reading and then incrementing
   the number that has been saved previously.
   Return an error if anything goes wrong
   during this process.
 */
esp_err_t save_restart_counter(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    // Read
    int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, "restart_conter", &restart_counter);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;

    // Write
    restart_counter++;
    err = nvs_set_i32(my_handle, "restart_conter", restart_counter);
    if (err != ESP_OK)
        return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
        return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

/* Read from NVS and print restart counter
   and the table with run times.
   Return an error if anything goes wrong
   during this process.
 */
esp_err_t print_what_saved(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    // Read restart counter
    int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, "restart_conter", &restart_counter);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    printf("Restart counter = %d\n", restart_counter);

    // Read myParamr
    int32_t myParam = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, "myParam", &myParam);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    printf("myParam = %d\n", myParam);

    // Read myString
    size_t required_size = 0; // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_str(my_handle, "myString", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    printf("myString:\n");
    if (required_size == 0)
    {
        printf("Nothing saved yet!\n");
    }
    else
    {

        char outValue[100] = {0};
        err = nvs_get_str(my_handle, "myString", outValue, &required_size);
        if (err != ESP_OK)
        {
            printf("ERROR read mystring\n");
            return err;
        }
        else
            printf("myString found\n");
        printf("%s\n", outValue);
    }

    // Read run time blob
    required_size = 0; // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_blob(my_handle, "run_time", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
        return err;
    printf("Run time:\n");
    if (required_size == 0)
    {
        printf("Nothing saved yet!\n");
    }
    else
    {
        uint32_t *run_time = (uint32_t *)malloc(required_size);
        err = nvs_get_blob(my_handle, "run_time", run_time, &required_size);
        if (err != ESP_OK)
        {
            free(run_time);
            return err;
        }
        for (int i = 0; i < required_size / sizeof(uint32_t); i++)
        {
            printf("%d: %d\n", i + 1, run_time[i]);
        }
        free(run_time);
    }

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

void setupDefaultData()
{
    size_t required_size = 0;

    

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
