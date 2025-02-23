#ifndef GLOBALVARIABLES
#define GLOBALVARIABLES

using namespace std;
#include <queue>
#include <mutex>
#include "esp_err.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "freertos/task.h"

#include "dataStoring.h"
#include "movementXY.h"
#include "driver/gpio.h"

#include <string>

// Additional USB Interface
#define TXD_PIN (GPIO_NUM_33)
#define RXD_PIN (GPIO_NUM_32)
#define BAUDRATE 460800 // 115200 // 460800

// SPI for DAC X Y Z
#define GPIO_MOSI_VSPI 23
#define GPIO_MISO_VSPI 19
#define GPIO_SCLK_VSPI 18
#define GPIO_CS_VSPI_DACX 26 // 17
#define GPIO_CS_VSPI_DACY 16
#define GPIO_CS_VSPI_DACZ 5

// Additional ports for monitoring/debugging

#define DMA_CHAN_VSPI 2

extern esp_err_t retVspi;
extern spi_transaction_t tVspi;

extern WORD_ALIGNED_ATTR char sendbufferVspi[3];
extern WORD_ALIGNED_ATTR char recvbufferVspi[3];

extern TaskHandle_t handleUartLoop;
extern TaskHandle_t handleUartRcvLoop;

extern TaskHandle_t handleVspiLoop;
extern TaskHandle_t handleSendDatasets;
extern TaskHandle_t handleMeasureLoop;
extern TaskHandle_t handleTunnelLoop;
extern TaskHandle_t handleDataTransmissionLoop;
extern TaskHandle_t handleAdjustLoop;
extern TaskHandle_t handleSinusLoop;
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

#define ADC_VALUE_MAX 32767   // 2^15-1
#define ADC_VOLTAGE_MAX 2.048 // Voltage from 0 to 2.048V mapped to 0 to 32767
#define ADC_VOLTAGE_DIVIDER 3

#define RESISTOR_PREAMP_MOHM 100 // 100 MOhm Preamp resistor

#define DAC_VALUE_MAX 65535 // 2^16-1
extern uint16_t targetTunnelAdc, toleranceTunnelAdc;
extern double kP,
    kI, kD, targetTunnelnA, currentTunnelnA, toleranceTunnelnA;
extern uint16_t startX, startY, multimultiplicatorGridAdc, measureMs;
extern uint16_t nvs_maxX, nvs_maxY;
extern bool direction;
extern uint16_t sendDataAfterXDatasets;

// Declare the queue handle as an extern variable
extern QueueHandle_t queueToPc;

extern scanGrid rtmGrid;

extern uint16_t configExisting; // each bit stands for one config param. specifier 2 will write to second byte
extern int64_t controller_start_time;

extern uint16_t maxNumberAttemptsSPI;

extern intr_handle_t s_timer_handle;

#define MEASURE_TIMER_MS 1

#define IO_02 GPIO_NUM_2
#define IO_04 GPIO_NUM_4
#define IO_27 GPIO_NUM_27
#define IO_25 GPIO_NUM_25
#define IO_17 GPIO_NUM_17

// Declare the global adjustIsActive variable
extern bool adjustIsActive;
extern bool measureIsActive;
extern bool tunnelIsActive;
extern bool sinusIsActive;
extern bool dataTransmissionIsActive;

// Declare the queue handle
extern QueueHandle_t queueFromPc;

#endif // GLOBALVARIABLES
