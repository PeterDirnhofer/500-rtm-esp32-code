#include "dataStoring.h"
// #include "communication.h"
#include "globalVariables.h"

// constructor of dataElement for queue
DataElement::DataElement(uint16_t x, uint16_t y, uint16_t z)
    : x(x), y(y), z(z)
{
}

// Constructor of DataElementTunnel
DataElementTunnel::DataElementTunnel(uint16_t dacz, float currentNa, bool isTunnel)
    : dacz(dacz), currentNa(currentNa), isTunnel(isTunnel)
{
}