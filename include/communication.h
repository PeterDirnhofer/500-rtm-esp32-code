#ifndef COMMUNICATION
#define COMMUNICATION


#define POSITION_ESP_ENDTAG 0
#define POSITION_PI_REQUEST 1
#define POSITION_ESP_READY_CONFIGFILE 2
#define POSITION_ESP_READY_DATA 3
#define POSITION_ESP_SEND_DATA 4
#define POSITION_PI_SEND_CONFIG 5
#define POSITION_PI_RCV_BAD 6

#define SPECIFIER_CONFIG_KI 0
#define SPECIFIER_CONFIG_KP 1
#define SPECIFIER_CONFIG_DESTINATION_CURRENT 2 
#define SPECIFIER_CONFIG_REMAINING_CURRENT 3 
#define SPECIFIER_CONFIG_START_X 4
#define SPECIFIER_CONFIG_START_Y 5
#define SPECIFIER_CONFIG_DIRECTION 6
#define SPECIFIER_CONFIG_MAX_X 7
#define SPECIFIER_CONFIG_MAX_Y 8
#define SPECIFIER_CONFIG_MULTIPLICATOR_ADC 9

#include <stdint.h>

class protocolElement{
    uint8_t dataBytewise[10];
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint8_t config;
    bool configBad;
    
public:
    protocolElement(uint16_t x, uint16_t y, uint16_t z, uint8_t config);
    protocolElement(uint8_t *dataBytewise);
    uint8_t getDataBytewise(int byteNumber);
    bool getConfigBad();
};

#endif