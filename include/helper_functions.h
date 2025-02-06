#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <stdint.h>
extern "C" void setupError(const char *errormessage);
extern "C" void errorBlink();
void ledStatusAdc(int16_t adcValue, uint16_t targetAdc, uint16_t toleranceAdc, uint16_t dac);
double calculateTunnelNa(int16_t adcValue);
uint16_t computePiDac(int16_t adcValue, int16_t targetAdc);
uint16_t calculateAdcFromnA(double targetNa);

#endif // HELPER_FUNCTIONS_H