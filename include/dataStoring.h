#ifndef DATASTORING_H
#define DATASTORING_H

#include "stdint.h" // Include for uint16_t, uint8_t, etc.

class DataElement {
    uint16_t x;
    uint16_t y;
    uint16_t z;

public:
    DataElement();
    DataElement(uint16_t x, uint16_t y, uint16_t z);

    uint16_t getDataX() const;
    uint16_t getDataY() const;
    uint16_t getDataZ() const;
};

#endif // DATASTORING_H
