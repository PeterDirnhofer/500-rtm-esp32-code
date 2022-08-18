#include <stdio.h>
#include <cstring>
#include "globalVariables.h"
#include "communication.h"
#include "spi.h"
#include "dataStoring.h"

protocolElement::protocolElement(uint16_t x, uint16_t y, uint16_t z, uint8_t config)

{
    this->dataBytewise[0] = config;
    this->dataBytewise[1] = (x & 0xFF);
    this->dataBytewise[2] = (x & (0xFF << 8)) >> 8;
    this->dataBytewise[3] = (y & 0xFF);
    this->dataBytewise[4] = (y & (0xFF << 8)) >> 8;
    this->dataBytewise[5] = (z & 0xFF);
    this->dataBytewise[6] = (z & (0xFF << 8)) >> 8;
}
protocolElement::protocolElement(uint8_t *dataBytewise)

{

    // received data and going to analyze it
    this->x = 0x00;
    this->y = 0x00;
    this->z = 0x00;
    this->configBad = false;
    uint8_t configByte = dataBytewise[0];

    // printf("received config: 0x%02x\n", configByte);
    switch (configByte)
    {
    case ((1 << POSITION_PI_SEND_CONFIG)):
    {
        configExisting |= (1 << dataBytewise[1]);
        
        if (lastConfigExisting != configExisting)
        {
            printf("configExisting: %d \n", configExisting);
        }
        
        lastConfigExisting = configExisting;
        lastConfigByte = configByte;

        double param = 0;
        std::memcpy(&param, &(dataBytewise[2]), 8);
        saveConfigParam(param, dataBytewise[1]);
        break;
    }
    case ((1 << POSITION_ESP_READY_CONFIGFILE)):
        if (lastConfigByte != configByte)
        {
            printf("ESP ready to receive config file \n");
        }

        lastConfigByte = configByte;

        /*send config*/
        break;
    case ((1 << POSITION_ESP_READY_DATA)):
        printf("ESP ready to send datasets \n");
        /*receive data*/
        break;
    case ((1 << POSITION_ESP_SEND_DATA)):
        printf("ESP sending data to pi \n");
        /*save to queue*/
        break;
    case ((1 << POSITION_ESP_SEND_DATA) | (1 << POSITION_ESP_ENDTAG)):
        printf("ESP sending data to pi, last dataset \n");
        /*save to queue*/
        break;
    case ((1 << POSITION_PI_RCV_BAD)):
        printf("Pi received bad transmition, sending previous again \n");
        configBad = true;
        break;
    default:
        // printf("unknown protocol config byte: 0x%02x\n", configByte);
        break;
    }
}

uint8_t protocolElement::getDataBytewise(int byteNumber)
{
    return this->dataBytewise[byteNumber];
}

bool protocolElement::getConfigBad()
{
    return this->configBad;
}
