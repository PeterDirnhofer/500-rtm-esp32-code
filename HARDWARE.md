# ADC
## Differential Input   

## function readAdc() 
Returns int16_t

## ADJUST MONITORING

adjustLoop()
int16_t adcValue = readAdc();

```cpp
int16_t adcValue = readAdc(); // Read voltage from preamplifier

        currentTunnelnA = (adcValue * ADC_VOLTAGE_MAX * 1e3) /
                          (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);

        double adcInVolt = (adcValue * ADC_VOLTAGE_MAX * 1e2) /
                           (ADC_VALUE_MAX * RESISTOR_PREAMP_MOHM);
        UsbPcInterface::send("ADJUST,%f,%d\n", adcInVolt, adcValue);



```
sendTunnelPaket()
```
 while (!tunnelQueue.empty())
    {
        // Getter methods for queue elements
        int16_t adcz = tunnelQueue.front().getDataZ();
        int16_t adc = tunnelQueue.front().getAdc();
        bool istunnel = tunnelQueue.front().getIsTunnel();
        // Conditional logic to send "tunnel" or "no"
       
        const char *tunnelStatus = (istunnel) ? "tunnel" : "no";

        // Send data to PC with logging
        UsbPcInterface::send("TUNNEL;%d,%d,%s\n", adcz, adc, tunnelStatus);

        tunnelQueue.pop(); // Remove processed element from queue
    }
```



A0 Input:  +1,609 Volt Battery direct  
A1 Input:  minus  
ADJUST monitors: + 1.16 Volt  25816 counts

A0 Input: minus   
A1 Input: +1,609 Volt Battery   
ADJUST monitors: - 1.16 Volt  -25816 counts

## TUNNEL MONITORING
