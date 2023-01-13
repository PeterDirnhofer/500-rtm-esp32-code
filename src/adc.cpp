#include <stdint.h>

#include "driver/i2c.h"

#include "globalVariables.h"

uint16_t readAdc(){
    uint8_t dataH, dataL = 0;
    
    i2c_cmd_handle_t cmd;

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_READ, true);
    i2c_master_read_byte(cmd, &dataH, (i2c_ack_type_t) false);
    i2c_master_read_byte(cmd, &dataL, (i2c_ack_type_t) false);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 0);

    i2c_cmd_link_delete(cmd);

    uint16_t temp = dataH << 8 | dataL; 
    //printf("ADC %d\n", temp);
    return temp;
}

esp_err_t i2cAdcInit(){
    i2c_port_t i2c_master_port = I2C_MASTER_NUM;

    i2cConf.mode = I2C_MODE_MASTER;
    i2cConf.sda_io_num = I2C_MASTER_SDA_IO;
    i2cConf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    i2cConf.scl_io_num = I2C_MASTER_SCL_IO;
    i2cConf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    i2cConf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &i2cConf);

    //write config to ADS 1115 Single Conversion AI3 <-> GND
    
    esp_err_t errInstall = i2c_driver_install(i2c_master_port, i2cConf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x01, true);
    i2c_master_write_byte(cmd, 0x74, true); // Configbits 15 to 8
    i2c_master_write_byte(cmd, 0xE3, true); // Configbits 7 to 0
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    //activate interrupt on conversion end part 1
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x02, true); //lo_thresh
    i2c_master_write_byte(cmd, 0x00, true); // bits 15 to 8
    i2c_master_write_byte(cmd, 0x00, true); // bits 7 to 0
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    //activate interrupt on conversion end part 2
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x03, true); //hi_thresh
    i2c_master_write_byte(cmd, 0x8F, true); // bits 15 to 8
    i2c_master_write_byte(cmd, 0xFF, true); // bits 7 to 0
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 0);
    i2c_cmd_link_delete(cmd);
    

    return errInstall;
}