#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <stdint.h>

extern "C" int sendTunnelPaket();
double constrain(double value, double min, double max);
uint16_t saturate16bit(uint32_t input, uint16_t min, uint16_t max);
int16_t adcValueDebounced(int16_t adcValue);
void ledStatus(double currentTunnelnA, double targetTunnelnA, double toleranceTunnelnA, uint16_t dac);
double calculateTunnelNa(int16_t adcValue);
double clamp(double value, double minValue, double maxValue);
uint16_t computePI(double currentNa, double targetNa);

#endif // HELPER_FUNCTIONS_H