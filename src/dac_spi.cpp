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
#include "spi_flash_mmap.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "globalVariables.h"
#include "dac_spi.h"
#include "controller.h"

static const char *TAG = "spi.cpp";
using namespace std;

/**@brief Start Init spi for DACs and run one vspiDacLoop
 *
 */
void vspiDacStart()
{

    vspiDacInit();
    xTaskCreatePinnedToCore(vspiDacLoop, "vspiDacLoop", 10000, NULL, 3, &handleVspiLoop, 1);
}

/**@brief Init spi for DACX DACY and DACZ
 *
 */
void vspiDacInit()
{
    // Connection to DACs
    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI_VSPI,
        .miso_io_num = GPIO_MISO_VSPI,
        .sclk_io_num = GPIO_SCLK_VSPI,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = 0,    // PeDi Added
        .data5_io_num = 0,    // PeDi Added
        .data6_io_num = 0,    // PeDi Added
        .data7_io_num = 0,    // PeDi Added
        .max_transfer_sz = 0, // PeDi Added
        .flags = 0,           // PeDi Added
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0, // PeDi Added

    };

    // Configuration for the SPI slave devices -> syntax configuration
    spi_device_interface_config_t devcfgDacX = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // CPOL = 0, CPHA = 0 -> Mode 0
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,     // 50% duty cycle
        .cs_ena_pretrans = 0,      // PeDi added
        .cs_ena_posttrans = 3,     // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = 5000000, // 5MHz
        .input_delay_ns = 0,       // PeDi added
        .spics_io_num = GPIO_CS_VSPI_DACX,
        .flags = 0, // PeDi added
        .queue_size = 3,
        .pre_cb = 0,   // PeDi added
        .post_cb = 0}; // PeDi added

    spi_device_interface_config_t devcfgDacY = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // CPOL = 0, CPHA = 0 -> Mode 0
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,     // 50% duty cycle
        .cs_ena_pretrans = 0,      // PeDi added
        .cs_ena_posttrans = 3,     // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = 5000000, // 5MHz
        .input_delay_ns = 0,       // PeDi added
        .spics_io_num = GPIO_CS_VSPI_DACY,
        .flags = 0, // PeDi added
        .queue_size = 3,
        .pre_cb = 0,   // PeDi added
        .post_cb = 0}; // PeDi added

    spi_device_interface_config_t devcfgDacZ = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0, // CPOL = 0, CPHA = 0 -> Mode 0
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,     // 50% duty cycle
        .cs_ena_pretrans = 0,      // PeDi added
        .cs_ena_posttrans = 3,     // Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .clock_speed_hz = 5000000, // 5MHz
        .input_delay_ns = 0,       // PeDi added
        .spics_io_num = GPIO_CS_VSPI_DACZ,
        .flags = 0, // PeDi added
        .queue_size = 3,
        .pre_cb = 0,   // PeDi added
        .post_cb = 0}; // PeDi added

    // Configuration for the CS lines
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << GPIO_CS_VSPI_DACX) | (1 << GPIO_CS_VSPI_DACY) | (1 << GPIO_CS_VSPI_DACZ),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,     // PeDi added
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // PeDi added
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
}

void vspiDacLoop(void *unused)
{

    ESP_LOGD(TAG, "+++ vspiDacLoop started\n");

    unique_ptr<uint16_t> buffer = make_unique<uint16_t>();

    uint16_t lastXDac = currentXDac;
    uint16_t lastYDac = currentYDac;

    // Z Dac init position??
    vspiSendDac(0, buffer.get(), handleDacZ); // Dac Z

    vspiSendDac(currentXDac, buffer.get(), handleDacX); // Dac X
    vspiSendDac(currentYDac, buffer.get(), handleDacY); // Dac Y
    vTaskSuspend(NULL);                                 // will be resumed by controller

    // Resumed by Controller
    while (1)
    {

        if (currentXDac != lastXDac)
        {                                                       // only if new value has been written to currentXDac
            vspiSendDac(currentXDac, buffer.get(), handleDacX); // Dac X
            lastXDac = currentXDac;
        }
        if (currentYDac != lastYDac)
        {                                                       // only if new value has been written to currentYDac
            vspiSendDac(currentYDac, buffer.get(), handleDacY); // Dac Y
            lastYDac = currentYDac;
        }

        vspiSendDac(currentZDac, buffer.get(), handleDacZ); // Dac Z
       
        // TODO set PWM
        if (currentZDac == 0)
        {
            gpio_set_level(IO_17, 1); // white LED
        }
        else
        {
            gpio_set_level(IO_17, 0); // white LED
        }

        vTaskSuspend(NULL); // will be resumed by controller
    }
}

uint16_t endianSwap(uint16_t toBeConverted)
{
    return (((toBeConverted) >> 8) | (toBeConverted << 8));
}

/// @brief Set DAC with dacValue.
/// @param dacValue Output voltage DAC. Outpt-Voltage im mV = dacValue * 0.0625
/// @param tx_buffer
/// @param dacDeviceHandle Handle for DAC X or DAC Y or DAC Z
void vspiSendDac(uint16_t dacValue, uint16_t *tx_buffer, spi_device_handle_t dacDeviceHandle)
{
    *tx_buffer = endianSwap(dacValue); // Change byte orientation, ESP is using little endian, dac big endian
    tVspi.cmd = 0x30;                  // write and update command dac

    tVspi.length = 16; // 16 bit raw data, without command
    tVspi.tx_buffer = tx_buffer;
    tVspi.rx_buffer = recvbufferVspi;

    retVspi = spi_device_transmit(dacDeviceHandle, &tVspi);
}
