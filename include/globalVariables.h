#ifndef GLOBALVARIABLES
#define GLOBALVARIABLES

#include <queue>          // std::queue
#include "esp_err.h"
#include "driver/spi_slave.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"
#include "freertos/task.h"

#include "dataStoring.h"
#include "movementXY.h"


#define MODUS_RUN 0
#define MODUS_MONITOR_TUNNEL_CURRENT 1


#define TXD_PIN (GPIO_NUM_33)
#define RXD_PIN (GPIO_NUM_32)


#define GPIO_MOSI_VSPI 23
#define GPIO_MISO_VSPI 19
#define GPIO_SCLK_VSPI 18
#define GPIO_CS_VSPI_DACX 26 //17
#define GPIO_CS_VSPI_DACY 16
#define GPIO_CS_VSPI_DACZ 5

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
extern TaskHandle_t handleTask;

extern spi_device_interface_config_t devcfgDacX;
extern spi_device_interface_config_t devcfgDacY;
extern spi_device_interface_config_t devcfgDacZ;

extern spi_device_handle_t handleDacX;
extern spi_device_handle_t handleDacY;
extern spi_device_handle_t handleDacZ;

//extern uint16_t currentXPos;
//extern uint16_t currentYPos;

extern uint16_t currentXDac;
extern uint16_t currentYDac;
extern uint16_t currentZDac;

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO GPIO_NUM_22               /*!< gpio number for I2C master clock */
//#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL
#define I2C_MASTER_SDA_IO GPIO_NUM_21               /*!< gpio number for I2C master data  */
//#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_NUM I2C_NUMBER(1)                /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 1000000                             /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0                           /*!< I2C master doesn't need buffer */

#define ADS_1115_ADDRESS_WRITE 0x90
#define ADS_1115_ADDRESS_READ  0x91 

#define ADC_VALUE_MAX 32767 //2^15-1
#define ADC_VOLTAGE_MAX 2.048 //Voltage from 0 to 2.048V mapped to 0 to 32767

#define RESISTOR_PREAMP_MOHM 100 //100 MOhm Preamp resistor

#define DAC_VALUE_MAX 65535 //2^16-1
extern i2c_config_t i2cConf;


extern double kI, kP, destinationTunnelCurrentnA, currentTunnelCurrentnA, remainingTunnelCurrentDifferencenA;
extern uint16_t startX, startY;
extern bool direction;
extern uint16_t sendDataAfterXDatasets;

extern std::queue<dataElement> dataQueue;
extern scanGrid rtmGrid;


extern bool configNeeded, rtmDataReady;
extern uint16_t configExisting; //each bit stands for one config param. specifier 2 will write to second byte 
extern uint16_t lastConfigExisting;


extern uint16_t maxNumberAttemptsSPI;

extern intr_handle_t s_timer_handle;
extern uint8_t lastConfigByte;

#define MODE_INVALID -1
#define MODE_MONITOR_TUNNEL_CURRENT 1
#define MODE_MEASURE 2
#define MODE_PARAMETER 3

#define BLUE_LED GPIO_NUM_2

#endif
