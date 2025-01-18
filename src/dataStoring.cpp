// dataStoring.cpp
#include "dataStoring.h"

// Default constructor
DataElement::DataElement() : x(0), y(0), z(0) {}

// Parameterized constructor
DataElement::DataElement(uint16_t x, uint16_t y, uint16_t z) : x(x), y(y), z(z) {}

// Getter for x
uint16_t DataElement::getDataX() const {
    return x;
}

// Getter for y
uint16_t DataElement::getDataY() const {
    return y;
}

// Getter for z
uint16_t DataElement::getDataZ() const {
    return z;
}