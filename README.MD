# RTM ESP32 - USB

The 500 EUR Raster Tunnel Microscope is controlled by a ESP32. The ESP32 is connected to a Computer via a additional USB Interface. No Raspberry needed.

## Communication-Protocol ESP32 to Computer

- ESP32 starts and sends cyclic "REQUEST" to computer and waits for Computer response.
- Computer answeres to "REQUEST" with "SETUP" or "MEASURE" or "PARAMETER"
  
---

- Case ESP receives "MEASURE" from computer:  
  ESP starts one measure cycle for 200 x 200 grid points.  
  ESP sends results in pakets to computer during measuring.  
  ESP sends "DONE".

- Case ESP receives "SETUP" from computer:  
  ESP starts infinite setup loop to send cyclic actual tunnel-current to Computer.

- Case ESP receives "PARAMETER" from computer:  
  ESP reads parameters from Computer and saves parameters to non volatile memory.  

---

- Stop ESP and reset:  
  ESP stops each action and does a reboot when receiving a "CTRL-C" / "Strg-C".
