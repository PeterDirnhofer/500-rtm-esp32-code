# RTM ESP32 - API

The 500 EUR Raster Tunnel Microscope is controlled by a ESP32. The ESP32 has an additional USB Interface for communication with the computer.

## Communication-Protocol ESP32 to Computer



- Start:  
  ***ESP32-->Computer*** ESP sends cyclic **IDLE**. ESP waits for computer response

- ***Computer-->ESP*** Computer answers with **ADJUST** or **MEASURE** or **PARAMETER**

115200 Bit/s   
Strings are terminated with Linefeed \n = LF = 0x0A
  
---

## Use cases Computer --> ESP

- Case computer sends **MEASURE**  
  ESP starts one measure cycle for max 200 x 200 grid points.  
  ESP sends results in two pakets to computer during measuring.  
  ESP sends "DONE" when measure cycle if finished.

- Case computer sends **ADJUST**:  
  ESP starts infinite setup loop to send cyclic actual tunnel-current to Computer. Terminate with CTRL-C.

- Case computer sends **PARAMETER**:  
  ESP receives parameters from Computer and saves parameters to its non volatile memory.  
  **Example** PARAMETER,10,1000,10.0,0.01,0,0,0,199,199,10

- Case computer sends **PARAMETER,DEFAULT**:  
  Save default parameter in ESP  
    kI = 10  
    kP = 1000
    destinationTunnelCurrentnA = 10.0  
    remainingTunnelCurrentDifferencenA = 0.01  
    startX = 0  
    startY = 0  
    direction = 0  
    maxX = 199  
    maxY = 199  
    multiplicator = 10

- Case computer sends **PARAMETER,?**:  
  ESP sends actual parameters to computer

---

- **Stop ESP programm** execution and restart ESP:  
  Computer sends **CTRL-C** respectively **Strg-C** to stop ESP and force a restart. CTRL-C can be sent any time.
