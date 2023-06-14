#ifndef DATASTORING
#define DATASTORING

#include "stdint.h"

class DataElement{
    uint16_t x; 
    uint16_t y;
    uint16_t z;
public:
    // Contructor
    DataElement(uint16_t x, uint16_t y, uint16_t z);
    
    // Getter methods
    uint16_t getDataX(){ return x; };
    uint16_t getDataY(){ return y; };
    uint16_t getDataZ(){ return z; };
};



#endif 