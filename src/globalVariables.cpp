#include <queue>
#include <string>
#include <driver/spi_master.h>
#include "globalVariables.h"

int TUNNEL_REQUEST = 0;
esp_err_t retVspi;
spi_transaction_t tVspi;

WORD_ALIGNED_ATTR char sendbufferVspi[3] = "";
WORD_ALIGNED_ATTR char recvbufferVspi[3] = "";

TaskHandle_t handleUartLoop = NULL;
TaskHandle_t handleUartRcvLoop = NULL;
TaskHandle_t handleVspiLoop = NULL;
TaskHandle_t handleSendDatasets = NULL;
TaskHandle_t handleMeasureLoop = NULL;
TaskHandle_t handleAdjustLoop = NULL;
TaskHandle_t handleSinusLoop = NULL;
TaskHandle_t handleDataTransmissionLoop = NULL;
TaskHandle_t handleTask = NULL;
TaskHandle_t handleTunnelLoop = NULL;

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
uint16_t targetTunnelAdc;
uint16_t toleranceTunnelAdc;
double kP, kI, kD, targetTunnelnA, toleranceTunnelnA, currentTunnelnA;
uint16_t startX, startY = 0, measureMs = 1;
uint16_t nvs_maxX, nvs_maxY = 0, multiplicatorGridAdc;
bool direction = 0;
uint16_t sendDataAfterXDatasets = 100;

// Define the queue handle
QueueHandle_t queueToPc;

scanGrid rtmGrid(200, 200); // default grid

uint16_t configExisting = 0;
uint16_t maxNumberAttemptsSPI = 10;

intr_handle_t s_timer_handle;
// Define the global adjustIsActive variable
bool adjustIsActive = false;
bool measureIsActive = false;
bool tunnelIsActive = false;
bool sinusIsActive = false;
bool dataTransmissionIsActive = false;

uint16_t modeWorking;
