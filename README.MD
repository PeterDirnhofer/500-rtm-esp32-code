# 🔬 RTM ESP32 - Raster Tunnel Microscope Controller

> **500 EUR Raster Tunnel Microscope** controlled by ESP32 with USB interface for computer communication

[![ESP32](https://img.shields.io/badge/ESP32-Espressif-red.svg)](https://www.espressif.com/en/products/socs/esp32)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-ESP--IDF-orange.svg)](https://platformio.org/)
[![License](https://img.shields.io/github/license/PeterDirnhofer/500-rtm-esp32-code)](LICENSE)

---

## 🚀 Quick Start

### Prerequisites
- VS Code with the PlatformIO extension
- ESP32 development board
- USB cable for programming and communication

### Installation
1. Install VS Code and PlatformIO extension.
2. Clone the repository and open it in VS Code:

```bash
git clone https://github.com/PeterDirnhofer/500-rtm-esp32-code.git
cd 500-rtm-esp32-code
code .
```

3. Build & Upload using the PlatformIO toolbar (checkmark = build, arrow = upload, plug = monitor).

---

## Communication betwenn Computer and ESP32

Communication to the ESP32 can be done via three paths:

### 1) USB Interface
- Direct serial connection to the additional USB serial interface on the board (not the programming USB some dev boards expose).
- Serial Baud Rate: `460800`
- Line termination: LF (`\n`)

### 2) ESP Access Point (AP)
- The ESP always advertises the Access Point. ESP spans its own Wi‑Fi network. To use it, your computer must disconnect from your normal WLAN and connect to the ESP's Wi‑Fi network instead.
- SSID: `ESP_Server`
- Password: `esp12345`
- Device IP: `192.168.4.1` 


### 3) ESP as Station (join your WLAN)
- The ESP connects to your existing local WLAN so other devices like your computer can reach ESP.
- To join your WLAN, the ESP needs your WLAN `SSID` and `Password`.
- You have to store your `SSID` and `Password` in the ESP before you can use ESP as Station. 
- Storing `SSID` and `Password` can be done by using Interface 1) or 2) above and using the commands `SET_SSID` and `SET_PASSWORD` 

Notes:
- After setting Password and SSID reset the ESP. ESP then tries to access your WLAN using the stored SSID and PASSWORD and if successfull, LED5 turns on. 

---

## 📋 Command Reference (selected)

### MEASURE
Start a measurement cycle:

```
MEASURE
```

Sends data packets and finishes with `DONE`.

### ADJUST
Enter continuous monitoring mode:

```
ADJUST
```

Terminate with `CTRL-C`.

### TUNNEL
Diagnostic mode for tip positioning:

```
TUNNEL
```

### PARAMETER
Set parameters:

```
PARAMETER,kI,kP,targetCurrent,tolerance,startX,startY,direction,maxX,maxY,multiplier
```

Query current parameters:

```
PARAMETER,?
```

---

### SET_SSID / SET_PASSWORD
Set Wi‑Fi credentials via the serial command interface and saves it into ESP

Set SSID:
```
SET_SSID,MyWlanName
```

Set password:
```
SET_PASSWORD,MyWlanPassword
```

Notes:
- After providing both commands over USB serial, reboot the ESP to apply the new credentials (the device reads saved values from NVS on boot).

## 🔐 WiFi Provisioning (set credentials to enter STATION mode)

You can configure WiFi credentials in two ways: via the USB serial command interface or via the device HTTP provisioning endpoint. The device stores credentials in NVS and will attempt to connect as a station on next boot while keeping an AP available for provisioning.

1) USB (serial)

- Connect to the device serial port at `460800` (PlatformIO Monitor, `picocom`, `miniterm`, etc.).
- Send these two lines (each terminated with newline `\n`):

```
SET_SSID MyNetworkName
SET_PASSWORD MySecretPass
```

- The input preserves case and internal spaces; do not alter your SSID casing or spacing.
- After both commands succeed, power-cycle or reboot the device so it reads the new credentials and attempts STA connect.

2) HTTP provisioning (AP mode)

- While the device is in AP/provisioning mode (default AP IP: `192.168.4.1`), POST to `/setwifi` with form body `ssid=...&pass=...`.

Example using curl:

```bash
curl -X POST http://192.168.4.1/setwifi \
  -d "ssid=MyNetworkName&pass=MySecretPass"
```

- On success the device will attempt to connect immediately and saves the credentials to NVS.

Notes:
- After saving credentials via USB you must reboot the device to cause the STA connection attempt (HTTP provisioning triggers an immediate try).
- The device runs in AP+STA mode (AP remains available for provisioning even when STA is connected).
- Credentials are stored in NVS and persist across reboots.

---




## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push and open a Pull Request

---

## 📄 License

This project is licensed under the terms specified in the [LICENSE](LICENSE) file.

---

## 👨‍💻 Author

**Peter Dirnhofer**

---
