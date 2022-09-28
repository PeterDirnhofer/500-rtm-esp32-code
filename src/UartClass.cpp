#include <UartClass.h>
// C:\Users\peter\git-esp-idf\UART\src

// https://github.com/espressif/esp-idf/blob/30e8f19f5ac158fc57e80ff97c62b6cc315aa337/examples/peripherals/uart/uart_async_rxtxtasks/main/uart_async_rxtxtasks_main.c
/* UART asynchronous example, that uses separate RX and TX tasks
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h" // esp uart
#include "string.h"
#include "driver/gpio.h"
#include "string"
#include "uartLocal.h" // own uart
#include "globalVariables.h"

#include "spi.h"
#include "communication.h"
#include "controller.h"
#include "timer.h"
#include "string.h"
#include <iostream>
#include <string>
#include <cstring>
#include <esp_log.h>
#include "timer.h"
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const int RX_BUF_SIZE = 100;
static const char *TAG = "UartClass";

UartClass::UartClass()
    : task_handle(NULL)
{
}

UartClass::~UartClass()
{
}

/**
 * @brief init Uart for communication with Laptop usb
 *
 *
 */
void UartClass::start()
{

    this->uartInit();
    // xTaskCreatePinnedToCore(this->uartRcvLoop, "uartRcvLoop", 10000, NULL, 4, &this->task_handle, 0);
    xTaskCreatePinnedToCore(this->uartRcvLoop, "uartRcvLoop", 10000, NULL, 4, &this->task_handle, (BaseType_t)0);
}

void UartClass::uartInit()
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    
}

void UartClass::uartRcvLoop(void *unused)
{
    // https://github.com/espressif/esp-idf/blob/af28c1fa21fc0abf9cb3ac8568db9cbd99a7f4b5/examples/peripherals/uart/uart_async_rxtxtasks/main/uart_async_rxtxtasks_main.c

    std::string rcvString = "";
    bool found_CR;
    
    
    

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

    ESP_LOGI(TAG, "*** STARTED \n");
    found_CR = false;
    while (1)
    {

        // uart_flush()
        const int rxCount = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if ((rxCount > 0) && (found_CR == false))
        {
            data[rxCount] = 0;
            int i = 0;

            // Input ends with CR LF = 13 10
            while ((i < rxCount) && (found_CR == false))
            {
                if (data[i] == 0xD) // Carriage Return
                {
                    found_CR = true;
                }
                else if (data[i] >= 0x20) // Save only printable characters
                {
                    rcvString += (char)data[i];
                }
                i++;
            }
            usbReceive.clear();
            if (found_CR)
            {

                ESP_LOGI(TAG, "#### 13 found.  \n");

                ESP_LOGI(TAG, "Start Analyse rcvString\n%s\n", rcvString.c_str());
                usbReceive.append(rcvString);
                usbAvailable=true;

                ESP_LOGI(TAG,"usbReceive %s\n",usbReceive.c_str());

                // Split rcvString
                // https://www.tutorialspoint.com/cpp_standard_library/cpp_string_c_str.htm

                char *cstr = new char[rcvString.length() + 1];
                std::strcpy(cstr, rcvString.c_str());

                // how many comma are in string
                int numberOfValues=1;
                for (int i=0; i < rcvString.length();i++)
                    if(cstr[i]==',') numberOfValues++;
            
                ESP_LOGI(TAG,"numberOfValues %d",numberOfValues);

                //std::string parameters[10];
                int parametersIndex;

                std::string parameters[10];
                parameters[0]="wert1";

                
                char *p = std::strtok(cstr, ",");
                parametersIndex=0;
                while (p != 0)
                {
                    printf("%s\n", p);
                    char buffer[20];
                    sprintf(buffer,"%s",p);
                    parameters[parametersIndex++]=buffer;                 
                    p = std::strtok(NULL, ",");
                }

                delete[] cstr;
            }
        }
    }
    free(data);
}
