# ESP32 esp-idf for STM

## Communication with Monitor via additional UART

### Measuring Results

Results are sent when available automatically via UART

### Monitoring the test probes height

Press **ESC** during startup

### Measuring Parameter

Press **P** during startup

---

## Process flow controller (after initialisation)

- Timer starts controllerloop cyclic

- If tunnel-current is in limit
  - controllerloop saves valid values
  - rtmGrid.moveOn to set next XY Position currentX and currentY
  - sleep
- if tunnel-current Out off limit
  - controllerloop calculates new Z value currentZDac
  - sleep
- wait for next timer

---

## Communication between Terminal Raspberry and ESP32 - depracted

### Terminal

``sudo ./rtm_pi 100 1000 10.0 0.01 0 0 0 199 199``

### Raspberry

Receive parameter from Terminal in

**main(argv[])**

```c
int main (int argc, char *argv[]) {
    ...
    kI = atof(argv[1]);                                
    kP = atof(argv[2]);
    destinationTunnelCurrentnA = atof(argv[3]); 
    remainingTunnelCurrentDifferencenA = atof(argv[4]); 
    startX = (uint16_t) atoi(argv[5]);
    startY = (uint16_t) atoi(argv[6]);
    direction = (bool) atoi(argv[7]);
    maxX = (uint16_t) atoi(argv[8]);
    maxY = (uint16_t) atoi(argv[9]);
```

**Wait for handshake Interrupt from ESP32** falling edge

**spiSendConfig(double kI, .... , uint16_t maxX)**

```c
// create dataArray
double dataArray[] = {kI, kP, destinationTunnelCurrentnA, remainingTunnelCurrentDifferencenA, (double) startX, (double) startY, (double) direction, (double) maxX, (double) maxY};


char buf[10] = {0};

buf[0] = (char)(1 << POSITION_PI_SEND_CONFIG);  // = 5
buf[1] = (char)(configSend);
memcpy(&buf[2], &dataArray[configSend], 8);

bcm2835_spi_writenb(buf, 10);
```

# ESP32 Memory

I (369) heap_init: At 3FFAE6E0 len 00001920 (6 KiB): DRAM  
I (375) heap_init: At 3FFB4638 len 0002B9C8 (174 KiB): DRAM  
I (381) heap_init: At 3FFE0440 len 00003AE0 (14 KiB): D/IRAM  
I (388) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM  
I (394) heap_init: At 4008E39C len 00011C64 (71 KiB): IRAM  

0002B9C8 (178.632) Byte / 0x400 (1024) = AE (174) kiB DRAM

# Memory for measuring results

Number of Values for 1 Measurement 6 Byte:  
DATAELEMENT uint16_t x, uint16_t y, uint16_t z

Number of grid Points 40.000  
200*200 grid = 40.000*6
DATAELEMENT uint16_t x, uint16_t y, uint16_t z = 6 Byte  

Total 40.000 * 6 = 240.000 Byte
