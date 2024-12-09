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


## Config AD
```
Original was 0x48  0xE3
// INIT ADC
    //  Config ADS 1115 Single0
    //  ORIGIBANL  0x48      
    //  Bits 15 14 13 12  11 10 09 08
    //        0  1  0  0   1  0  0  0   = 0x48
    //       15 Single Conversion
    //           MUX
    //           1  0  0   : AINP = AIN0 and AINN = GND
    //                      
    //                     1  0  0    100 : FSR = ±0.512 V
    //                              0 Continuous mode, no reset after conversion
      
    
    
    
    //  actual  0x04 0xE3
    
    //        0  0  0  0   0  1  0  0   = 0x04
    //       15 Single Conversion
    //           MUX
    //           0  0  0  : AINP = AIN0 and AINN = AIN1
    //                     0  1  0  010 : FSR = ±2.048 V (default)
    //                              0 Continuous mode, no reset after conversion

    // 0xE3
    // Bits 07 06 05 04  03 02 01 00
    //       1  1  1  0   0  0  1  1
    //       1  1  1  Datarate 860 samples per second  (ACHTUNG passt das in ms takt?)
    //                0 Comparator traditional (unwichtig, nur für Threshold)
    //                    0 Alert/Rdy Active low
    //                        0  No latch comparator
    //                           1  1  Disable comparator





// Use espressif 5.3 or later
    // INIT ADC
    //  Config ADS 1115 Single0
    //  0x04
    //  Bits 15 14 13 12  11 10 09 08
    //        0  0  0  0   0  1  0  0
    //       15 Single Conversion
    //           0  0  0  Measure Comparator AIN0 against AIN1
    //                     0  1  0  gain +- 2.048 V
    //                              0 Continuous mode, no reset after conversion

    // 0xE3
    // Bits 07 06 05 04  03 02 01 00
    //       1  1  1  0   0  0  1  1
    //       1  1  1  Datarate 860 samples per second  (ACHTUNG passt das in ms takt?)
    //                0 Comparator traditional (unwichtig, nur für Threshold)
    //                    0 Alert/Rdy Active low
    //                        0  No latch comparator
    //                           1  1  Disable comparator

    // Define I2C master bus configuration
```

