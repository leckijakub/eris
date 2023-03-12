#!/usr/bin/env python3

import serial
import time
ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
ser.write(b"test\r")
print(ser.readall())
