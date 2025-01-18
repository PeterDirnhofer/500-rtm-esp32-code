#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <stdint.h>
extern "C" void errorBlink();
double constrain(double value, double min, double max);
uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);
int16_t adcValueDebounced(int16_t adcValue);
void ledStatus(double currentTunnelnA, double targetTunnelnA, double toleranceTunnelnA, uint16_t dac);
void ledStatusAdc(int16_t adcValue, uint16_t targetAdc, uint16_t toleranceAdc, uint16_t dac);
double calculateTunnelNa(int16_t adcValue);
double clamp(double value, double minValue, double maxValue);
int16_t clampAdc(int16_t value, int16_t minValue, int16_t maxValue);
uint16_t computePiDac(int16_t adcValue, int16_t targetAdc);
uint16_t calculateAdcFromnA(double targetNa);

#endif // HELPER_FUNCTIONS_H