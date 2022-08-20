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
#include "esp_log.h"
#include "driver/uart.h" // esp uart
#include "string.h"
#include "driver/gpio.h"
#include "string"
#include "uart.h" // own uart
#include "globalVariables.h"

#include "spi.h"
#include "communication.h"
#include "controller.h"
#include "timer.h"
#include "string.h"
#include <iostream>
#include <string>

static const int RX_BUF_SIZE = 100;

#define TXD_PIN (GPIO_NUM_33)
#define RXD_PIN (GPIO_NUM_32)

void uartInit(void)
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

    memset(recvbufferUart, 0, 10);
}

void uartStart()
{
    uartInit();

    //printf("Starting uartLoop ... ");
    //xTaskCreatePinnedToCore(uartLoop, "uartLoop", 10000, NULL, 4, &handleUartLoop, 0);

    printf("Starting uartRcvLoop ... ");
    xTaskCreatePinnedToCore(uartRcvLoop, "uartRcvLoop", 10000, NULL, 4, &handleUartRcvLoop, 0);
}

int logMonitor(const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    return txBytes;
}

using namespace std;
string convertToString(char *a)
{
    string s(a);
    return s;
}

float toFloat(char * h1)
{
    

    //printf("kI als String: %s\n", splitStrings[1]);
    
    string helps = convertToString(h1);
    char *ending;
    float converted_value = strtof(helps.c_str(), &ending);
    if (*ending != 0)
    {
        printf("Error cold not convert %s to float\n", h1);
    }
    printf("coverted value %f: \n", converted_value);
    return converted_value;
}

void uartRcvLoop(void *unused)
{
    int stIndex = 0;
    char st[100];

    

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    printf("uartRcvLoop GESTARTET \n");
    while (1)
    {

        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (rxBytes > 0)
        {

            // Terminate input with 0
            data[rxBytes] = 0;
            for (int i = 0; i < rxBytes; i++)
            {
                char c = data[i];

                st[stIndex++] = c;
                st[stIndex] = 0;

                if (stIndex > 99)
                {
                    printf("ERROR UART received too many bytes\n");

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
                    logMonitor(st); 
                    logMonitor("nach protocolElemet erzeugen\n");
                    stIndex = 0;
                }
            }
        }
    }
}

void uartLoop(void *unused)
{

    // Raspberry sends
    // uint8_t transmitBuffer[10];
    // Raspberry receives
    // spiReceiveCommand(uint16_t sizeBytes, uint8_t* transmitBuffer, uint8_t* receiveBuffer);

    printf("uartLoop GESTARTET\n");
    bool dataReadyLastCycle = false;
    u_int8_t lastConfigStatus = 0;

    while (1)
    {
        /* code */

        // Clear receive buffer, set send buffer to something sane
        memset(recvbufferUart, 0x00, 10);

        if (dataReadyLastCycle)
        {
            printf("suspend last cycle");
            logMonitor("suspend last cycle\n");
            // handshake line high
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);
            dataReadyLastCycle = false;
            vTaskResume(handleControllerLoop);
            vTaskSuspend(NULL);
        }

        // check if transaction is necessary or datasets are waiting for being sent, otherwise suspend and wait for
        // printf("config: %02x \n", configExisting);

        if ((configExisting == 0x1FF) && !dataReady)
        { // 0x1FF is 0b111111111 -> nine ones
            // handshake line high
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);
            configNeeded = false;
            printf("UART Loop suspend \n");
            logMonitor("UART Loop suspend \n");
            printf("%f \n", kI);
            printf("%f \n", kP);
            printf("%f \n", destinationTunnelCurrentnA);
            printf("%f \n", remainingTunnelCurrentDifferencenA);
            printf("%d \n", rtmGrid.getCurrentX());
            printf("%d \n", rtmGrid.getCurrentY());
            printf("%d \n", direction);
            printf("%d \n", rtmGrid.getMaxX());
            printf("%d \n", rtmGrid.getMaxY());

            controllerStart();
            vTaskSuspend(NULL);
        }

        // transaction is necessary, set configbyte
        if (configNeeded)
        {
            if (lastConfigStatus != (uint8_t)(1 << POSITION_ESP_READY_CONFIGFILE))
            {
                logMonitor("config Needed \n");
            }
            lastConfigStatus = (uint8_t)(1 << POSITION_ESP_READY_CONFIGFILE);

            sendbufferUart[0] = (uint8_t)(1 << POSITION_ESP_READY_CONFIGFILE); // 0x04
        }
        else if (dataReady)
        {
            vTaskSuspend(handleControllerLoop);
            dataQueue.front();
            uint16_t X = dataQueue.front().getDataX();
            uint16_t Y = dataQueue.front().getDataY();
            uint16_t Z = dataQueue.front().getDataZ();
            printf("%d %d %d\n", X, Y, Z);

            //logMonitor("%d %d %d\n");

            dataQueue.pop(); // remove from queue

            // decide whether last dataset to send or not, effects endtag in configbyte
            if (dataQueue.empty())
            {
                dataReady = false;
                dataReadyLastCycle = true;
                // printf("Last dataset to send\n");
                sendbufferUart[0] = (uint8_t)((1 << POSITION_ESP_SEND_DATA) | (1 << POSITION_ESP_ENDTAG)); // 0x11
            }
            else
            {
                sendbufferUart[0] = (uint8_t)(1 << POSITION_ESP_SEND_DATA); // 0x10
            }
            memcpy(&sendbufferUart[1], &X, 2);
            memcpy(&sendbufferUart[3], &Y, 2);
            memcpy(&sendbufferUart[5], &Z, 2);
        }

        vTaskDelay(1); // Verhindert wdt overflow
    }
}        
