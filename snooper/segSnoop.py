#!/usr/bin/python3
# https://www.youtube.com/@macintoshkeyboardhacking/streams

import serial
import binascii
import signal
from time import sleep

ECU = {}
ECU[0x20] = bytes(bytearray(0x100*2))
ECU[0x21] = bytes(bytearray(0x100*2))
ECU[0x22] = bytes(bytearray(0x100*2))
ECU[0x23] = bytes(bytearray(0x100*2))

#ECU=bytes(bytearray(0x100*2))


def handler(signum, frame):
    print("lunch")
    with open("ecu.bin", "wb") as outL:
        outL.write(ECU[0x20])
        outL.write(ECU[0x21])
        outL.write(ECU[0x22])
        outL.write(ECU[0x23])
    exit(1)


signal.signal(signal.SIGINT, handler)

ser1=serial.Serial("/dev/ttyUSB1")
#ser.port = "/dev/ttyUSB0"
ser1.baudrate = 115_200
ser1.bytesize = serial.EIGHTBITS  # number of bits per bytes
ser1.parity = serial.PARITY_NONE  # set parity check: no parity
ser1.stopbits = serial.STOPBITS_ONE  # number of stop bits
#ser1.timeout = None  # block read
ser1.timeout = 1            #non-block read
ser1.xonxoff = False  # disable software flow control
ser1.rtscts = False  # disable hardware (RTS/CTS) flow control
ser1.dsrdtr = False  # disable hardware (DSR/DTR) flow control
ser1.writeTimeout = 1  # timeout for write


ser2=serial.Serial("/dev/ttyUSB0")
#ser.port = "/dev/ttyUSB0"
ser2.baudrate = 115_200
ser2.bytesize = serial.EIGHTBITS  # number of bits per bytes
ser2.parity = serial.PARITY_NONE  # set parity check: no parity
ser2.stopbits = serial.STOPBITS_ONE  # number of stop bits
#ser1.timeout = None  # block read
ser2.timeout = 1            #non-block read
ser2.xonxoff = False  # disable software flow control
ser2.rtscts = False  # disable hardware (RTS/CTS) flow control
ser2.dsrdtr = False  # disable hardware (DSR/DTR) flow control
ser2.writeTimeout = 1  # timeout for write


tBuf=bytearray()
iBuf=bytearray()
oBuf=bytearray()

oDat = bytearray(64)
meCnt = 0

with open("ecu.bin", mode="rb") as file:
    ECU[0x20] = bytearray(file.read(0x100*2))
    ECU[0x21] = bytearray(file.read(0x100*2))
    ECU[0x22] = bytearray(file.read(0x100*2))
    ECU[0x23] = bytearray(file.read(0x100*2))
    file.close()


with open("outlog.bin", "wb") as outLog:
    while True:
        # wait for something to respond to
#        response = str(ser1.readline())
        iDat = bytearray(ser1.readline())
        if len(iDat):
            iBuf = iBuf+iDat
            iStart=0
            for i in range(0, len(iBuf)-2):
              if ( (iBuf[i+1]==0x5a) & (iBuf[i+2]==0xa5) ):
                width=(i-iStart)
                chunk = iBuf[iStart : i+1]
                iStart=i+1
                hex_repr = " ".join(f"{byte:02x}" for byte in chunk)
                ascii_repr = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in chunk)
                print(f'RX: {hex_repr.ljust(width * 3)} |{ascii_repr}|')
            iBuf=iBuf[iStart:]

        oDat = bytearray(ser2.readline())
        if len(oDat):
#            print("READING: "+str(len(oBuf)))
            oBuf = oBuf+oDat
            oStart=0
            for i in range(0, len(oBuf)-2):
              if ( (oBuf[i+1]==0x5a) & (oBuf[i+2]==0xa5) ):
                width=(i-oStart)
                chunk = oBuf[oStart : i+1]
                oStart=i+1
                hex_repr = " ".join(f"{byte:02x}" for byte in chunk)
                ascii_repr = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in chunk)
                print(f'TX: {hex_repr.ljust(width * 3)} |{ascii_repr}|')
#    print(f"{i:08x}: {hex_repr.ljust(width * 3)} |{ascii_repr}|")
            oBuf=oBuf[oStart:]





ser1.close()  # close port
ser2.close()  # close port

exit()


width = 0x10
for i in range(0, len(ECU[33]), width):
    chunk = ECU[33][i : i + width]
    hex_repr = " ".join(f"{byte:02x}" for byte in chunk)
    ascii_repr = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in chunk)
    print(f"{i:08x}: {hex_repr.ljust(width * 3)} |{ascii_repr}|")
