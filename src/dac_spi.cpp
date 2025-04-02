#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <memory>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include <lwip/sockets.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/igmp.h>

#include <esp_system.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <soc/rtc_periph.h>
#include <esp_log.h>
#include <spi_flash_mmap.h>
#include <driver/gpio.h>
#include <driver/spi_master.h>

#include "globalVariables.h"
#include "dac_spi.h"
#include "controller.h"

static const char *TAG = "spi.cpp";

// Initialize SPI for DACs and start the VSPI DAC loop
void vspiDacStart()
{
    vspiDacInit();
    xTaskCreatePinnedToCore(vspiDacLoop, "vspiDacLoop", 10000, NULL, 3, &handleVspiLoop, 1);
}

// Initialize SPI for DACX, DACY, and DACZ
void vspiDacInit()
{
    static const char *TAG = "vspiDacInit";
    esp_log_level_set(TAG, ESP_LOG_INFO);

    // Configuration for the SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI_VSPI,
        .miso_io_num = GPIO_MISO_VSPI,
        .sclk_io_num = GPIO_SCLK_VSPI,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = 0,
        .data5_io_num = 0,
        .data6_io_num = 0,
        .data7_io_num = 0,
        .max_transfer_sz = 0,
        .flags = 0,
        .isr_cpu_id = ESP_INTR_CPU_AFFINITY_AUTO,
        .intr_flags = 0,
    };

    // Configuration for the SPI slave devices
    spi_device_interface_config_t devcfgDacX = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 3,
        .clock_speed_hz = 5000000,
        .input_delay_ns = 0,
        .spics_io_num = GPIO_CS_VSPI_DACX,
        .flags = 0,
        .queue_size = 3,
        .pre_cb = 0,
        .post_cb = 0};

    spi_device_interface_config_t devcfgDacY = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 3,
        .clock_speed_hz = 5000000,
        .input_delay_ns = 0,
        .spics_io_num = GPIO_CS_VSPI_DACY,
        .flags = 0,
        .queue_size = 3,
        .pre_cb = 0,
        .post_cb = 0};

    spi_device_interface_config_t devcfgDacZ = {
        .command_bits = 8,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = 0,
        .clock_source = SPI_CLK_SRC_DEFAULT,
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 3,
        .clock_speed_hz = 5000000,
        .input_delay_ns = 0,
        .spics_io_num = GPIO_CS_VSPI_DACZ,
        .flags = 0,
        .queue_size = 3,
        .pre_cb = 0,
        .post_cb = 0};

    // Configuration for the CS lines
    gpio_config_t io_conf = {
        .pin_bit_mask = (1 << GPIO_CS_VSPI_DACX) | (1 << GPIO_CS_VSPI_DACY) | (1 << GPIO_CS_VSPI_DACZ),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    // Configure CS as output
    gpio_config(&io_conf);

    // Initialize SPI master interface
    retVspi = spi_bus_initialize(VSPI_HOST, &buscfg, DMA_CHAN_VSPI);
    assert(retVspi == ESP_OK);

    // Initialize SPI slaves
    retVspi = spi_bus_add_device(VSPI_HOST, &devcfgDacX, &handleDacX);
    assert(retVspi == ESP_OK);
    retVspi = spi_bus_add_device(VSPI_HOST, &devcfgDacY, &handleDacY);
    assert(retVspi == ESP_OK);
    retVspi = spi_bus_add_device(VSPI_HOST, &devcfgDacZ, &handleDacZ);
    assert(retVspi == ESP_OK);

    memset(recvbufferVspi, 0, 3);
    memset(&tVspi, 0, sizeof(tVspi));
    ESP_LOGI(TAG, "DAC X Y Z init ok");
}

// VSPI DAC loop task
void vspiDacLoop(void *unused)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_LOGD(TAG, "+++ vspiDacLoop started\n");

    std::unique_ptr<uint16_t> buffer = std::make_unique<uint16_t>();

    uint16_t lastXDac = DAC_VALUE_MAX;
    uint16_t lastYDac = DAC_VALUE_MAX;
    uint16_t lastZDac = DAC_VALUE_MAX;

    vspiSendDac(0, buffer.get(), handleDacZ);

    vspiSendDac(currentXDac, buffer.get(), handleDacX);
    vspiSendDac(currentYDac, buffer.get(), handleDacY);
    vTaskSuspend(NULL);

    while (1)
    {
        if (currentXDac != lastXDac)
        {
            vspiSendDac(currentXDac, buffer.get(), handleDacX);
            lastXDac = currentXDac;
        }
        if (currentYDac != lastYDac)
        {
            vspiSendDac(currentYDac, buffer.get(), handleDacY);
            lastYDac = currentYDac;
        }
        if (currentZDac != lastZDac)
        {
            vspiSendDac(currentZDac, buffer.get(), handleDacZ);
            lastZDac = currentZDac;

            ESP_LOGI(TAG, "FOO currentZDac: %d", currentZDac);
            // Set LED
            if (currentZDac == 0)
            {
                gpio_set_level(IO_17_DAC_NULL, 1);
            }
            else
            {
                gpio_set_level(IO_17_DAC_NULL, 0);
            }

            if (currentZDac >= DAC_VALUE_MAX)
            {
                gpio_set_level(IO_04_DAC_MAX, 1);
            }
            else
            {
                gpio_set_level(IO_04_DAC_MAX, 0);
            }
        }
        vTaskSuspend(NULL);
    }
}

// Swap endianness of a 16-bit value
uint16_t endianSwap(uint16_t toBeConverted)
{
    return (((toBeConverted) >> 8) | (toBeConverted << 8));
}

// Send DAC value via VSPI
void vspiSendDac(uint16_t dacValue, uint16_t *tx_buffer, spi_device_handle_t dacDeviceHandle)
{
    *tx_buffer = endianSwap(dacValue);
    tVspi.cmd = 0x30;
    tVspi.length = 16;
    tVspi.tx_buffer = tx_buffer;
    tVspi.rx_buffer = recvbufferVspi;

    retVspi = spi_device_transmit(dacDeviceHandle, &tVspi);
}
