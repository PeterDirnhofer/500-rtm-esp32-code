#ifndef MOVEMENTXY
#define MOVEMENTXY

#include "esp_log.h"
#include "stdint.h"


class scanGrid {
  uint16_t maxX;
  uint16_t maxY;
  uint16_t currentX;
  uint16_t currentY;
  uint16_t minX;
  uint16_t minY;
  bool currentDirection;
  uint16_t multiplicatorGridAdc;

public:
  scanGrid(uint16_t maxX, uint16_t maxY);
  scanGrid(uint16_t maxX, uint16_t maxY, uint16_t startX, uint16_t startY);
  bool moveOn();

  void setMaxX(uint16_t);
  void setMaxY(uint16_t);
  void setStartX(uint16_t);
  void setStartY(uint16_t);
  void setDirection(bool);
  void setMultiplicatorGridAdc(uint16_t);

  uint16_t getCurrentX();
  uint16_t getCurrentY();
  uint16_t getMinX();
  uint16_t getMinY();
  uint16_t getMaxX();
  uint16_t getMaxY();
  bool getCurrentDirection();
  uint16_t getMultiplicatorGridAdc();
};

uint16_t gridToDacValue(uint16_t currentGridValue, uint16_t maxGridValue,
                        uint16_t maxValueDac, uint16_t multiplicator);

#endif