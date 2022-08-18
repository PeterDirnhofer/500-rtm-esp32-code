#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/igmp.h"

#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "soc/rtc_periph.h"
#include "esp_log.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/timer.h"

#include "globalVariables.h"
#include "spi.h"
#include "communication.h"
#include "controller.h"
#include "timer.h"

#include "uart.h"

// Called after a transaction is queued and ready for pickup by master. We use this to set the handshake line high.
void my_post_setup_cb(spi_slave_transaction_t *trans)
{
    WRITE_PERI_REG(GPIO_OUT_W1TS_REG, (1 << GPIO_HANDSHAKE_HSPI));
}

// Called after transaction is sent/received. We use this to set the handshake line low.
void my_post_trans_cb(spi_slave_transaction_t *trans)
{
    WRITE_PERI_REG(GPIO_OUT_W1TC_REG, (1 << GPIO_HANDSHAKE_HSPI));
}

void hspiInit()
{

    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI_HSPI,
        .miso_io_num = GPIO_MISO_HSPI,
        .sclk_io_num = GPIO_SCLK_HSPI,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // Configuration for the SPI slave interface
    spi_slave_interface_config_t slvcfg = {
        .spics_io_num = GPIO_CS_HSPI,
        .flags = 0,
        .queue_size = 1,
        .mode = 0,
        //.post_setup_cb=my_post_setup_cb,
        //.post_trans_cb=my_post_trans_cb
    };

    // Configuration for the handshake line
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << GPIO_HANDSHAKE_HSPI),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE};

    // Configure handshake line as output
    gpio_config(&io_conf);
    // Enable pull-ups on SPI lines so we don't detect rogue pulses when no master is connected.
    gpio_set_pull_mode(GPIO_MOSI_HSPI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_SCLK_HSPI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_CS_HSPI, GPIO_PULLUP_ONLY);

    // idle handshake line
    gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);

    // Initialize SPI slave interface
    retHspi = spi_slave_initialize(HSPI_HOST, &buscfg, &slvcfg, DMA_CHAN_HSPI);
    assert(retHspi == ESP_OK);

    memset(recvbufferHspi, 0, 10);
    memset(&tHspi, 0, sizeof(tHspi));
}

void hspiStart()
{
    hspiInit();
    // Creates loop task for operation as slave on hspi
    // timer_tg0_initialise(1000000);
    // timer_start(TIMER_GROUP_0, TIMER_0);
    xTaskCreatePinnedToCore(hspiLoop, "hspiloop", 10000, NULL, 4, &handleHspiLoop, 0);
}

void hspiLoop(void *unused)
{
    uartSendData("HSPI Loop started\n");
    // printf("HSPI Loop started\n");
    bool dataReadyLastCycle = false;
    while (1)
    {

        // Clear receive buffer, set send buffer to something sane
        memset(recvbufferHspi, 0x00, 10);

        if (dataReadyLastCycle)
        {
            printf("suspend last cycle");
            // handshake line high
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);
            dataReadyLastCycle = false;
            vTaskResume(handleControllerLoop);
            vTaskSuspend(NULL);
        }

        // check if transaction is necessary or datasets are waiting for being sent, otherwise suspend and wait for
        // printf("config: %02x \n", configExisting);

        if ((configExisting == 0x1FF) && !dataReady)
        { // 0x1FF is 0b111111111 -> nine ones
            // handshake line high
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);
            configNeeded = false;
            printf("HSPI Loop suspend \n");
            printf("%f \n", kI);
            printf("%f \n", kP);
            printf("%f \n", destinationTunnelCurrentnA);
            printf("%f \n", remainingTunnelCurrentDifferencenA);
            printf("%d \n", rtmGrid.getCurrentX());
            printf("%d \n", rtmGrid.getCurrentY());
            printf("%d \n", direction);
            printf("%d \n", rtmGrid.getMaxX());
            printf("%d \n", rtmGrid.getMaxY());

            controllerStart();
            vTaskSuspend(NULL);
        }

        // transaction is necessary, set configbyte
        if (configNeeded)
        {
            sendbufferHspi[0] = (uint8_t)(1 << POSITION_ESP_READY_CONFIGFILE); // 0x04
        }
        else if (dataReady)
        {
            vTaskSuspend(handleControllerLoop);
            dataQueue.front();
            uint16_t X = dataQueue.front().getDataX();
            uint16_t Y = dataQueue.front().getDataY();
            uint16_t Z = dataQueue.front().getDataZ();
            printf("%d %d %d\n", X, Y, Z);

            dataQueue.pop(); // remove from queue

            // decide whether last dataset to send or not, effects endtag in configbyte
            if (dataQueue.empty())
            {
                dataReady = false;
                dataReadyLastCycle = true;
                // printf("Last dataset to send\n");
                sendbufferHspi[0] = (uint8_t)((1 << POSITION_ESP_SEND_DATA) | (1 << POSITION_ESP_ENDTAG)); // 0x11
            }
            else
            {
                sendbufferHspi[0] = (uint8_t)(1 << POSITION_ESP_SEND_DATA); // 0x10
            }
            memcpy(&sendbufferHspi[1], &X, 2);
            memcpy(&sendbufferHspi[3], &Y, 2);
            memcpy(&sendbufferHspi[5], &Z, 2);
        }

        // PeDi wieder deaktivieren
        // printf("send: ");
        // for(int i = 9; i >= tHspi.trans_len/8; i--){
        //    printf("%x", ((uint8_t*) tHspi.tx_buffer)[i]);
        //}
        // printf("\n");
        // END

        // Set up a transaction of 10 bytes to send/receive
        tHspi.length = 10 * 8;
        tHspi.tx_buffer = sendbufferHspi;
        tHspi.rx_buffer = recvbufferHspi;

        tPreviousHspi = tHspi;
        // spi_slave_transmit does not return until the master has done a transmission, so by here we have sent our data and
        // received data from the master. Print it.
        spiSend(HSPI_HOST, &tHspi, portMAX_DELAY, &tPreviousHspi);

        if ((!std::equal(std::begin(sendbufferHspi), std::end(sendbufferHspi), std::begin(oldSendbufferHspi))) ||
            (!std::equal(std::begin(recvbufferHspi), std::end(recvbufferHspi), std::begin(oldRecvbufferHspi))))
        {
            //uartSendData("Neu in spiSend: \n");
            char help[10];
            uartSendData("ESP Send ");
            //for (int i = 0; i < tHspi.trans_len / 8; i++)
            for (int i = 0; i < 10; i++)
            {
                sprintf(help, "%2X", ((uint8_t *)tHspi.tx_buffer)[i]);
                
                uartSendData(help);
            }
            uartSendData("\n");
            uartSendData("Received ");

            for (int i = 0; i < 80; i++)
            {
                sprintf(help, "%2x", ((uint8_t *)tHspi.rx_buffer)[i]);
                
                uartSendData(help);
            }
            uartSendData("\n");

            // printf("\n");
            // printf("Received %d bytes:  \n", tHspi.trans_len / 8);
            // printf("recv: ");
            // for (int i = 0; i < tHspi.trans_len / 8; i++)
            //{
            //     printf("%x", ((uint8_t *)tHspi.rx_buffer)[i]);
            // }
            // printf("\n");

            memcpy(oldSendbufferHspi, sendbufferHspi, 10);
            memcpy(oldRecvbufferHspi, recvbufferHspi, 10);
        }
    }
}

void vspiStart()
{
    printf("VSPI start \n");
    vspiInit();
    printf("start VSPI Loop \n");
    printf("vspiStart Core: %d \n", xPortGetCoreID());
    xTaskCreatePinnedToCore(vspiLoop, "vspiloop", 10000, NULL, 3, &handleVspiLoop, 1);
    printf("started VSPI Loop \n");
}

void vspiInit()
{
    printf("start VSPI INIT");
    // Connection to DACs
    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI_VSPI,
        .miso_io_num = GPIO_MISO_VSPI,
        .sclk_io_num = GPIO_SCLK_VSPI,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    // Configuration for the SPI slave devices -> syntax configuration
    spi_device_interface_config_t devcfgDacX = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,                 // CPOL = 0, CPHA = 0 -> Mode 0
        .duty_cycle_pos = 128,     // 50% duty cycle
        .cs_ena_posttrans = 3,     // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = 5000000, // 5MHz
        .spics_io_num = GPIO_CS_VSPI_DACX,
        .queue_size = 3};

    spi_device_interface_config_t devcfgDacY = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,                 // CPOL = 0, CPHA = 0 -> Mode 0
        .duty_cycle_pos = 128,     // 50% duty cycle
        .cs_ena_posttrans = 3,     // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = 5000000, // 5MHz
        .spics_io_num = GPIO_CS_VSPI_DACY,
        .queue_size = 3};

    spi_device_interface_config_t devcfgDacZ = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,                 // CPOL = 0, CPHA = 0 -> Mode 0
        .duty_cycle_pos = 128,     // 50% duty cycle
        .cs_ena_posttrans = 3,     // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = 5000000, // 5MHz
        .spics_io_num = GPIO_CS_VSPI_DACZ,
        .queue_size = 3};

    printf("test3 \n");
    // Configuration for the CS lines
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << GPIO_CS_VSPI_DACX) | (1 << GPIO_CS_VSPI_DACY) | (1 << GPIO_CS_VSPI_DACZ),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE};

    // Configure cs as output
    gpio_config(&io_conf);

    // Initialize SPI master interface
    retVspi = spi_bus_initialize(VSPI_HOST, &buscfg, DMA_CHAN_VSPI);
    assert(retVspi == ESP_OK);

    // init spi slaves
    retVspi = spi_bus_add_device(VSPI_HOST, &devcfgDacX, &handleDacX);
    assert(retVspi == ESP_OK);
    retVspi = spi_bus_add_device(VSPI_HOST, &devcfgDacY, &handleDacY);
    assert(retVspi == ESP_OK);
    retVspi = spi_bus_add_device(VSPI_HOST, &devcfgDacZ, &handleDacZ);
    assert(retVspi == ESP_OK);

    memset(recvbufferVspi, 0, 3);

    memset(&tVspi, 0, sizeof(tVspi));
    printf("VSPI INIT    ++++++++++++ OK\n");
}

void vspiLoop(void *unused)
{
    printf("++++++++++++++++++++++ vspiLoop Prio: %d \n", xPortGetCoreID());

    std::unique_ptr<uint16_t> buffer = std::make_unique<uint16_t>();

    uint16_t lastXDac = currentXDac;
    uint16_t lastYDac = currentYDac;

    // Z Dac init position??
    vspiSendDac(0, buffer.get(), handleDacZ); // Dac Z

    vspiSendDac(currentXDac, buffer.get(), handleDacX); // Dac X
    vspiSendDac(currentYDac, buffer.get(), handleDacY); // Dac Y
    vTaskSuspend(NULL);                                 // will be resumed by controller
    while (1)
    {
        // printf("vspiLoop");
        printf("X, new: %d, old: %d \n", currentXDac, lastXDac);

        if (currentXDac != lastXDac)
        {                                                       // only if new value has been written to currentXDac
            vspiSendDac(currentXDac, buffer.get(), handleDacX); // Dac X
            lastXDac = currentXDac;
            printf("new X \n");
        }
        if (currentYDac != lastYDac)
        {                                                       // only if new value has been written to currentYDac
            vspiSendDac(currentYDac, buffer.get(), handleDacY); // Dac Y
            lastYDac = currentYDac;
            printf("new Y \n");
        }

        vspiSendDac(currentZDac, buffer.get(), handleDacZ); // Dac Z
        printf("cccc Z: %d\n", currentZDac);
        vTaskSuspend(NULL); // will be resumed by controller
    }
}

uint16_t endianSwap(uint16_t toBeConverted)
{
    return (((toBeConverted) >> 8) | (toBeConverted << 8));
}

void vspiSendDac(uint16_t dacValue, uint16_t *tx_buffer, spi_device_handle_t dacDeviceHandle)
{
    *tx_buffer = endianSwap(dacValue); // Change byte orientation, ESP is using little endian, dac big endian
    tVspi.cmd = 0x30;                  // write and update command dac

    tVspi.length = 16; // 16 bit raw data, without command
    tVspi.tx_buffer = tx_buffer;
    tVspi.rx_buffer = recvbufferVspi;

    retVspi = spi_device_transmit(dacDeviceHandle, &tVspi);
}

esp_err_t spiSend(spi_host_device_t host, spi_slave_transaction_t *current, TickType_t ticks_to_wait, spi_slave_transaction_t *previous)
{
    /* send actual transaction */

    gpio_set_level(GPIO_HANDSHAKE_HSPI, 0);
    retHspi = spi_slave_transmit(host, current, ticks_to_wait);
    gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);

    std::unique_ptr<protocolElement> currentReceivedCommand = std::make_unique<protocolElement>((uint8_t *)current->rx_buffer); // create protocolElement, constructor will proceed

    bool previousConfigBad = currentReceivedCommand->getConfigBad();
    bool currentConfigBad = false;
    // resending previous transaction
    if (previousConfigBad)
    {
        std::unique_ptr<protocolElement> previousReceivedCommand;
        for (uint16_t attemptCounter = 0; attemptCounter <= maxNumberAttemptsSPI; attemptCounter++)
        {
            // handshake line low
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 0);
            retHspi = spi_slave_transmit(host, previous, ticks_to_wait);
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);

            previousReceivedCommand = std::make_unique<protocolElement>((uint8_t *)previous->rx_buffer); // create protocolElement, constructor will proceed
            if (attemptCounter == 0)
            {
                currentConfigBad = previousReceivedCommand->getConfigBad();
            }
            uint8_t recvBufferAnswer[10];
            spiSendEmpty(host, ticks_to_wait, recvBufferAnswer);
            previousReceivedCommand = std::make_unique<protocolElement>(recvBufferAnswer);
            if (!previousReceivedCommand->getConfigBad())
            { // if previous transaction has been transmitted succesfully
                break;
            }
            else if (attemptCounter == maxNumberAttemptsSPI)
            {
                printf("ERROR, cant transmit succesfully \n");
                return retHspi;
            }
        }
    }
    if (currentConfigBad)
    {
        std::unique_ptr<protocolElement> currentReceivedCommand;
        for (uint16_t attemptCounter = 0; attemptCounter <= maxNumberAttemptsSPI; attemptCounter++)
        {
            // handshake line low
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 0);
            retHspi = spi_slave_transmit(host, current, ticks_to_wait);
            gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);

            currentReceivedCommand = std::make_unique<protocolElement>((uint8_t *)current->rx_buffer); // create protocolElement, constructor will proceed

            uint8_t recvBufferAnswer[10];
            spiSendEmpty(host, ticks_to_wait, recvBufferAnswer);
            currentReceivedCommand = std::make_unique<protocolElement>(recvBufferAnswer);
            if (!currentReceivedCommand->getConfigBad())
            { // if previous transaction has been transmitted succesfully
                break;
            }
            else if (attemptCounter == maxNumberAttemptsSPI)
            {
                printf("ERROR, cant transmit succesfully \n");
                return retHspi;
            }
        }
    }
    return retHspi;
}

void spiSendEmpty(spi_host_device_t host, TickType_t ticks_to_wait, uint8_t *recvBuffer)
{
    uint8_t emptyBuffer[10];
    memset(emptyBuffer, 0x00, 10);
    memset(recvBuffer, 0x00, 10);

    spi_slave_transaction_t emptyTransaction;

    emptyTransaction.length = 10 * 8;
    emptyTransaction.tx_buffer = emptyBuffer;
    emptyTransaction.rx_buffer = recvBuffer;

    gpio_set_level(GPIO_HANDSHAKE_HSPI, 0);
    retHspi = spi_slave_transmit(host, &emptyTransaction, ticks_to_wait);
    gpio_set_level(GPIO_HANDSHAKE_HSPI, 1);
}