#include "dataStoring.h"

// Constructor for DataElement
DataElement::DataElement(uint16_t x, uint16_t y, uint16_t z)
    : x(x), y(y), z(z)
{
    // Initialization logic if needed
}

// Constructor for DataElementTunnel (Matching the declaration)
DataElementTunnel::DataElementTunnel(int16_t dacz, int16_t adc, bool isTunnel)
    : dacz(dacz), adc(adc), isTunnel(isTunnel)
{
    // Initialization logic if needed
}
