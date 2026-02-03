#include "movementXY.h"
#include "globalVariables.h"
#include <stdio.h>

static const char *TAG = "UsbPcInterface";

scanGrid::scanGrid(uint16_t widthX, uint16_t widthY)
    : maxX(widthX - 1), maxY(widthY - 1), currentX(0), currentY(0), minX(0),
      minY(0), currentDirection(0) {}

scanGrid::scanGrid(uint16_t maxX, uint16_t maxY, uint16_t startX,
                   uint16_t startY)
    : maxX(maxX), maxY(maxY), currentX(startX), currentY(startY), minX(startX),
      minY(startY), currentDirection(0) {}

/**
 * @brief Berechnung der Piezo-DAC Werte 'currentXDac' und 'currentYDac' für die
 * nächste anzusteuernde X Y Position. Es werden lediglich die DAC Werte
 * berechnet. Die eigentliche Ansteuerung des Piezo erfolgt separat in der
 * vspiDacLoop.
 * @return true, wenn grid komplett abgefahren ist
 */
bool scanGrid::moveOn() {
  bool bidirectional = (direction != 0);

  if (!bidirectional) {
    if (currentX < maxX) {
      currentX++; // Move right
      currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX,
                                   this->getMultiplicatorGridAdc());
    } else {
      currentX = startX;
      currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX,
                                   this->getMultiplicatorGridAdc());

      if (currentY != maxY) {
        currentY++; // Move to next row
        currentYDac = gridToDacValue(currentY, this->getMaxY(), DAC_VALUE_MAX,
                                     this->getMultiplicatorGridAdc());
      } else {
        return true; // All points scanned
      }
    }
    return false;
  }

  switch (static_cast<int>(currentDirection)) {
  case false:
    if (currentX < maxX) {
      currentX++; // Move right
      currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX,
                                   this->getMultiplicatorGridAdc());
    } else {
      if (currentY != maxY) {
        currentY++; // Move to next row
        currentYDac = gridToDacValue(currentY, this->getMaxY(), DAC_VALUE_MAX,
                                     this->getMultiplicatorGridAdc());
        currentDirection = true; // Change direction
      } else {
        return true; // All points scanned
      }
    }
    return false;
  case true:
    if (currentX > 0) {
      currentX--; // Move left
      currentXDac = gridToDacValue(currentX, this->getMaxX(), DAC_VALUE_MAX,
                                   this->getMultiplicatorGridAdc());
    } else {
      if (currentY != maxY) {
        currentY++; // Move to next row
        currentYDac = gridToDacValue(currentY, this->getMaxY(), DAC_VALUE_MAX,
                                     this->getMultiplicatorGridAdc());
        currentDirection = false; // Change direction
      } else {
        return true; // All points scanned
      }
    }
    return false;
  default:
    ESP_LOGE(TAG, "error move \n");
    return false;
  }
}

uint16_t scanGrid::getCurrentX() { return currentX; }

uint16_t scanGrid::getCurrentY() { return currentY; }

void scanGrid::setMaxX(uint16_t maxX) { this->maxX = maxX; }

void scanGrid::setMaxY(uint16_t maxY) { this->maxY = maxY; }

void scanGrid::setStartX(uint16_t startX) {
  this->currentX = startX;
  this->minX = startX;
}

void scanGrid::setStartY(uint16_t startY) {
  this->currentY = startY;
  this->minY = startY;
}

void scanGrid::setDirection(bool direction) {
  this->currentDirection = direction;
}

void scanGrid::setMultiplicatorGridAdc(uint16_t multiplicator) {
  this->multiplicatorGridAdc = multiplicator;
}

uint16_t scanGrid::getMaxX() { return this->maxX; }

uint16_t scanGrid::getMaxY() { return this->maxY; }

bool scanGrid::getCurrentDirection() { return this->currentDirection; }

uint16_t scanGrid::getMultiplicatorGridAdc() {
  return this->multiplicatorGridAdc;
}

uint16_t scanGrid::getMinX() { return this->minX; }

uint16_t scanGrid::getMinY() { return this->minY; }

/**
 * @brief Berechnung Piezo DAC Wert aus gewünschter Piezo Koordinate (X oder Y)
 *
 * @param currentGridValue Gewünschte X oder Y Koordinate im Grid
 * @param maxGridValue
 * @param maxValueDac
 * @param multiplicator
 * @return uint16_t ADC Wert für eine Koordinate, der an Piezo übergeben werden
 * kann
 */
uint16_t gridToDacValue(uint16_t currentGridValue, uint16_t maxGridValue,
                        uint16_t maxValueDac, uint16_t multiplicator) {
  return maxValueDac / 2 +
         (currentGridValue - (maxGridValue / 2 + 1)) * multiplicator;
}