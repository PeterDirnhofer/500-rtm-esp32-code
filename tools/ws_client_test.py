import websocket
import sys

try:
    ws = websocket.create_connection("ws://192.168.178.58/ws", timeout=5)
    print("Connected")
    ws.send("PING_FROM_WSCLIENT")
    print("Sent")
    ws.settimeout(5)
    msg = ws.recv()
    print("Received:", msg)
    ws.close()
except Exception as e:
    print("Error:", e)
    sys.exit(1)
