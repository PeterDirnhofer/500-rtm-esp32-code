# Communication between Terminal Raspberry and ESP32

## Terminal

``sudo ./rtm_pi 100 1000 10.0 0.01 0 0 0 199 199``

## Raspberry

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

--

## Programmieren über die JTAG Schnittstelle mit ESP-Prog


Während der Entwicklung des Programms wird das ESP-Prog Tool um den Code von Visual Studio auf den ESP zu übertragen.    
So bleibt die Standard ESP USB Schnittstelle für die Kommunikation mit dem Anwender frei.

So gehts:

<https://docs.platformio.org/en/latest/plus/debug-tools/esp-prog.html>

In platformio.ini:

``
upload_port = 
upload_protocol = esp-prog``

Verbindung ESP32-ESP-Prog Tool:

GND  
ESP-TMS GPIO_14  
ESP-TCK GPIO_13  
ESP_TDO GPIO_15  
ESP-TDI GPIO_12  
