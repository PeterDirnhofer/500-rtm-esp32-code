#ifndef DATASTORING
#define DATASTORING

#include "stdint.h"

class DataElement
{
    uint16_t x;
    uint16_t y;
    uint16_t z;

public:
    // Contructor
    DataElement(uint16_t x, uint16_t y, uint16_t z);

    // Getter methods
    uint16_t getDataX() { return x; };
    uint16_t getDataY() { return y; };
    uint16_t getDataZ() { return z; };
};

// DataElementTunnel Class Definition with isTunnel
class DataElementTunnel
{
    uint16_t dacz;
    float currentNa; // New float member
    bool isTunnel;    // New boolean member to indicate tunnel state (on/off)
public:
    // Constructor
    DataElementTunnel(uint16_t dacz, float currentNa, bool isTunnel);

    // Getter methods
    uint16_t getDataZ() { return dacz; }
    float getCurrent() { return currentNa; }
    bool getIsTunnel() { return isTunnel; } // Getter for isTunnel
};

#endif