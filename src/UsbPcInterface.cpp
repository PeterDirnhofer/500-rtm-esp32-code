#include <UsbPcInterface.h>
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

#include <memory>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/uart.h" // esp uart
#include "string.h"
#include "driver/gpio.h"
#include "string"
#include "globalVariables.h"

#include "spi.h"
#include "controller.h"
#include "timer.h"
#include "string.h"
#include <iostream>
#include <string>
#include <cstring>
#include <esp_log.h>
#include "timer.h"
#include <stdarg.h>

#include "UsbPcInterface.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <vector>
using namespace std;

static const int RX_BUF_SIZE = 200;
static const char *TAG = "UsbPcInterface";

UsbPcInterface::UsbPcInterface()
    : mTaskHandle(NULL), mStarted(false)
{
}

UsbPcInterface::~UsbPcInterface()
{
}

/**
 * @brief init Uart for communication with Laptop usb
 *
 *
 */
void UsbPcInterface::start()
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

    xTaskCreatePinnedToCore(this->mUartRcvLoop, "uartRcvLoop", 10000, NULL, 4, &this->mTaskHandle, (BaseType_t)0);
    this->mStarted = true;
}

/**
 * @brief Read CR terminated string from PC-usb into usbReceiveString. Set usbAvailable.    
 * 
 * 
 */
void UsbPcInterface::mUartRcvLoop(void *unused)
{
    // https://github.com/espressif/esp-idf/blob/af28c1fa21fc0abf9cb3ac8568db9cbd99a7f4b5/examples/peripherals/uart/uart_async_rxtxtasks/main/uart_async_rxtxtasks_main.c

    string rcvString = "";
    bool found_CR;

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

    ESP_LOGI(TAG, "*** STARTED \n");
    found_CR = false;
    while (1)
    {

        // uart_flush()
        const int rxCount = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);

        if (rxCount > 0) // && (found_CR == false))
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
                else if (data[i] == 0x3) // CTRL C
                {
                    esp_restart();
                }
                else if (data[i] >= 0x20) // Save only printable characters
                {
                    rcvString += (char)data[i];
                }
                i++;
            }

            if (found_CR)
            {
                ESP_LOGI(TAG, "#### 13 found.  \n");

                UsbPcInterface::mUsbReceiveString.clear();
                UsbPcInterface::mUsbReceiveString.append(rcvString);
                UsbPcInterface::usbAvailable = true;
                rcvString.clear();
                found_CR = false;
                ESP_LOGI(TAG, "usbReceive %s\n", mUsbReceiveString.c_str());
                free(data);
            }
        }
    }
    free(data);
}

int UsbPcInterface::send(const char *fmt, ...)
{

    va_list ap;
    va_start(ap, fmt);

    char s[100] = {0};
    vsprintf(s, fmt, ap);
    ESP_LOGI(TAG, "uartsend %s\n", s);

    const int len = strlen(s);
    int rc = uart_write_bytes(UART_NUM_1, s, len);

    va_end(ap);

    return rc;
}

/**
 * @brief Read USB input from Computer. 
 * Set workingMode to: SETUP or PARAMETER or MEASURE. 
 * getworkingMode() reads workingMode
 */
extern "C" esp_err_t UsbPcInterface::getCommandsFromPC()
{
    // Request PC. Wait for PC response
    uint32_t i = 0;
    int ledLevel = 0;

    while (UsbPcInterface::usbAvailable == false)
    {

        if ((i % 20) == 0)
        {

            // Invert Blue LED
            ledLevel++;
            gpio_set_level(BLUE_LED, ledLevel % 2);
            this->send("REQUEST\n");
        }

        vTaskDelay(100 / portTICK_RATE_MS);
        i++;
    }

    // Command received from PC
    gpio_set_level(BLUE_LED, 1);
    // Split usbReceive csv to parameters[]
    // https://www.tutorialspoint.com/cpp_standard_library/cpp_string_c_str.htm

    char *cstr = new char[UsbPcInterface::mUsbReceiveString.length() + 1];
    strcpy(cstr, UsbPcInterface::mUsbReceiveString.c_str());

    // how many comma are in string
    int numberOfValues = 1;
    for (int i = 0; i < UsbPcInterface::mUsbReceiveString.length(); i++)
        if (cstr[i] == ',')
            numberOfValues++;

    ESP_LOGI(TAG, "numberOfValues %d", numberOfValues);

    char *p = strtok(cstr, ",");

    this->mParametersVector.clear();
    while (p != 0)
    {
        //printf("%s\n", p);
        char buffer[20];
        sprintf(buffer, "%s", p);
        puts(strupr(buffer));

        this->mParametersVector.push_back(buffer);

        p = strtok(NULL, ",");
    }
    free(cstr);

    ESP_LOGI(TAG, "ParametersVector[0]: %s", this->mParametersVector[0].c_str());

    if (strcmp(this->mParametersVector[0].c_str(), "SETUP") == 0)
    {
        this->mWorkingMode = MODE_SETUP;
        ESP_LOGI(TAG, "SETUP detected\n");
        return ESP_OK;
    }
    else if (strcmp(this->mParametersVector[0].c_str(), "MEASURE") == 0)
    {
        this->mWorkingMode = MODE_MEASURE;
        ESP_LOGI(TAG, "MEASURE detected\n");
        return ESP_OK;
    }
    else if (strcmp(this->mParametersVector[0].c_str(), "PARAMETER") == 0)
    {
        this->mWorkingMode = MODE_PARAMETER;
        ESP_LOGI(TAG, "PARAM detected\n");
        return ESP_OK;
    }

    this->mWorkingMode = MODE_INVALID;
    ESP_LOGW(TAG, "INVALID command %s\n",mParametersVector[0].c_str());
    return ESP_ERR_INVALID_ARG;

}

int UsbPcInterface::getWorkingMode()
{
    return this->mWorkingMode;
}

vector<string> UsbPcInterface::getParametersFromPc()
{
    return this->mParametersVector;
}

void UsbPcInterface::printErrorMessageAndRestart(string error_string){

    send("ERROR %s\n",error_string.c_str());
    send("Press Ctrl-C to restart\n");
    while(1){
         vTaskDelete(NULL);
    }
}