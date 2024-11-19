#include "dataStoring.h"

// Constructor for DataElement
DataElement::DataElement(uint16_t x, uint16_t y, uint16_t z)
    : x(x), y(y), z(z)
{
    // Initialization logic if needed
}

// Constructor for DataElementTunnel (Matching the declaration)
DataElementTunnel::DataElementTunnel(uint32_t dacz, int16_t adc, bool isTunnel, float currentNa)
    : dacz(dacz), adc(adc), isTunnel(isTunnel), currentNa(currentNa)
{
    // Initialization logic if needed
}