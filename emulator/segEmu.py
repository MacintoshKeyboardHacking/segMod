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

ser = serial.Serial("/dev/ttyUSB0")
ser.port = "/dev/ttyUSB0"
ser.baudrate = 115_200
ser.bytesize = serial.EIGHTBITS  # number of bits per bytes
ser.parity = serial.PARITY_NONE  # set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE  # number of stop bits
ser.timeout = None  # block read
# ser.timeout = 1            #non-block read
ser.xonxoff = False  # disable software flow control
ser.rtscts = False  # disable hardware (RTS/CTS) flow control
ser.dsrdtr = False  # disable hardware (DSR/DTR) flow control
ser.writeTimeout = 1  # timeout for write

# print(ser.name)         # check which port was really used

oDat = bytearray(64)
meCnt = 0

with open("ecu.bin", mode="rb") as file:
    ECU[0x20] = bytearray(file.read(0x100*2))
    ECU[0x21] = bytearray(file.read(0x100*2))
    ECU[0x22] = bytearray(file.read(0x100*2))
    ECU[0x23] = bytearray(file.read(0x100*2))
    file.close()

#ECU[0x20][0x1ba]=0x30
#ECU[0x20][0x1bb]=0x30
#ECU[0x20][0x1bc]=0x30
#ECU[0x20][0x1bd]=0x30
#ECU[0x20][0x1be]=0x30
#ECU[0x20][0x1bf]=0x30

with open("destination.bin", "wb") as outLog:
    while True:
        # wait for something to respond to
        response = str(ser.readline())
        print("-=> " + response)
        xA = response.split(":")
        if len(xA) == 3:
            # confirm it's RX:data: from the ESP32
            iDat = bytearray(binascii.unhexlify(xA[1].rstrip()))
            outLog.write(iDat)

            # Ninebot protocol
            if (iDat[0] == 0x5A):
                rSrc = iDat[3]  # source MCU
                rRsp = iDat[4]  # target MCU
                rCmd = iDat[5]  # CMD
                rAdr = iDat[6]  # 8bit page selection

                if (rCmd == 0x02) | (rCmd == 0x03) | (rCmd == 0x50):
                    rLen = iDat[2]  # data len

                    print("WRITING "+str(rRsp)+":"+str(rLen)+"@"+str(rAdr))
                    if rRsp == 0x50:
                        rRsp = 0x21

                    # store bytes
                    for i in range(0, rLen):
                        ECU[rRsp][(rAdr*2) + i] = iDat[i + 7]

                    # behavior? If odd# bytes, add 0
                    if (rLen&1):
                        ECU[rRsp][(rAdr*2) + rLen] = 0

                    # write confirmation
                    if (rCmd == 0x02):
                    #if (True):
                        rP = bytearray(bytes(10))
                        rP[0:2] = {0x5A, 0xA5, 0x01}
                        rP[3] = rRsp
                        rP[4] = rSrc
                        rP[5] = 0x05    # write response
                        rP[6] = rAdr
                        rP[7] = 1       # success

                        sum = 0
                        for i in range(2, 8):
                            sum += rP[i]
                        resum = 0xFFFF - (sum & 0xFFFF)
                        rP[8] = resum & 0xFF
                        rP[9] = resum >> 8

                        print("WR ACK")
                        ser.write(rP)
                        sleep(25 / 1000)


                elif (rCmd != 0x01):
                    print("mismatch CMD")
                else:
                    # READING
#                    rRsp=0x20
                    rLen = iDat[7]

                    #                if (rLen >16): rLen=16
                    rP = bytearray(bytes(rLen + 9))
                    rP[0:2] = {0x5A, 0xA5}
                    rP[2] = rLen
                    rP[3] = rRsp
                    rP[4] = rSrc
                    rP[5] = 0x04    # read response
                    rP[6] = rAdr

                    for i in range(0, rLen):
                        rP[i+7] = ECU[rRsp][(rAdr*2) + i]

                    # CMD_CMAP_ACK_RD=0x04    # Response packet to instructon reading
                    sum = 0
                    for i in range(2, rLen + 7):
                        sum += rP[i]
                    resum = 0xFFFF - (sum & 0xFFFF)
                    rP[rLen + 7] = resum & 0xFF
                    rP[rLen + 8] = resum >> 8

                    oDat = rP
                    width = 0x08
                    for i in range(0, len(oDat), width):
                        chunk = oDat[i : i + width]
                        hex_repr = " ".join(f"{byte:02x}" for byte in chunk)
                        ascii_repr = "".join(
                            chr(byte) if 32 <= byte <= 126 else "." for byte in chunk
                        )
                        #print(f'{i:08x}: {hex_repr.ljust(width * 3)} |{ascii_repr}|')
#chunked output=good?bad? behavior
                        ser.write(oDat[i : i + width])
                        sleep(25 / 1000)

#                    ser.write(oDat)
#                    sleep(25 / 1000)

ser.close()  # close port

exit()


width = 0x10
for i in range(0, len(ECU[33]), width):
    chunk = ECU[33][i : i + width]
    hex_repr = " ".join(f"{byte:02x}" for byte in chunk)
    ascii_repr = "".join(chr(byte) if 32 <= byte <= 126 else "." for byte in chunk)
    print(f"{i:08x}: {hex_repr.ljust(width * 3)} |{ascii_repr}|")
