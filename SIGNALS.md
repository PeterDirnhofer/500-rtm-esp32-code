## Compute DAC value (Proportional/Integral)  
### uint16_t computePiDac(int16_t adcValue, int16_t targetAdc)
error = targetAdc - adcValue  
proportional = kP * error  
integralError += error  
integral = kI * integralError  
dacValue = currentZDac - proportional - integral  
return dacValue


### extern "C" void measureLoop(void *unused)
errorAdc = targetTunnelAdc - adcValue  
if (abs(errorAdc) > toleranceTunnelAdc):  
newDacZ = computePiDac(adcValue, targetTunnelAdc);
currentZDac = newDacZ



