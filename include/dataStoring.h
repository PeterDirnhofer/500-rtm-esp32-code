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

class DataElementTunnel
{
    int16_t dacz;
    int16_t adc;
    bool isTunnel;
    float currentNa; // New member for current

public:
    // Constructor
    DataElementTunnel(uint32_t dacz, int16_t adc, bool isTunnel, float currentNa);

    // Getter methods
    uint32_t getDacZ() { return dacz; }
    int16_t getAdc() { return adc; }
    bool getIsTunnel() { return isTunnel; }
    float getCurrentNa() { return currentNa; } // Getter for currentNa
};

#endif // DATASTORING_H
