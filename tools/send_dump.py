#!/usr/bin/env python3
"""
Send DUMP to a serial port and print response. Requires pyserial: pip install pyserial
Usage: python tools/send_dump.py [COM9] [115200]
"""
import sys
import time
try:
    import serial
except ImportError:
    print('pyserial not installed. Run: pip install pyserial')
    sys.exit(1)

port = sys.argv[1] if len(sys.argv) > 1 else 'COM9'
baud = int(sys.argv[2]) if len(sys.argv) > 2 else 115200

with serial.Serial(port, baud, timeout=1) as ser:
    # Wait a bit longer after opening the port so the device can finish
    # any automatic reset sequence and become ready to receive commands.
    time.sleep(3.0)
    ser.write(b'DUMP\n')
    # Read for up to 2 seconds collecting output
    end = time.time() + 2.0
    chunks = []
    while time.time() < end:
        data = ser.read(1024)
        if data:
            chunks.append(data)
        else:
            time.sleep(0.05)
    out = b''.join(chunks)
    try:
        print(out.decode('utf-8', errors='replace'))
    except Exception:
        print(out)
