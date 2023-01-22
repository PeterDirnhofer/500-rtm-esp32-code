#include <stdint.h>

#include "driver/i2c.h"

#include "globalVariables.h"

uint16_t readAdc(){

   
    
    uint8_t dataH, dataL = 0;
    
    i2c_cmd_handle_t cmd;
    

    cmd = i2c_cmd_link_create();
    
    gpio_set_level(IO_02, 1);
    gpio_set_level(IO_02, 0);
    
    i2c_master_start(cmd);
    
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_READ, true);


    i2c_master_read_byte(cmd, &dataH, (i2c_ack_type_t) false);
    i2c_master_read_byte(cmd, &dataL, (i2c_ack_type_t) false);
    i2c_master_stop(cmd);
    
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 0);
   
   
    
    i2c_cmd_link_delete(cmd);

    uint16_t temp = dataH << 8 | dataL; 

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
    
    // Init I2C 
    esp_err_t errInstall = i2c_param_config(i2c_master_port, &i2cConf);
    if (errInstall != ESP_OK) {
    
        return errInstall;
    }

    errInstall = i2c_driver_install(i2c_master_port, i2cConf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
    if (errInstall != ESP_OK) {
    
        return errInstall;
    }

    //INIT ADC 
    // Config ADS 1115 Single 
    // Bits 15 14 13 12  11 10 09 08
    //       0  1  1  1   0  1  0  0
    //      15 Single Conversion
    //          1  1  1  Measure Comparator A3 against GND
    //                    0  1  0  gain +- 2.048 V
    //                             0 Continuous mode, no reset after conversion


    // Bits 07 06 05 04  03 02 01 00
    //       1  1  1  0   0  0  1  1
    //       1  1  1  Datarate 860 samples per second  (ACHTUNG passt das in ms takt?)
    //                0 Comparator traditional (unwichtig, nur für Threshold)
    //                    0 Alert/Rdy Active low
    //                        0  No latch comparator
    //                           1  1  Disable coparator
    
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Set Configuration Register od ADS1115
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x01, true); // Select Register Configuration
    i2c_master_write_byte(cmd, 0x74, true); // Configbits 15 to 8 0111 0100
    i2c_master_write_byte(cmd, 0xE3, true); // Configbits 7 to 0  1110 0011
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    // Set lo threshold Register od ADS1115
    //activate interrupt on conversion end part 1
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x02, true); //Select Register lo_threshold
    i2c_master_write_byte(cmd, 0x00, true); // bits 15 to 8
    i2c_master_write_byte(cmd, 0x00, true); // bits 7 to 0
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    //activate interrupt on conversion end part 2
    // Set hi threshold Register od ADS1115
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x03, true); //Select Register hi_threshold
    i2c_master_write_byte(cmd, 0x8F, true); // bits 15 to 8
    i2c_master_write_byte(cmd, 0xFF, true); // bits 7 to 0
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ADS_1115_ADDRESS_WRITE, true);
    i2c_master_write_byte(cmd, 0x00, true); // Select Register Conversion
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 0);
    i2c_cmd_link_delete(cmd);
    

    return errInstall;
}