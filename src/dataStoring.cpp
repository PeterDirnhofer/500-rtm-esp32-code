#include "dataStoring.h"
// #include "communication.h"
#include "globalVariables.h"


// TODO define dataElement in controller.cpp
// constructor of dataElement for queue
DataElement::DataElement(uint16_t x, uint16_t y, uint16_t z)
    : x(x), y(y), z(z)
{
}