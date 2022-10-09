# RTM ESP32 - API 

The 500 EUR Raster Tunnel Microscope is controlled by a ESP32. The ESP32 is connected to a Computer via an additional USB Interface.

## Communication-Protocol ESP32 to Computer

- Start:   
  ***ESP32-->Computer*** cyclic **REQUEST**. ESP waits for computer response

- ***Computer-->ESP*** Computer answers **SETUP** or **MEASURE** or **PARAMETER**
  
---

## Details Computer --> ESP
- Case **MEASURE**  
  ESP starts one measure cycle for max 200 x 200 grid points.  
  ESP sends results in pakets to computer during measuring.  
  ESP sends "DONE".

- Case **SETUP**:  
  ESP starts infinite setup loop to send cyclic actual tunnel-current to Computer.

- Case **PARAMETER**:  
  ESP reads parameters from Computer and saves parameters to non volatile memory.  
  **Example** PARAMETER,10,1000,10.0,0.01,0,0,0,199,199

- Case **PARAMETER,DEFAULT**:  
  Set default parameter  
    kI = 10  
    kP = 1000
    destinationTunnelCurrentnA = 10.0  
    remainingTunnelCurrentDifferencenA = 0.01  
    startX = 0  
    startY = 0  
    direction = 0  
    maxX = 199  
    maxY = 199

- Case ESP receives **PARAMETER,?** from computer:  
  ESP sends actual parameters to computer

---

- Stop ESP and reset:  
  Sending **CTRL-C** respectively **Strg-C** stops ESP and forces a restart.
