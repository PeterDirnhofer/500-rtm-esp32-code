#ifndef CONTROLLER
#define CONTROLLER

extern "C" void controllerStart();
extern "C" void controllerLoop(void* unused);
extern "C" uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);

extern "C" void displayTunnelCurrent();

#endif