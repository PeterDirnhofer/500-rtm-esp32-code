#ifndef DATASTORING_H
#define DATASTORING_H

#include "stdint.h" // Include for uint16_t, uint8_t, etc.

class DataElement
{
    uint16_t x;
    uint16_t y;
    uint16_t z;

public:
    // Constructor for DataElement
    DataElement(uint16_t x, uint16_t y, uint16_t z);

    // Getter methods
    uint16_t getDataX() { return x; };
    uint16_t getDataY() { return y; };
    uint16_t getDataZ() { return z; };
};

// DataElementTunnel Class Definition with isTunnel
class DataElementTunnel
{
    int16_t dacz;  // DCZ value
    int16_t adc;   // ADC value
    bool isTunnel; // Tunnel state (true or false)

public:
    // Constructor for DataElementTunnel (Declaration)
    DataElementTunnel(int16_t dacz, int16_t adc, bool isTunnel);

    // Getter methods
    int16_t getDataZ() { return dacz; }
    int16_t getAdc() { return adc; }
    bool getIsTunnel() { return isTunnel; }
};

#endif // DATASTORING_H
