#ifndef CONTROLLER
#define CONTROLLER

extern "C" void measurementStart();
extern "C" void measurementLoop(void* unused);
extern "C" uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);
extern "C" int sendPaketWithData(bool terminate = false);

extern "C" void displayTunnelCurrent();
extern "C" void setTip(uint32_t z);

#endif