#include <UsbPcInterface.h>

// https://github.com/espressif/esp-idf/blob/30e8f19f5ac158fc57e80ff97c62b6cc315aa337/examples/peripherals/uart/uart_async_rxtxtasks/main/uart_async_rxtxtasks_main.c

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
        .rx_flow_ctrl_thresh = 0, // !!! PeDi Added to avoid Warning -Wmissing-field-initializers
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
extern "C" void UsbPcInterface::mUartRcvLoop(void *unused)
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

                if (data[i] == 0xA) // Linefeed
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
                ESP_LOGI(TAG, "#### 0A found.  \n");

                string part3 = rcvString.substr(0, 3);
                for (int x = 0; x < strlen(part3.c_str()); x++)
                    part3[x] = toupper(part3[x]);
                ESP_LOGI(TAG, "NACHHER2  %s", part3.c_str());

                if (strcmp(part3.c_str(), "TIP") == 0)
                {
                    UsbPcInterface::mUpdateTip(rcvString);
                    rcvString.clear();
                    found_CR = false;
                }
                else
                {
                    UsbPcInterface::mUsbReceiveString.clear();
                    UsbPcInterface::mUsbReceiveString.append(rcvString);
                    UsbPcInterface::mUsbAvailable = true;
                    rcvString.clear();
                    found_CR = false;
                    ESP_LOGI(TAG, "usbReceive %s\n", mUsbReceiveString.c_str());
                }
            }
        }
    }
}
// free(data);

int UsbPcInterface::send(const char *fmt, ...)
{

    va_list ap;
    va_start(ap, fmt);

    char s[100] = {0};
    vsprintf(s, fmt, ap);

    const int len = strlen(s);
    int rc = uart_write_bytes(UART_NUM_1, s, len);

    va_end(ap);

    return rc;
}

int UsbPcInterface::sendParameter(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char s[100] = {0};
    vsprintf(s, fmt, ap);

    ESP_LOGI(TAG, "uartsend %s\n", s);
    string s1 = "PARAMETER,";

    int len = strlen(s);
    s1.append(s, len);
    len = strlen(s1.c_str());
    int rc = uart_write_bytes(UART_NUM_1, s1.c_str(), len);
    va_end(ap);
    return rc;
}

esp_err_t UsbPcInterface::sendData()
{
    // replaces hspiLoop
    // vTaskSuspend(handleControllerLoop);
    while (!dataQueue.empty())
    {
        dataQueue.front();
        uint16_t X = dataQueue.front().getDataX();
        uint16_t Y = dataQueue.front().getDataY();
        uint16_t Z = dataQueue.front().getDataZ();
        send("DATA,%d,%d,%d\r", X, Y, Z);
        dataQueue.pop();
    }
    send("DATA,COMPLETE\n");

    return ESP_OK;
}

esp_err_t UsbPcInterface::mUpdateTip(string s)
{

    int l = strlen(s.c_str());
    ESP_LOGI(TAG, "length TIP command: %d", l);
    if ((l < 5) || s[3] != ',')
    {
        ESP_LOGW("mUpdateTip", "No valid parameter for TIP. Needs 'TIP,integer'. Got %s\n", s.c_str());
        UsbPcInterface::send("No valid parameter for TIP. Needs 'TIP,integer'\n");
        return ESP_ERR_INVALID_ARG;
    }

    if (s[3] != ',')
    {
        ESP_LOGW("mUpdateTip", "INVALID command. TIP needs format 'TIP,integer'. Received %s\n", s.c_str());
    }
    ESP_LOGI("mUpdateTip", "TIP detected found.  \n");
    string v = s.substr(4, strlen(s.c_str()) - 4);

    int i;
    ESP_LOGI("mUpdateTip", "TIP detected value:%s\n", v.c_str());

    char *endPtr;
    long int il = strtol(v.c_str(), &endPtr, 10);
    if (strlen(endPtr) > 0)
    {
        ESP_LOGW("mUpdateTip", "INVALID command. TIP,1 is no number %s\n", s.c_str());

        return ESP_ERR_INVALID_ARG;
    }
    i = (int16_t)il;

    int newZ = currentZDac + i;
    if (newZ < 0)
        newZ = 0;
    if (newZ > DAC_VALUE_MAX)
        newZ = DAC_VALUE_MAX;

    UsbPcInterface::send("TIP,%d,%d\n", currentZDac, newZ);
    ESP_LOGI("mUpdateTip", "TIP detected TIP,%d,%d\n", currentZDac, newZ);
    currentZDac = (uint16_t)newZ;
    vTaskResume(handleVspiLoop); // realize newZ. Will suspend itself
    return ESP_OK;
}

/**
 * @brief Read USB input from Computer.
 * Set workingMode to: ADJUST or PARAMETER or MEASURE.
 * getworkingMode() reads workingMode
 */
extern "C" esp_err_t UsbPcInterface::getCommandsFromPC()
{

    esp_log_level_set("*", ESP_LOG_INFO);

    uint32_t i = 0;
    int ledLevel = 0;
    // Request PC. Wait for PC response
    while (UsbPcInterface::mUsbAvailable == false)
    {
        if ((i % 50) == 0)
        {
            // Invert Blue LED
            ledLevel++;
            gpio_set_level(BLUE_LED, ledLevel % 2);
            if (this->getWorkingMode() != MODE_IDLE)
                this->send("IDLE\n");
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
    numberOfValues = 1;
    for (int i = 0; i < UsbPcInterface::mUsbReceiveString.length(); i++)
        if (cstr[i] == ',')
            numberOfValues++;

    ESP_LOGI(TAG, "numberOfValues %d", numberOfValues);

    char *p = strtok(cstr, ",");

    this->mParametersVector.clear();
    while (p != 0)
    {
        // printf("%s\n", p);
        char buffer[20];
        sprintf(buffer, "%s", p);
        puts(strupr(buffer));
        this->mParametersVector.push_back(buffer);
        p = strtok(NULL, ",");
    }
    free(cstr);

    ESP_LOGI(TAG, "ParametersVector[0]: %s", this->mParametersVector[0].c_str());

    if (strcmp(this->mParametersVector[0].c_str(), "ADJUST") == 0)
    {
        ESP_LOGI(TAG, "ADJUST detected");
        this->mWorkingMode = MODE_ADJUST_TEST_TIP;
        ESP_LOGI(TAG, "ADJUST detected\n");
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
        ESP_LOGI(TAG, "PARAMETER detected\n");
        return ESP_OK;
    }

    this->mWorkingMode = MODE_INVALID;
    ESP_LOGW(TAG, "INVALID command %s\n", mParametersVector[0].c_str());
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

void UsbPcInterface::printErrorMessageAndRestart(string error_string)
{

    send("ERROR %s\n", error_string.c_str());
    send("Press Ctrl-C to restart\n");
    while (1)
    {
        vTaskDelete(NULL);
    }
}

void UsbPcInterface::printMessageAndRestart(string msg)
{
    send("%s\n", msg.c_str());
    send("Press Ctrl-C to restart\n");
    while (1)
    {
        vTaskDelete(NULL);
    }
}