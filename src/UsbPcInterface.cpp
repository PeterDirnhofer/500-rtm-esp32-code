#include <UsbPcInterface.h>
#include <globalVariables.h>
#include <helper_functions.h>
#include <controller.h>
#include <string>
#include <vector>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <sstream>
#include <algorithm>

// Define the queue handle
QueueHandle_t queueFromPc = NULL;

static char *TAG = "UsbPcInterface";

static const char *TIP_ERROR_MESSAGE = "Invalid format 'TIP' command. \nSend 'TIP,10000,20000,30000' to set X,Y,Z\n'TIP,?' to see actual X Y Z values\n";



UsbPcInterface::UsbPcInterface()
    : mTaskHandle(NULL), mStarted(false)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
}

UsbPcInterface::~UsbPcInterface()
{
}

void UsbPcInterface::start()
{
    const uart_config_t uart_config = {
        .baud_rate = BAUDRATE, // Ensure this matches the sender's baud rate
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
        .flags = 0, // Initialize the flags member to zero
    };

    // Install UART driver
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    xTaskCreatePinnedToCore(this->mUartRcvLoop, "uartRcvLoop", 10000, NULL, 4, &this->mTaskHandle, (BaseType_t)0);

    this->mStarted = true;
}



void UsbPcInterface::mUartRcvLoop(void *unused)
{
    if (queueFromPc == NULL)
    {
        // Create the queue with a size of 10 and a maximum string length of 255 characters
        queueFromPc = xQueueCreate(2, sizeof(char) * 255);
        ESP_LOGI(TAG, "uartQueue started");
    }

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    if (data == NULL)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for data buffer");
        vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "Start mUartRcvLoop");

    while (1)
    {
        // Read data from UART
        const int rxCount = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);

        if (rxCount > 0)
        {
            data[rxCount] = 0; // Null-terminate the received data
            ESP_LOGI(TAG, "AAA");
            // Find the newline character
            char *lineEnd = strchr((char *)data, '\n');
            if (lineEnd != nullptr)
            {
                // Calculate the length of the line
                size_t lineLength = lineEnd - (char *)data + 1; // Include the LF character

                // Ensure the string is null-terminated
                if (lineLength < 255)
                {
                    data[lineLength] = '\0';
                }
                else
                {
                    data[254] = '\0'; // Ensure null-termination if lineLength is exactly 255
                }

                ESP_LOGI(TAG, "Received data: %s", data);
              

                // Copy data to a local buffer before sending to the queue
                char localBuffer[255];
                strncpy(localBuffer, (char *)data, 255);
                localBuffer[254] = '\0'; // Ensure null-termination
                // Convert localBuffer to uppercase
                for (int i = 0; localBuffer[i] != '\0'; i++)
                {
                    localBuffer[i] = toupper(localBuffer[i]);
                }
                ESP_LOGI(TAG, "BBB");
                // Send the received line to the queue
                if (xQueueSend(queueFromPc, localBuffer, portMAX_DELAY) != pdPASS)
                {
                    ESP_LOGE(TAG, "Failed to send to queue");
                }
                else
                {
                    ESP_LOGI(TAG, "Sent to queue: %s", localBuffer);
                }
            }
        }
    }

    ESP_LOGI(TAG, "Quit mUartRcvLoop");
    free(data);
    vTaskDelete(NULL);
}

int UsbPcInterface::send(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char s[100] = {0};
    vsprintf(s, fmt, ap);

    const int len = strlen(s);
    int rc = uart_write_bytes(UART_NUM_1, s, len);

    va_end(ap);
    // Wait for completion (if uart_write_bytes is non-blocking)
    if (rc >= 0)
    {
        // Assume `uart_wait_tx_done` blocks until all bytes are sent.
        uart_wait_tx_done(UART_NUM_1, portMAX_DELAY);
    }

    return rc;
}

int16_t normToMaxMin(long int invalue)
{
    if (invalue < 0)
        return 0;

    if (invalue > DAC_VALUE_MAX)
        return DAC_VALUE_MAX;

    return (int16_t)invalue;
}

esp_err_t UsbPcInterface::mUpdateTip(std::string s)
{
    static char *TAG = "mUpdateTip";
    esp_log_level_set(TAG, ESP_LOG_INFO);


    // Check if the command starts with "TIP,"
    if (s.rfind("TIP,", 0) != 0)
    {
        
        UsbPcInterface::send("#1 %s", TIP_ERROR_MESSAGE);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Remove "TIP," prefix
    s = s.substr(4);
    
    // Split the remaining string by commas
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        tokens.push_back(token);
    }
    
    
    // Ensure there are exactly 3 values
    if (tokens.size() != 3)
    {
        UsbPcInterface::send("#2 need 3 values. %s", TIP_ERROR_MESSAGE);
        return ESP_ERR_INVALID_ARG;
    }

  
    // Convert the values to integers
    char *endPtr;
    long int xl = strtol(tokens[0].c_str(), &endPtr, 10);
    if (*endPtr != '\0')
    {
        UsbPcInterface::send("%sDetail: X value is not an integer\n", TIP_ERROR_MESSAGE);
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t x = normToMaxMin(xl);
    
    long int yl = strtol(tokens[1].c_str(), &endPtr, 10);
    if (*endPtr != '\0')
    {
        UsbPcInterface::send("%sDetail: Y value is not an integer\n", TIP_ERROR_MESSAGE);
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t y = normToMaxMin(yl);

    long int zl = strtol(tokens[2].c_str(), &endPtr, 10);
    if (*endPtr != '\0')
    {
        UsbPcInterface::send("%sDetail: Z value is not an integer\n", TIP_ERROR_MESSAGE);
        return ESP_ERR_INVALID_ARG;
    }
    uint16_t z = normToMaxMin(zl);




    // Set X Y Z Tip Values
    currentXDac = x;
    currentYDac = y;
    currentZDac = z;

    
    vTaskResume(handleVspiLoop); // realize X Y Z. Will suspend itself
    
    return ESP_OK;
}
