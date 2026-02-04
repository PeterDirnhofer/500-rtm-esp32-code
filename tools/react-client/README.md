# ESP SSE Client (minimal)

- Open `tools/react-client/index.html` in a browser after connecting to ESP AP.
- ESP AP default SSID: `ESP_Server`, password: `esp12345`.
- The client uses `EventSource('/events')` to receive server-sent events and `POST /send` to transmit commands.

If the ESP is in AP mode, access the client at `http://192.168.4.1/index.html`.
If the ESP is connected to your router in STA mode, find its IP from the router (FRITZ!Box) and open `http://<esp-ip>/index.html`.
