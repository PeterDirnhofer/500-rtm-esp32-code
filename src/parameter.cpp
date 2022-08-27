
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

#define STORAGE_NAMESPACE "parameters"
nvs_handle_t my_handle;
esp_err_t err;



void nvsSet(int16_t value, const char *key)
{
    bool labelExists=false;
    printf("+++   nvsSet\n");

    nvs_iterator_t it = nvs_entry_find(STORAGE_NAMESPACE, NULL, NVS_TYPE_ANY);
    printf("+++   nvsSet1\n");
    while (it != NULL)
    {
        printf("+++   nvsSet2\n");
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        it = nvs_entry_next(it);
        printf("key '%s', type '%d' ", info.key, info.type);
        if (strcmp(info.key, key) == 0)
        {
            labelExists = true;
        }
    }
    printf("+++   nvsSet3\n");
    nvs_release_iterator(it);
    printf("+++   nvsSet4\n");
    if (!labelExists)
    {
        printf("Adding new label \n");
        err=nvs_set_i16(my_handle,key,value);

        printf("result: %s\n",esp_err_to_name(err));
        
        err = nvs_commit(my_handle);
        printf("confirm: %s\n",esp_err_to_name(err));

    }
    printf("+++   nvsSet6\n");
    // Note: no need to release iterator obtained from nvs_entry_find function when nvs_entry_find or nvs_entry_next function return NULL, indicating no other element for specified criteria was found. }
}





esp_err_t print_what_saved(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read restart counter
    int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, "restart_conter", &restart_counter);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    printf("Restart counter = %d\n", restart_counter);

    // Read run time blob
    size_t required_size = 0;  // value will default to 0, if not set yet in NVS
    // obtain required memory space to store blob being read from NVS
    err = nvs_get_blob(my_handle, "run_time", NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;
    printf("Run time:\n");
    if (required_size == 0) {
        printf("Nothing saved yet!\n");
    } else {
        uint32_t* run_time = (uint32_t*)malloc(required_size);
        err = nvs_get_blob(my_handle, "run_time", run_time, &required_size);
        if (err != ESP_OK) {
            free(run_time);
            return err;
        }
        for (int i = 0; i < required_size / sizeof(uint32_t); i++) {
            printf("%d: %d\n", i + 1, run_time[i]);
        }
        free(run_time);
    }

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}



esp_err_t nvsGetString(const char * value,const char * key){
    nvs_handle_t my_handle;
    esp_err_t err;    
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    

 
    char* outvalue="so ein tAg";
    // Read the size of memory space required for blob
    size_t required_size = (size_t)10;
   
    printf("outvalue vorher %s\n",outvalue);

    err = nvs_get_str(my_handle,key,outvalue,&required_size);


    printf("result getting  nvs_string: %s\n",esp_err_to_name(err));


    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND){
        printf("ERROR getting  nvs_string: %s\n",esp_err_to_name(err));
        return err;
    }
 
    printf("outvalue nachher %s\n",outvalue);
    return ESP_OK;


}


esp_err_t nvsSetString(const char * value,const char * key){
    nvs_handle_t my_handle;
    esp_err_t err;    
    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
    
    
    size_t required_size = strlen(value);
    printf("required_size is %d\n",(int)required_size);

    err = nvs_set_str(my_handle,key,value);
    if (err != ESP_OK){
        printf("ERROR writing nvs_string: %s\n",esp_err_to_name(err));
        return err;
    } 

    err = nvs_commit(my_handle);    
     if (err != ESP_OK){
        printf("ERROR nvs_commit %s\n",esp_err_to_name(err));
        return err;
    } 
    printf("write no error \n");

    return ESP_OK;
}


esp_err_t nvsGet(int16_t &value, const char* key){
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
    // Read
    int16_t rvalue = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i16(my_handle, key, &rvalue);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) 
        return err;
    
    value=rvalue;
    return ESP_OK;
    
}

esp_err_t nvsSet(int16_t &value, const char* key){
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;
   // Write
    err = nvs_set_i16(my_handle, key, value);
    if (err != ESP_OK) 
        return err;
    return ESP_OK;
    
}


esp_err_t save_restart_counter(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return err;

    // Read
    int32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
    err = nvs_get_i32(my_handle, "restart_conter", &restart_counter);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) return err;

    // Write
    restart_counter++;
    err = nvs_set_i32(my_handle, "restart_conter", restart_counter);
    if (err != ESP_OK) return err;

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK) return err;

    // Close
    nvs_close(my_handle);
    return ESP_OK;
}

bool nvsInit()
{

printf("\n\n+++ app_main START +++\n");
    esp_err_t ret = 0;
    nvs_handle handler = 0;
    ret = nvs_flash_init_partition("nvs");
    printf("nvs_flash_init_partition CODE: %d\n", ret);

    

    ret = nvs_open_from_partition("nvs", "tele", NVS_READWRITE, &handler);
    printf("nvs_open CODE: %d\n", ret);

    

    ret = nvs_set_u8(handler, "kk", 7);
    printf("nvs_set_u8 CODE: %d\n", ret);

  

    ret = nvs_commit(handler);
    printf("nvs_commit CODE: %d\n", ret);

   

    int8_t max_buffer_size = 256;
    ret = nvs_get_i8(handler, "kk", &max_buffer_size);
    printf("nvs_get_u8 CODE: %d\n", ret);
    printf("nvs_get_u8 STORED: %d\n", max_buffer_size);

   

    ret = nvs_flash_deinit_partition("nvs");
    printf("nvs_flash_deinit_partition CODE: %d\n", ret);

    nvs_close(handler);






return true;
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK)
    {
        printf("ERROR nvs_flash_init %s", esp_err_to_name(err));
        return false;
    }

    // open
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
    {
        printf("ERROR: cannot open NVS storage\n");
        return false;
    }
    else
    {
        printf("+++ NVS open ok\n");
    }


    
    return true;
}

bool nvsRead(const char *label)
{
    // Read the size of memory space required for blob
    size_t required_size = 0;
    err = nvs_get_blob(my_handle, label, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND)
    {
        printf("ERROR nvs_get_blob\n");
        return false;
    }
    else
    {
        printf("+++ nvs_get_blob OK\n");
    }

    // Read previously saved blob if available
    uint32_t *run_time = (uint32_t *)malloc(required_size + sizeof(uint32_t));
    if (required_size > 0)
    {
        err = nvs_get_blob(my_handle, label, run_time, &required_size);
        if (err != ESP_OK)
        {
            free(run_time);
            printf("ERROR nvs_get_blob\n");
            return false;
        }
    }

    return true;
}

void setupDefaultData()
{
    size_t required_size = 0;

    /*


        // Write value including previously saved blob if available
        required_size += sizeof(uint32_t);
        run_time[required_size / sizeof(uint32_t) - 1] = xTaskGetTickCount() * portTICK_PERIOD_MS;
        err = nvs_set_blob(my_handle, "run_time", run_time, required_size);
        free(run_time);

        if (err != ESP_OK)
        {
            printf("ERROR nvs_setBlb\n");
            return;
        }
        err = nvs_commit(my_handle);


    */

    // Simulation data configExisting
    // configNeeded = false;

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
