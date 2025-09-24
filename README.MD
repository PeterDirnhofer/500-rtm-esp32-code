# RTM ESP32 - API

The 500 EUR Raster Tunnel Microscope is controlled by a ESP32. The ESP32 has an additional USB Interface for communication with the computer.

---

## Installation


### Using PlatformIO

1. **Install PlatformIO**  
   - Install [VS Code](https://code.visualstudio.com/)  
   - Install the [PlatformIO extension](https://platformio.org/install/ide?install=vscode)


2. **Clone this repository**  
   ```
   cd C:\your\desired\installation\path
   git clone https://github.com/PeterDirnhofer/500-rtm-esp32-code.git
   cd 500-rtm-esp32-code
   ```
  3. **Open the project in VS Code using the command line**  
    - Run the following command in your project directory:
      ```
      code .
      ```

4. **Build and upload**  
   - Click the PlatformIO "Build" button (checkmark icon) in the status bar.
   - Click the "Upload" button (right arrow icon) to flash the firmware to your ESP32.

5. **Monitor serial output**  
   - Click the "Monitor" button (plug icon) or use the PlatformIO Serial Monitor.

#### Python requirements

If you need to install Python dependencies manually (for PlatformIO or related tooling), use:

```
pip install -r requirements.txt
```

You can also list all installed Python packages with:

```
pip freeze
```

---

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
  Send to PC:   
  DATA, rtmGrid.getCurrentX(), rtmGrid.getCurrentY(), currentZDac if valid   
  DATA, targetTunnelAdc, adcValue, currentZDac if not in Tunnel current


  1, adcValue, currentZDac if in Tunnel current limits    

  ESP sends "DONE" when measure cycle if finished.

- Case computer sends **ADJUST**:  
  ESP starts infinite setup loop to send cyclic actual tunnel-current to Computer. Terminate with CTRL-C.
  
- Case computer sends **TUNNEL**:  
  ESP starts infinite tunnel loop. Used for setup and diagnostics to adjust tip near of tunnel contact.  
  Send to PC:   
  TUNNEL, 0, adcValue, currentZDac if out of limits   
  TUNNEL, 1, adcValue, currentZDac if in Tunnel current limits    
  Terminate with CTRL-C.

- Case computer sends **PARAMETER**:  
  ESP receives parameters from Computer and saves parameters to its non volatile memory.  
  **Example** PARAMETER,10,1000,10.0,0.01,0,0,0,199,199,10

- Case computer sends **PARAMETER,DEFAULT**:  
  Save default parameter in ESP  
    kI = 10  
    kP = 1000
    targetTunnelCurrentnA = 10.0  
    toleranceTunnelCurrentnA = 0.01  
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
  1, adcValue, currentZDac if in Tunnel current limits    

  ESP sends "DONE" when measure cycle if finished.

- Case computer sends **ADJUST**:  
  ESP starts infinite setup loop to send cyclic actual tunnel-current to Computer. Terminate with CTRL-C.
  
- Case computer sends **TUNNEL**:  
  ESP starts infinite tunnel loop. Used for setup and diagnostics to adjust tip near of tunnel contact.  
  Send to PC:   
  TUNNEL, 0, adcValue, currentZDac if out of limits   
  TUNNEL, 1, adcValue, currentZDac if in Tunnel current limits    
  Terminate with CTRL-C.

- Case computer sends **PARAMETER**:  
  ESP receives parameters from Computer and saves parameters to its non volatile memory.  
  **Example** PARAMETER,10,1000,10.0,0.01,0,0,0,199,199,10

- Case computer sends **PARAMETER,DEFAULT**:  
  Save default parameter in ESP  
    kI = 10  
    kP = 1000
    targetTunnelCurrentnA = 10.0  
    toleranceTunnelCurrentnA = 0.01  
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
