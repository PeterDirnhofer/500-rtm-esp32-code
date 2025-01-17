#ifndef DATASTORING_H
#define DATASTORING_H

#include "stdint.h" // Include for uint16_t, uint8_t, etc.

class DataElement
{
    uint16_t x;
    uint16_t y;
    uint16_t z;

public:
    DataElement() : x(0), y(0), z(0) {}
    DataElement(uint16_t x, uint16_t y, uint16_t z) : x(x), y(y), z(z) {}

    uint16_t getDataX() const { return x; }
    uint16_t getDataY() const { return y; }
    uint16_t getDataZ() const { return z; }
};


#endif // DATASTORING_H
