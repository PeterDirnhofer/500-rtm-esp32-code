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
    bool found_13;

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    uint8_t *extracted = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    memset(extracted, 0, RX_BUF_SIZE + 1);

    ESP_LOGI(TAG, "*** STARTED \n");
    found_13 = false;
    while (1)
    {

        // uart_flush()
        const int rxCount = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if ((rxCount > 0) && (found_13 == false))
        {
            data[rxCount] = 0;
            int i = 0;

            // ESP_LOGI(TAG, "rxCount: %d\n", rxCount);

            while ((i < rxCount) && (found_13 == false))
            {
                if (data[i] == 13)
                {
                    found_13 = true;
                }
                else
                {
                    rcvString += (char)data[i];
                }
                i++;
            }

            if (found_13)
            {

                ESP_LOGI(TAG, "13 found.  \n");

                const char *str = rcvString.c_str();
                ESP_LOGI(TAG, "Start Analyse rcvString = %s\n", str);
               

                // https://www.includehelp.com/c-programs/c-program-to-split-string-by-space-into-words.aspx

                char splitStrings[10][10]; // can store 10 words of 10 characters
                int j, cnt;

                j = 0;
                cnt = 0;
            }

            /*
                        for (int i = 0; i < rxCount; i++)
                        {

                            ESP_LOGI(TAG,"bytes read %d\n",rxCount);
                            char c = data[i];

                            st[stIndex++] = c;
                            st[stIndex] = 0;

                            if (stIndex > 99)
                            {
                                ESP_LOGE(TAG,"ERROR UART received too many bytes\n");

                                stIndex = 0;
                                memset(&(st[0]), 0, 100); // clear array st
                            }



                            if (c == 13)
                            {

                                // std::string str(st);

                                // https://www.includehelp.com/c-programs/c-program-to-split-string-by-space-into-words.aspx
                                // char str[100];
                                char splitStrings[10][10]; // can store 10 words of 10 characters
                                int i, j, cnt;

                                j = 0;
                                cnt = 0;
                                for (i = 0; i <= (strlen(st)); i++)
                                {
                                    // if space or NULL found, assign NULL into splitStrings[cnt]
                                    if (st[i] == ' ' || st[i] == '\0')
                                    {
                                        splitStrings[cnt][j] = '\0';
                                        cnt++; // for next word
                                        if (cnt > 9)
                                        {
                                            // logMonitor ("ERROR. maximum 9 Data");
                                            printf("ERROR. maximum 9 Data");
                                            return;
                                        }

                                        j = 0; // for next word, init index to 0
                                    }
                                    else
                                    {
                                        splitStrings[cnt][j] = st[i];
                                        j++;
                                    }
                                }

                                printf("\nStrings (words) after split by space:\n");
                                for (i = 0; i < cnt; i++)
                                {

                                    printf(splitStrings[i]);
                                    printf("\n");
                                }

                                kI = atof(splitStrings[1]);
                                kP = atof(splitStrings[2]);
                                destinationTunnelCurrentnA = toFloat(splitStrings[3]);
                                remainingTunnelCurrentDifferencenA = toFloat(splitStrings[4]);
                                startX = (uint16_t)atoi(splitStrings[5]);
                                startY = (uint16_t)atoi(splitStrings[6]);
                                direction = (bool)atoi(splitStrings[7]);

                                // maxX = (uint16_t) atoi(splitStrings[8]);
                                // maxY = (uint16_t) atoi(splitStrings[9]);

                                // double dataArray[] = {kI, kP, destinationTunnelCurrentnA, remainingTunnelCurrentDifferencenA, (double) startX, (double) startY, (double) direction, (double) maxX, (double) maxY};

                                //    printf("%s\n", splitStrings[i]);

                                // std::unique_ptr<protocolElement> currentReceivedCommand = std::make_unique<protocolElement>((uint8_t*) st); //create protocolElement, constructor will proceed

                                ESP_LOGI(TAG,"nach protocolElemet erzeugen\n");
                                stIndex = 0;
                            }
                        }
            */
        }
    }
    free(data);
}
