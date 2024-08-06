#include "globalVariables.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c_master.h"
#include "driver/i2c_slave.h"

#define TAG "ADC"
i2c_master_dev_handle_t dev_handle;

uint16_t readAdc()
{

    uint8_t data_rd[2];

    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, data_rd, 2, -1));
    uint16_t temp = data_rd[1] << 8 | data_rd[0];

    // uint8_t dataH = 0, dataL = 0;

    // uint16_t temp = dataH << 8 | dataL;

    return temp;
}

esp_err_t i2cAdcInit()
{
    // INIT ADC
    //  Config ADS 1115 Single
    //  Bits 15 14 13 12  11 10 09 08
    //        0  1  1  1   0  1  0  0
    //       15 Single Conversion
    //           1  1  1  Measure Comparator A3 against GND
    //                     0  1  0  gain +- 2.048 V
    //                              0 Continuous mode, no reset after conversion

    // Bits 07 06 05 04  03 02 01 00
    //       1  1  1  0   0  0  1  1
    //       1  1  1  Datarate 860 samples per second  (ACHTUNG passt das in ms takt?)
    //                0 Comparator traditional (unwichtig, nur für Threshold)
    //                    0 Alert/Rdy Active low
    //                        0  No latch comparator
    //                           1  1  Disable coparator

    // Define I2C master bus configuration
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_PORT_NUM_0,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true},
    };
    i2c_master_bus_handle_t bus_handle;
    // Create I2C master bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // Define I2C device configuration
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = I2C_DEVICE_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    // Add device to I2C master bus
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // Set Configuration Register of ADS1115
    uint8_t data_wr[3];
    data_wr[0] = CONFIG_REGISTER;
    data_wr[1] = 0x74;
    data_wr[2] = 0xE3;

    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_wr, 3, -1));

    // Set lo threshold Register of ADS1115
    data_wr[0] = THRESHOLD_LO_REGISTER;
    data_wr[1] = 0x00; // bits 15 to 8
    data_wr[2] = 0x00; // bits 7 to 0

    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_wr, 3, -1));

    // Set hi threshold Register of ADS1115
    data_wr[0] = THRESHOLD_HI_REGISTER;
    data_wr[1] = 0x8F; // bits 15 to 8
    data_wr[2] = 0xFF; // bits 7 to 0

    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_wr, 3, -1));

    // Set CONVERSION_REGISTER of ADS1115
    data_wr[0] = CONVERSION_REGISTER;
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data_wr, 1, -1));

    return ESP_OK;
}