#ifndef DATASTORING
#define DATASTORING

#include "stdint.h"

class dataElement{
    uint16_t x; 
    uint16_t y;
    uint16_t z;
public:
    dataElement(uint16_t x, uint16_t y, uint16_t z);
    uint16_t getDataX(){ return x; };
    uint16_t getDataY(){ return y; };
    uint16_t getDataZ(){ return z; };
};

void saveConfigParam(double param, uint8_t configDescriptionByte);

void sendDatasets(void* unused);

#endif 