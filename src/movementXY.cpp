#include "movementXY.h"
#include "globalVariables.h"
#include "stdio.h"

scanGrid::scanGrid(uint16_t widthX, uint16_t widthY)
    : maxX(widthX - 1), maxY(widthY - 1), currentX(0), currentY(0), currentDirection(0)
{
}

scanGrid::scanGrid(uint16_t maxX, uint16_t maxY, uint16_t startX, uint16_t startY)
    : maxX(maxX), maxY(maxY), currentX(startX), currentY(startY), currentDirection(0)
{
}

/**
 * @brief Berechnung der Piezo-DAC Werte 'currentXDac' und 'currentYDac' für die nächste anzusteuernde Position.
 * Es werden lediglich die DAC Werte berechnet. Die eigentliche Ansteuerung des Piezo erfolgt später in der vspiDacLoop.
 *
 *
 * Scanpattern
 *
 * Y=000    X=000 +++ X=199    Line 00,001 -  00,200
 * Y=001    X=199 --- X=000    Line 00,201 -  00,400
 * Y=002    X=000 +++ X=199    Line 00,401 -  00,600
 * Y=003    X=199 --- X=000    Line 00,601 -  00,800
 * ...
 * ...
 * Y=196    x=000 +++ X=199    Line 39,201 -  39,400
 * Y=197    X=199 --- X=000    Line 39,401 -  39,600
 * Y=198    x=000 +++ X=199    Line 39,601 -  39,800
 * Y=199    X=199 --- X=000    Line 39,801 -  40,000
 *
 * @return true, wenn grid komplett abgefahren ist
 */
bool scanGrid::moveOn()
{
    // get global parameter direction 
    bool bidirectional;

    if (direction==0)
        bidirectional=false;
    else
        bidirectional=true;

    if (bidirectional == false)
    {
        if (currentX < maxX)
        {
            currentX++; // rightwise
            currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());
        }
        else
        {
            currentX=0;
            currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());


            if (currentY != maxY)
            {

                currentY++; // next row
                currentYDac = gridToDacValue(currentY, this->getMaxY(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());
            }
            else
            {
                return true; // all points scanned
            }
        }
        return false;
    }


    // printf("moveOn %d\n",(int)currentDirection);
    switch ((int)currentDirection)
    {
    case false:

        if (currentX < maxX)
        {
            currentX++; // rightwise
            // printf("moveOn currentX++ %d\n", currentX);

            currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());
        }
        else
        {

            if (currentY != maxY)
            {

                currentY++; // next row
                currentYDac = gridToDacValue(currentY, this->getMaxY(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());
                currentDirection = true; // direction change
            }
            else
            {
                return true; // all points scanned
            }
        }
        return false;
        break;
    case true:

        if (currentX > 0)
        {
            currentX--; // leftwise
            currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());
        }
        else
        {
            if (currentY != maxY)
            {

                currentY++; // next row
                currentYDac = gridToDacValue(currentY, this->getMaxY(), DAC_VALUE_MAX, this->getMultiplicatorGridAdc());
                currentDirection = false; // direction change
            }
            else
            {
                return true; // all points scanned
            }
        }
        return false;
        break;
    default:
        printf("error move \n");
        return false;
    }
}

uint16_t scanGrid::getCurrentX()
{
    return currentX;
}

uint16_t scanGrid::getCurrentY()
{
    return currentY;
}

void scanGrid::setMaxX(uint16_t maxX)
{
    this->maxX = maxX;
}

void scanGrid::setMaxY(uint16_t maxY)
{
    this->maxY = maxY;
}

void scanGrid::setStartX(uint16_t startX)
{
    this->currentX = startX;
}

void scanGrid::setStartY(uint16_t startY)
{
    this->currentY = startY;
}

void scanGrid::setDirection(bool direction)
{
    this->currentDirection = direction;
}

void scanGrid::setMultiplicatorGridAdc(uint16_t multiplicator)
{
    this->multiplicatorGridAdc = multiplicator;
}

uint16_t scanGrid::getMaxX()
{
    return this->maxX;
}

uint16_t scanGrid::getMaxY()
{
    return this->maxY;
}

bool scanGrid::getCurrentDirection()
{
    return this->currentDirection;
}

uint16_t scanGrid::getMultiplicatorGridAdc()
{
    return this->multiplicatorGridAdc;
}

/**
 * @brief Berechnung Piezo DAC Wert aus gewünschter Piezo Koordinate (X oder Y)
 *
 * @param currentGridValue Gewünschte X oder Y Koordinate im Grid
 * @param maxGridValue
 * @param maxValueDac
 * @param multiplicator
 * @return uint16_t ADC Wert für eine Koordinate, der an Piezo übergeben werden kann
 */
uint16_t gridToDacValue(uint16_t currentGridValue, uint16_t maxGridValue, uint16_t maxValueDac, uint16_t multiplicator)
{
    return maxValueDac / 2 + (currentGridValue - (maxGridValue / 2 + 1)) * multiplicator;
}