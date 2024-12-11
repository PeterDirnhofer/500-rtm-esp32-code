#ifndef GLOBALVARIABLES
#define GLOBALVARIABLES

using namespace std;
#include <queue>
#include "esp_err.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "freertos/task.h"

#include "dataStoring.h"
#include "movementXY.h"

extern int ACTMODE;
extern int TUNNEL_REQUEST;

struct PIResult
{
    double targetNa;
    double currentNa;
    double error;
    uint16_t dacz;
};

#define MODUS_RUN 0
#define MODUS_MONITOR_TUNNEL_CURRENT 1

// Additional USB Interface
#define TXD_PIN (GPIO_NUM_33)
#define RXD_PIN (GPIO_NUM_32)

// SPI for DAC X Y Z
#define GPIO_MOSI_VSPI 23
#define GPIO_MISO_VSPI 19
#define GPIO_SCLK_VSPI 18
#define GPIO_CS_VSPI_DACX 26 // 17
#define GPIO_CS_VSPI_DACY 16
#define GPIO_CS_VSPI_DACZ 5

// Additional ports for monitoring/debugging
#define IO_02 GPIO_NUM_2
#define IO_04 GPIO_NUM_4
#define IO_17 GPIO_NUM_17
#define IO_25 GPIO_NUM_25
#define IO_27 GPIO_NUM_27
#define IO_35 GPIO_NUM_35

#define DMA_CHAN_VSPI 2

extern esp_err_t retVspi;
extern spi_transaction_t tVspi;

extern WORD_ALIGNED_ATTR char sendbufferVspi[3];
extern WORD_ALIGNED_ATTR char recvbufferVspi[3];

extern TaskHandle_t handleUartLoop;
extern TaskHandle_t handleUartRcvLoop;

extern TaskHandle_t handleVspiLoop;
extern TaskHandle_t handleSendDatasets;
extern TaskHandle_t handleControllerLoop;
extern TaskHandle_t handleTunnelLoop;
extern TaskHandle_t handleAdjustLoop;
extern TaskHandle_t handleTask;

extern spi_device_interface_config_t devcfgDacX;
extern spi_device_interface_config_t devcfgDacY;
extern spi_device_interface_config_t devcfgDacZ;

extern spi_device_handle_t handleDacX;
extern spi_device_handle_t handleDacY;
extern spi_device_handle_t handleDacZ;

extern uint16_t currentXDac;
extern uint16_t currentYDac;
extern uint16_t currentZDac;
// extern uint16_t multimultiplicatorGridAdc;

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

// I2C for ADC
#define I2C_PORT_NUM_0 0
#define I2C_DEVICE_ADDRESS 0x48

#define I2C_MASTER_SCL_IO GPIO_NUM_22 /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21 /*!< gpio number for I2C master data  */

#define I2C_MASTER_NUM I2C_NUMBER(1) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 1000000   /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0  /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0  /*!< I2C master doesn't need buffer */

#define CONFIG_REGISTER 0x01
#define THRESHOLD_LO_REGISTER 0x02
#define THRESHOLD_HI_REGISTER 0x03
#define CONVERSION_REGISTER 0x00

// #define ADS_1115_ADDRESS_WRITE 0x90
// #define ADS_1115_ADDRESS_READ 0x91

#define ADC_VALUE_MAX 32767   // 2^15-1
#define ADC_VOLTAGE_MAX 2.048 // Voltage from 0 to 2.048V mapped to 0 to 32767

#define RESISTOR_PREAMP_MOHM 100    // 100 MOhm Preamp resistor
#define ADC_VOLTAGE_DIVIDER 0.33333 // Voltage divider at ADC Input

#define DAC_VALUE_MAX 65535 // 2^16-1

extern double kP,
    kI, kD, targetTunnelnA, currentTunnelnA, toleranceTunnelnA;
extern uint16_t startX, startY, multimultiplicatorGridAdc;
extern uint16_t nvs_maxX, nvs_maxY;
extern bool direction;
extern uint16_t sendDataAfterXDatasets;

extern queue<DataElement> dataQueue;
// extern queue<string> tunnelQueue;

extern scanGrid rtmGrid;

extern bool configNeeded;
extern uint16_t configExisting; // each bit stands for one config param. specifier 2 will write to second byte
extern uint16_t lastConfigExisting;
extern int64_t controller_start_time;

extern uint16_t maxNumberAttemptsSPI;

extern intr_handle_t s_timer_handle;
extern uint8_t lastConfigByte;

#define MODE_IDLE 0
#define MODE_INVALID -1
#define MODE_ADJUST_TEST_TIP 1
#define MODE_MEASURE 2
#define MODE_PARAMETER 3
#define MODE_TUNNEL_FIND 4
#define MODE_RESTART 5 // starts esp_restart();

#define TUNNEL_TIMER_MS 1 // Period time in TUNNEL find loop
#define MEASURE_TIMER_MS 1



#endif
