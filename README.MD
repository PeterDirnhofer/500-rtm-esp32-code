# 🔬 RTM ESP32 - Raster Tunnel Microscope Controller

> **500 EUR Raster Tunnel Microscope** controlled by ESP32 with USB interface for computer communication

[![ESP32](https://img.shields.io/badge/ESP32-Espressif-red.svg)](https://www.espressif.com/en/products/socs/esp32)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP--IDF-orange.svg)](https://platformio.org/)
[![License](https://img.shields.io/github/license/PeterDirnhofer/500-rtm-esp32-code)](LICENSE)

---

## 🚀 Quick Start

### Prerequisites
- [VS Code](https://code.visualstudio.com/) with [PlatformIO extension](https://platformio.org/install/ide?install=vscode)
- ESP32 development board
- USB cable for programming and communication

### Installation

1. **🔧 Install Development Environment**
   ```bash
   # Install VS Code and PlatformIO extension
   # Follow the official PlatformIO installation guide
   ```

2. **📥 Clone Repository**
   ```bash
   cd C:\your\desired\installation\path
   git clone https://github.com/PeterDirnhofer/500-rtm-esp32-code.git
   cd 500-rtm-esp32-code
   ```

3. **🗂️ Open Project**
   ```bash
   code .
   ```
   > ⏱️ **Note:** PlatformIO may take a few moments to initialize the project on first open. Wait for the toolbar to appear before proceeding.

4. **🔨 Build & Upload**
   - **Build**: Click the ✅ checkmark icon in the status bar
   - **Upload**: Click the ➡️ right arrow icon to flash firmware
   - **Monitor**: Click the 🔌 plug icon for serial monitoring

---

## 📡 Communication between Controller and PC

### Additional Hardware Communication Interface
> **Important**: Communication to the controller uses a **separate USB interface**, not the ESP32's built-in programming USB interface. This dedicated interface ensures reliable communication between the computer and the microscope controller.

### Serial Configuration Communication Interface
- **Baud Rate**: `460800 bps`
- **Termination**: Line Feed (`\n` / `LF` / `0x0A`)
- **Flow**: ESP32 ↔ Computer bidirectional communication

### Protocol Flow
```
ESP32 → Computer: Sends cyclic "IDLE" status
Computer → ESP32: Responds with commands (ADJUST/MEASURE/PARAMETER)
```
  
---

## 📋 Command Reference

### 🔍 **MEASURE** Command
**Function**: Initiates measurement cycle for scanning tunnel microscope grid

```
Computer → ESP32: MEASURE
```

**Behavior**:
- Starts measurement cycle for max **200 × 200** grid points
- Sends real-time data during measurement in two packet types:

**Data Packets**:
```
DATA, currentX, currentY, currentZDac          // Valid measurement
DATA, targetTunnelAdc, adcValue, currentZDac   // Out of tunnel range
1, adcValue, currentZDac                       // Within tunnel limits
```

**Completion**: ESP32 sends `"DONE"` when measurement cycle finishes

---

### ⚙️ **ADJUST** Command
**Function**: Continuous setup mode for tunnel current monitoring

```
Computer → ESP32: ADJUST
```

**Behavior**:
- Starts infinite loop sending cyclic tunnel current data
- Used for system calibration and setup
- **Termination**: Send `CTRL-C` to stop

---

### 🔧 **TUNNEL** Command  
**Function**: Diagnostic mode for tip positioning and tunnel contact adjustment

```
Computer → ESP32: TUNNEL
```

**Response Packets**:
```
TUNNEL, 0, adcValue, currentZDac    // Out of tunnel limits
TUNNEL, 1, adcValue, currentZDac    // Within tunnel limits
```

**Termination**: Send `CTRL-C` to stop

---

### 📊 **PARAMETER** Commands

#### Set Parameters
```
Computer → ESP32: PARAMETER,kI,kP,targetCurrent,tolerance,startX,startY,direction,maxX,maxY,multiplier
```

**Example**:
```
PARAMETER,10,1000,10.0,0.01,0,0,0,199,199,10
```

#### Load Default Parameters
```
Computer → ESP32: PARAMETER,DEFAULT
```

**Default Values**:
| Parameter | Value | Description |
|-----------|--------|-------------|
| `kI` | 10 | Integral gain |
| `kP` | 1000 | Proportional gain |
| `targetTunnelCurrentnA` | 10.0 | Target tunnel current (nA) |
| `toleranceTunnelCurrentnA` | 0.01 | Current tolerance (nA) |
| `startX` | 0 | Starting X position |
| `startY` | 0 | Starting Y position |
| `direction` | 0 | Scan direction |
| `maxX` | 199 | Maximum X coordinate |
| `maxY` | 199 | Maximum Y coordinate |
| `multiplicator` | 10 | Scaling factor |

#### Query Current Parameters
```
Computer → ESP32: PARAMETER,?
ESP32 → Computer: [Current parameter values]
```

---

### 🛑 **Emergency Stop**
**Function**: Immediately halt ESP32 program execution and restart

```
Computer → ESP32: CTRL-C (or Strg-C)
```

**Behavior**:
- Can be sent **any time** during operation
- Forces immediate program termination and ESP32 restart
- Use for emergency stops or system reset

---

## 🏗️ Project Structure

```
500-rtm-esp32-code/
├── 📁 include/          # Header files
├── 📁 src/              # Source code
├── 📁 lib/              # Libraries
├── 📄 platformio.ini    # PlatformIO configuration
├── 📄 CMakeLists.txt    # CMake build configuration
└── 📄 README.MD         # This file
```

---

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the terms specified in the [LICENSE](LICENSE) file.

---

## 👨‍💻 Author

**Peter Dirnhofer**  
📧 Contact: [GitHub Profile](https://github.com/PeterDirnhofer)

---

<div align="center">
  <strong>🔬 Built for precision microscopy control with ESP32 🔬</strong>
</div>
