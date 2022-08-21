#include <queue>
#include "globalVariables.h"
#include "driver/spi_master.h"
#include "driver/i2c.h"

esp_err_t retHspi;
spi_slave_transaction_t tHspi;
spi_slave_transaction_t tPreviousHspi;

WORD_ALIGNED_ATTR char sendbufferHspi[10]="";
WORD_ALIGNED_ATTR char recvbufferHspi[10]="";

WORD_ALIGNED_ATTR char oldSendbufferHspi[10]="";
WORD_ALIGNED_ATTR char oldRecvbufferHspi[10]="";


WORD_ALIGNED_ATTR char sendbufferUart[10]="";
WORD_ALIGNED_ATTR char recvbufferUart[10]="";


esp_err_t retVspi;
spi_transaction_t tVspi;

WORD_ALIGNED_ATTR char sendbufferVspi[3]="";
WORD_ALIGNED_ATTR char recvbufferVspi[3]="";

TaskHandle_t  handleHspiLoop = NULL;
TaskHandle_t  handleUartLoop = NULL;
TaskHandle_t  handleUartRcvLoop = NULL;
TaskHandle_t  handleVspiLoop = NULL;
TaskHandle_t  handleSendDatasets = NULL;
TaskHandle_t  handleControllerLoop = NULL;
TaskHandle_t  handleTask = NULL;


spi_device_interface_config_t devcfgDacX;
spi_device_interface_config_t devcfgDacY;
spi_device_interface_config_t devcfgDacZ;

spi_device_handle_t handleDacX;
spi_device_handle_t handleDacY;
spi_device_handle_t handleDacZ;

uint16_t currentXPos;
uint16_t currentYPos;

uint16_t currentXDac;
uint16_t currentYDac;
uint16_t currentZDac;

i2c_config_t i2cConf;


double kI, kP, destinationTunnelCurrentnA, currentTunnelCurrentnA, remainingTunnelCurrentDifferencenA = 0;
uint16_t startX, startY = 0;
bool direction = 0;
uint16_t sendDataAfterXDatasets = 100;

std::queue<dataElement> dataQueue;
scanGrid rtmGrid(200,200); //default grid

bool configNeeded = true;
uint8_t lastConfigByte=100;

/**
 * @brief Kompletter Messdatensatz verfügbar in queue. Wird von controller gesetzt
 */
bool rtmDataReady = false;
uint16_t configExisting = 0;
uint16_t lastConfigExisting = 1000;

uint16_t maxNumberAttemptsSPI = 10;

intr_handle_t s_timer_handle;
