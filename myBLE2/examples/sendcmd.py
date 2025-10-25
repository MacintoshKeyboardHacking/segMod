#!/usr/bin/python3
#
# sends a raw command.  automatically computes the checksum, but not the length field
# ./sendcmd 5aa5023e1601550200ffff would read 2 bytes from ECU 0x16 register 0x55.
# no provision is given to display the results however
#
# https://www.youtube.com/@MacintoshKeyboardHacking/streams

import serial
import signal

import signal
import subprocess


import sys
from time import sleep


count=0


def handler(signum, frame):
    exit(1)

# save on exit
signal.signal(signal.SIGINT, handler)

ser = serial.Serial("/dev/ttyUSB0")
ser.baudrate = 115_200
ser.bytesize = serial.EIGHTBITS  # number of bits per bytes
ser.parity = serial.PARITY_NONE  # set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE  # number of stop bits
# ser.timeout = None  # block read
ser.timeout = 1 / 100  # non-block read
ser.xonxoff = False  # disable software flow control
ser.rtscts = False  # disable hardware (RTS/CTS) flow control
ser.dsrdtr = False  # disable hardware (DSR/DTR) flow control
ser.writeTimeout = 1  # timeout for write

cmdZ = b"\x5A\xA5\x00\x04\x16\x7A\x00\x6B\xFF"  # 0416:7a00

fuzDly = 10
fuzTim = fuzDly

debug=1
transmit=1
verbose=1


doWrite=0


cmdS = b"\x5a\xa5\x06\x3e\x16\x03\x46\x50\x32\x19\x00\x50\x32\xFF\xFF"
cmdS = b"\x5a\xa5\x06\x3e\x16\x03\x46\x11\x11\x11\x11\x11\x11\xFF\xFF"
cmdS = b"\x5a\xa5\x04\x3e\x16\x03\x47\xcc\xcc\xcc\xcc\xFF\xFF"
cmdZ = bytearray(cmdS)
#bytes.fromhex(hex_string)
#print(bytes_object)

#myIn=sys.argv[1].fromhex(hex_string)
myIn=bytes.fromhex(sys.argv[1])
cmdZ = bytearray(myIn)
cmdY = bytearray(bytes.fromhex("5aa5003e1680002bff"))

for q in range(255,256):
#    cmdZ[5]=q

    sum = 0
    stl=len(cmdZ)
    for i in range(2, (stl-2)):
        sum += cmdZ[i]
        resum = 0xFFFF - (sum & 0xFFFF)
    cmdZ[stl-2] = resum & 0xFF
    cmdZ[stl-1] = (resum >> 8)&0xff


#    print("going with " + cmdY.hex())
    print("going with " + cmdZ.hex())
#    print("I got "+myIn)
#    ser.write(cmdY)
#    sleep(.25)
    ser.write(cmdZ)
#    sleep(1)

