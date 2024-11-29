#include <UsbPcInterface.h>

static const char *TAG = "UsbPcInterface";
static const char *TIP_ERROR_MESSAGE = "Invalid format 'TIP' command. \nSend 'TIP,10000,20000,30000' to set X,Y,Z\n'TIP,?' to see actual X Y Z values\n";

UsbPcInterface::UsbPcInterface()
    : mTaskHandle(NULL), mStarted(false)
{
}

UsbPcInterface::~UsbPcInterface()
{
}

void UsbPcInterface::start()
{
    const uart_config_t uart_config = {
        // TODO higher baudrate ? HTERM can e.g. 256000
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

extern "C" void UsbPcInterface::mUartRcvLoop(void *unused)
{

    string rcvString = "";
    bool found_LF;

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);

    found_LF = false;
    while (1)
    {

        const int rxCount = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 100 / portTICK_PERIOD_MS);
        if (rxCount > 0)
        {
            data[rxCount] = 0;
            int i = 0;

            while ((i < rxCount) && (found_LF == false))
            {

                if (data[i] == 0xA) // Linefeed
                {
                    found_LF = true;
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

            if (found_LF)
            {
                string part3 = rcvString.substr(0, 3);
                for (int x = 0; x < strlen(part3.c_str()); x++)
                    part3[x] = toupper(part3[x]);

                if (strcmp(part3.c_str(), "TIP") == 0) // TIP command
                {
                    UsbPcInterface::mUpdateTip(rcvString);
                }
                else // other commands
                {
                    UsbPcInterface::mUsbReceiveString.clear();
                    UsbPcInterface::mUsbReceiveString.append(rcvString);
                    UsbPcInterface::mUsbAvailable = true;
                }
                rcvString.clear();
                found_LF = false;
            }
        }
    }
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

    return rc;
}

int UsbPcInterface::sendParameter(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char s[100] = {0};
    vsprintf(s, fmt, ap);

    ESP_LOGD(TAG, "uartsend %s\n", s);
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

    while (!dataQueue.empty())
    {
        // Store the front element in a variable
        DataElement &element = dataQueue.front();

        // Now use this element to access data
        uint16_t X = element.getDataX();
        uint16_t Y = element.getDataY();
        uint16_t Z = element.getDataZ();
        send("DATA,%u,u%,%u\r", X, Y, Z);
        dataQueue.pop();
    }
    send("DATA,COMPLETE\n");

    return ESP_OK;
}

int16_t normToMaxMin(long int invalue)
{
    if (invalue < 0)
        return 0;

    if (invalue > DAC_VALUE_MAX)
        return DAC_VALUE_MAX;

    return (int16_t)invalue;
}

/// @brief Set X Y Z TIP position or return actual X Y Z
/// @param s Input String 'TIP,X,Y,Z'  or 'TIP,?'. Strings X Y and Z are normalized to the range of '0' and '32767'
/// @return When received 'TIP,?' returns a string with actual X Y Z Position
esp_err_t UsbPcInterface::mUpdateTip(string s)
{

    char *cstr = new char[s.length() + 1];
    strcpy(cstr, s.c_str());

    vector<string> arguments;

    // how many comma are in string
    int numberOfValues = 1;
    for (int i = 0; i < UsbPcInterface::mUsbReceiveString.length(); i++)
        if (cstr[i] == ',')
            numberOfValues++;

    char *p = strtok(cstr, ",");

    arguments.clear();
    while (p != 0)
    {
        char buffer[20];
        sprintf(buffer, "%s", p);
        arguments.push_back(buffer);
        p = strtok(NULL, ",");
    }

    if (UsbPcInterface::adjustIsActive == false)
    {
        UsbPcInterface::send("No valid command. 'TIP' is only valid in ADJUST mode\n");
        return ESP_ERR_INVALID_ARG;
    }

    int l = arguments.size();

    if (l == 2) //  && (strcmp(arguments[1].c_str(), "ADJUST") == 0))
    {

        if (strcmp(arguments[1].c_str(), "?") == 0)
        {
            UsbPcInterface::send("TIP,%d,%d,%d\n", currentXDac, currentYDac, currentZDac);
            return ESP_OK;
        }
        else
        {
            UsbPcInterface::send(TIP_ERROR_MESSAGE);
            return ESP_ERR_INVALID_ARG;
        }
    }

    if (l != 4)
    {
        UsbPcInterface::send(TIP_ERROR_MESSAGE);
        return ESP_ERR_INVALID_ARG;
    }

    // TIP,X,Y,Z
    if (l == 4)
    {
        uint16_t x = 0;
        uint16_t y = 0;
        uint16_t z = 0;

        char *endPtr;
        long int xl = strtol(arguments[1].c_str(), &endPtr, 10);

        if (strlen(endPtr) > 0)
        {
            UsbPcInterface::send("%sDetail: X value is not an integer\n", TIP_ERROR_MESSAGE);
            return ESP_ERR_INVALID_ARG;
        }
        x = normToMaxMin(xl);

        long int yl = strtol(arguments[2].c_str(), &endPtr, 10);
        if (strlen(endPtr) > 0)
        {
            UsbPcInterface::send("%sDetail: Y value is not an integer\n", TIP_ERROR_MESSAGE);
            return ESP_ERR_INVALID_ARG;
        }
        y = normToMaxMin(yl);

        long int zl = strtol(arguments[3].c_str(), &endPtr, 10);
        if (strlen(endPtr) > 0)
        {
            UsbPcInterface::send("%sDetail: Z value is not an integer\n", TIP_ERROR_MESSAGE);
            return ESP_ERR_INVALID_ARG;
        }
        z = normToMaxMin(zl);

        // Set X Y Z Tip Values
        currentXDac = x;
        currentYDac = y;
        currentZDac = z;
        vTaskResume(handleVspiLoop); // realize X Y Z. Will suspend itself

        return ESP_OK;
    }

    UsbPcInterface::send("TIP_ERROR_MESSAGE");
    return ESP_ERR_INVALID_ARG;
}

extern "C" esp_err_t UsbPcInterface::getCommandsFromPC()
{

    static const char *TAG = "UsbPcInterface::getCommandsFromPC";

    uint32_t i = 0;
    int ledLevel = 0;
    bool idle_was_sent = false;

    while (UsbPcInterface::mUsbAvailable == false)
    {
        if (this->getWorkingMode() == MODE_IDLE)
        {

            if (!idle_was_sent)
            {
                this->send("IDLE\n");
                idle_was_sent = true;
            }

            if ((i % 50) == 0)
            {
                ledLevel = !ledLevel;
                gpio_set_level(IO_27, ledLevel);
            }
        }
        else{
            idle_was_sent = false;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
        i++;
    }

    char *cstr = new char[UsbPcInterface::mUsbReceiveString.length() + 1];
    strcpy(cstr, UsbPcInterface::mUsbReceiveString.c_str());

    numberOfValues = 1;
    for (int i = 0; i < UsbPcInterface::mUsbReceiveString.length(); i++)
        if (cstr[i] == ',')
            numberOfValues++;

    char *p = strtok(cstr, ",");

    this->mParametersVector.clear();

    while (p != nullptr)
    {
        // Convert the token to uppercase and store it in the vector
        this->mParametersVector.push_back(toUpper(p));
        p = strtok(nullptr, ",");
    }

    if (strcmp(this->mParametersVector[0].c_str(), "ADJUST") == 0)
    {

        UsbPcInterface::m_workingmode = MODE_ADJUST_TEST_TIP;
        UsbPcInterface::adjustIsActive = true;
        ESP_LOGI(TAG, "ADJUST detected\n");
        return ESP_OK;
    }
    else if (strcmp(this->mParametersVector[0].c_str(), "MEASURE") == 0)
    {

        UsbPcInterface::m_workingmode = MODE_MEASURE;
        ESP_LOGI(TAG, "MEASURE detected\n");
        return ESP_OK;
    }
    else if (strcmp(this->mParametersVector[0].c_str(), "PARAMETER") == 0)
    {

        UsbPcInterface::m_workingmode = MODE_PARAMETER;
        ESP_LOGI(TAG, "PARAMETER detected\n");
        return ESP_OK;
    }

    else if (strcmp(this->mParametersVector[0].c_str(), "TUNNEL") == 0)
    {

        UsbPcInterface::m_workingmode = MODE_TUNNEL_FIND;
        ESP_LOGI(TAG, "TUNNEL_FIND detected\n");
        return ESP_OK;
    }
    else if (strcmp(this->mParametersVector[0].c_str(), "RESTART") == 0)
    {

        UsbPcInterface::m_workingmode = MODE_RESTART;
        ESP_LOGI(TAG, "RESTART detected\n");
        return ESP_OK;
    }

    UsbPcInterface::m_workingmode = MODE_INVALID;
    ESP_LOGW(TAG, "INVALID command %s\n", mParametersVector[0].c_str());
    return ESP_ERR_INVALID_ARG;
}

// Function to convert a C-string to uppercase
string UsbPcInterface::toUpper(const char *str)
{
    std::string result;
    while (*str)
    {
        result += static_cast<char>(std::toupper(*str));
        ++str;
    }
    return result;
}

int UsbPcInterface::getWorkingMode()
{
    return UsbPcInterface::m_workingmode;
}

vector<string> UsbPcInterface::getParametersFromPc()
{
    return this->mParametersVector;
}

vector<string> s_getParametersFromPC()
{
    vector<string> hs;

    hs.push_back("EINS");

    return hs;
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
