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

static const char *TAG = "UsbPcInterface";

static const char *TIP_ERROR_MESSAGE = "Invalid format 'TIP' command. \nSend 'TIP,10000,20000,30000' to set X,Y,Z\n'TIP,?' to see actual X Y Z values\n";

// Declare the queue handle
extern QueueHandle_t queueFromPc;

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
        // TODO higher baudrate ? HTERM can e.g. 256000
        .baud_rate = BAUDRATE,
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
        queueFromPc = xQueueCreate(10, sizeof(std::string));
        ESP_LOGI(TAG, "uartQueue started");
    }

    std::string buffer = "";
    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    ESP_LOGI(TAG, "Start mUartRcvLoop");

    while (1)
    {
        // Read data from UART
        const int rxCount = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);

        if (rxCount > 0)
        {
            data[rxCount] = 0; // Null-terminate the received data
            buffer.append((char *)data);

            size_t pos = 0;

            while ((pos = buffer.find_first_of("\n\r")) != std::string::npos)
            {
                std::string line = buffer.substr(0, pos);
                buffer.erase(0, buffer.find_first_not_of("\n\r", pos + 1));

                // Handle special case: If 0x3 (CTRL C) is received, restart
                if (line.find('\x03') != std::string::npos)
                {
                    esp_restart();
                }

                // Convert the entire line to uppercase
                std::transform(line.begin(), line.end(), line.begin(), ::toupper);

                // Send the received line to the queue
                if (xQueueSend(queueFromPc, &line, portMAX_DELAY) != pdPASS)
                {
                    ESP_LOGE(TAG, "Failed to send to queue");
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

int UsbPcInterface::sendParameter(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char s[100] = {0};
    vsprintf(s, fmt, ap);

    ESP_LOGD(TAG, "uartsend %s\n", s);
    std::string s1 = "PARAMETER,";

    int len = strlen(s);
    s1.append(s, len);
    len = strlen(s1.c_str());
    int rc = uart_write_bytes(UART_NUM_1, s1.c_str(), len);
    va_end(ap);
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

    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Check if the command starts with "TIP,"
    if (s.rfind("TIP,", 0) != 0)
    {
        UsbPcInterface::send(TIP_ERROR_MESSAGE);
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
        UsbPcInterface::send(TIP_ERROR_MESSAGE);
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

    ESP_LOGI(TAG, "TIP command received with values: X=%d, Y=%d, Z=%d", x, y, z);

    // Set X Y Z Tip Values
    currentXDac = x;
    currentYDac = y;
    currentZDac = z;
    vTaskResume(handleVspiLoop); // realize X Y Z. Will suspend itself
    return ESP_OK;
}
