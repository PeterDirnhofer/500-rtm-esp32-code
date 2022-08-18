#ifndef CONTROLLER
#define CONTROLLER

void controllerStart();
void controllerLoop(void* unused);
uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);

#endif