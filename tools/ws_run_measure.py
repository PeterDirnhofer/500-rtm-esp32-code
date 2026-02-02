#!/usr/bin/env python3
"""
Connect to ws://host:port/path, send a masked text frame with payload "MEASURE",
then print incoming text frames until DATA,DONE is received or timeout.
Usage: python tools/ws_run_measure.py <host> <port> <path>
"""
import sys, socket, base64, os, struct, time

if len(sys.argv) < 4:
    print("Usage: python tools/ws_run_measure.py <host> <port> <path>")
    sys.exit(2)

host = sys.argv[1]
port = int(sys.argv[2])
path = sys.argv[3]

key = base64.b64encode(os.urandom(16)).decode("ascii")
req = (
    f"GET {path} HTTP/1.1\r\n"
    f"Host: {host}:{port}\r\n"
    "Upgrade: websocket\r\n"
    "Connection: Upgrade\r\n"
    f"Sec-WebSocket-Key: {key}\r\n"
    "Sec-WebSocket-Version: 13\r\n"
    "\r\n"
)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    s.settimeout(15)
    s.connect((host, port))
    s.sendall(req.encode('utf-8'))
    buf = b''
    while b"\r\n\r\n" not in buf:
        chunk = s.recv(1024)
        if not chunk:
            break
        buf += chunk
    headers = buf.split(b"\r\n\r\n", 1)[0].decode('utf-8', errors='ignore')
    print('--- Server response ---')
    print(headers)
    if '101' not in headers.split('\r\n')[0]:
        print('Did not get 101 Switching Protocols; aborting')
        s.close()
        sys.exit(1)

    # send MEASURE command as masked text
    payload = b"MEASURE SIMULATE"
    fin_opcode = 0x81
    plen = len(payload)
    if plen <= 125:
        header = bytes([fin_opcode, 0x80 | plen])
    elif plen <= 0xFFFF:
        header = bytes([fin_opcode, 0x80 | 126]) + struct.pack('!H', plen)
    else:
        header = bytes([fin_opcode, 0x80 | 127]) + struct.pack('!Q', plen)
    mask = os.urandom(4)
    masked = bytearray(payload)
    for i in range(plen):
        masked[i] ^= mask[i % 4]
    s.sendall(header + mask + masked)
    print('Sent MEASURE SIMULATE')

    s.settimeout(120)
    start = time.time()
    while True:
        # read 2 bytes
        h = s.recv(2)
        if not h or len(h) < 2:
            print('No more frames or connection closed')
            break
        b1, b2 = h[0], h[1]
        masked_flag = (b2 & 0x80) != 0
        length = b2 & 0x7F
        if length == 126:
            ext = s.recv(2)
            length = struct.unpack('!H', ext)[0]
        elif length == 127:
            ext = s.recv(8)
            length = struct.unpack('!Q', ext)[0]
        mask_key = b''
        if masked_flag:
            mask_key = s.recv(4)
        data = b''
        while len(data) < length:
            chunk = s.recv(length - len(data))
            if not chunk:
                break
            data += chunk
        if masked_flag and mask_key:
            unmasked = bytearray(data)
            for i in range(len(unmasked)):
                unmasked[i] ^= mask_key[i % 4]
            data = bytes(unmasked)
        opcode = b1 & 0x0F
        if opcode == 1:
            txt = data.decode('utf-8', errors='ignore')
            print('RECV:', txt)
            if 'DONE' in txt or 'DATA_COMPLETE' in txt or 'DATA,DONE' in txt:
                print('Measurement finished')
                break
        elif opcode == 8:
            print('Close frame received')
            break
        # safety timeout
        if time.time() - start > 115:
            print('Timeout waiting for data')
            break

finally:
    s.close()

