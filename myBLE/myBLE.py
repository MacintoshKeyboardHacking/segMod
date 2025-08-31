#!/usr/bin/python3
# https://www.youtube.com/@MacintoshKeyboardHacking/streams

import serial
import binascii
import signal
from time import sleep

ECU = {}
ECU[0x04] = bytes(bytearray(0x100 * 2))
ECU[0x06] = bytes(bytearray(0x100 * 2))
ECU[0x07] = bytes(bytearray(0x100 * 2))
ECU[0x16] = bytes(bytearray(0x100 * 2))
ECU[0x20] = bytes(bytearray(0x100 * 2))
ECU[0x21] = bytes(bytearray(0x100 * 2))
ECU[0x22] = bytes(bytearray(0x100 * 2))
ECU[0x23] = bytes(bytearray(0x100 * 2))
ECU[0x3E] = bytes(bytearray(0x100 * 2))


def handler(signum, frame):
    print("lunch")
    with open("ecu.bin", "wb") as outL:
        outL.write(ECU[0x04])
        outL.write(ECU[0x06])
        outL.write(ECU[0x07])
        outL.write(ECU[0x16])
        outL.write(ECU[0x20])
        outL.write(ECU[0x21])
        outL.write(ECU[0x22])
        outL.write(ECU[0x23])
        outL.write(ECU[0x3E])
    exit(1)


signal.signal(signal.SIGINT, handler)

ser = serial.Serial("/dev/ttyUSB0")
# ser.port = "/dev/ttyUSB0"
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


# ser2 = serial.Serial("/dev/ttyUSB1")
# ser2.baudrate = 115_200
# ser2.bytesize = serial.EIGHTBITS  # number of bits per bytes
# ser2.parity = serial.PARITY_NONE  # set parity check: no parity
# ser2.stopbits = serial.STOPBITS_ONE  # number of stop bits
# ser2.timeout = (1 / 100)  # non-block read
# ser2.xonxoff = False  # disable software flow control
# ser2.rtscts = False  # disable hardware (RTS/CTS) flow control
# ser2.dsrdtr = False  # disable hardware (DSR/DTR) flow control
# ser2.writeTimeout = 1  # timeout for write

ser2 = ser

oDat = bytearray(64)

with open("ecu.bin", mode="rb") as file:
    ECU[0x04] = bytearray(file.read(0x100 * 2))
    ECU[0x06] = bytearray(file.read(0x100 * 2))
    ECU[0x07] = bytearray(file.read(0x100 * 2))
    ECU[0x16] = bytearray(file.read(0x100 * 2))
    ECU[0x20] = bytearray(file.read(0x100 * 2))
    ECU[0x21] = bytearray(file.read(0x100 * 2))
    ECU[0x22] = bytearray(file.read(0x100 * 2))
    ECU[0x23] = bytearray(file.read(0x100 * 2))
    ECU[0x3E] = bytearray(file.read(0x100 * 2))
    file.close()

cmd0 = b"\x5A\xA5\x00\x04\x16\x7A\x00\x6B\xFF"
cmd1 = b"\x5A\xA5\x00\x04\x16\x7B\x02\x68\xFF"
# cmd0=cmd1

# keepalive ECU 23 > 16
cmdZ = b"\x5a\xa5\x00\x23\x16\x01\xfc\xc9\xfe"
cmdZ = b"\x5a\xa5\x00\x23\x16\x01\xfc\xc9\xfe"
# cmdZ=b'\x5a\xa5\x00\x23\x16\x01\xfa\xcb\xfe'

fuzAdr = 0x10
fuzAdr = 0x0
fuzDly = 100
fuzTim = fuzDly

stall = 1

print("going with " + cmdZ.hex())

with open("eculog.bin", "wb") as outLog:
    iBuf = ""
    idleTime = 10
    activity = 0
    toggle = 0
    flipflop = 0
    while fuzAdr < 0x100:
        if toggle:
            iDat = ser.read(4096)
        else:
            iDat = ser2.read(4096)

        if iDat:
            print("RX"+str(toggle)+": "+ bytes(iDat).hex())

        outLog.write(iDat)
        toggle = 1 - toggle

        # validate packet header
        while len(iDat) > 6:
            if (iDat[0] == 0x5A) & (iDat[1] == 0xA5):
                rLen = iDat[2]  # data len, should be checked to see if RX is complete
                rSrc = iDat[3]  # source MCU
                rRsp = iDat[4]  # target MCU
                rCmd = iDat[5]  # CMD
                rAdr = iDat[6]  # 8bit page selection

                if (rSrc == 0x16) & (rRsp == 0x04) & ((rCmd == 0x7B)):
                    stall = 0

                # write, write-NR
                if (rCmd == 0x02) | (rCmd == 0x03):
                    stat = (
                        hex(rSrc)[2:] + ":" + hex(rRsp)[2:] + " " + hex(rAdr)[2:] + "="
                    )

                    # store bytes
                    for i in range(0, rLen):
                        if ((rAdr * 2) + i) < 512:
                            ECU[rRsp][(rAdr * 2) + i] = iDat[i + 7]
                        stat = stat + hex(iDat[i + 7])[2:] + "-"

                    # behavior? If odd# bytes, assume to add MSB=0?
                    if rLen & 1:
                        if ((rAdr * 2) + rLen) < 512:
                            ECU[rRsp][(rAdr * 2) + rLen] = 0

                    # send write confirmation
                    if rCmd == 0x02:
                        stat = stat + "!"
                        rP = bytearray(bytes(10))
                        rP[0] = 0x5A
                        rP[1] = 0xA5
                        rP[2] = 0x01
                        rP[3] = rRsp
                        rP[4] = rSrc
                        rP[5] = 0x05  # write response
                        rP[6] = rAdr
                        rP[7] = 1  # success

                        sum = 0
                        for i in range(2, 8):
                            sum += rP[i]
                        resum = 0xFFFF - (sum & 0xFFFF)
                        rP[8] = resum & 0xFF
                        rP[9] = resum >> 8

                        ser.write(rP)
                    # k                        activity = idleTime
                    print(stat)

                # read-response
                elif rCmd == 0x04:
                    stat = (
                        hex(rSrc)[2:] + ":" + hex(rRsp)[2:] + " " + hex(rAdr)[2:] + ":"
                    )
                    for i in range(0, rLen):
                        if ((rAdr * 2) + i) < 512:
                            ECU[rSrc][(rAdr * 2) + i] = iDat[i + 7]
                        stat = stat + hex(iDat[i + 7])[2:] + "-"

                    # behavior? If odd# bytes, assume to add MSB=0?
                    if rLen & 1:
                        if ((rAdr * 2) + rLen) < 512:
                            ECU[rSrc][(rAdr * 2) + rLen] = 0
                    print(stat)

                # respond to read request using our ecu.bin
                elif rCmd == 0x01:
                    rLen = iDat[7]

                    oDat = bytearray(bytes(rLen + 9))
                    oDat[0] = 0x5A
                    oDat[1] = 0xA5
                    oDat[2] = rLen
                    oDat[3] = rRsp
                    oDat[4] = rSrc
                    oDat[5] = 0x04  # read response
                    oDat[6] = rAdr

                    stat = (
                        hex(rSrc)[2:] + ":" + hex(rRsp)[2:] + " " + hex(rAdr)[2:] + "?"
                    )
                    for i in range(0, rLen):
                        if ((rAdr * 2) + i) < 512:
                            oDat[i + 7] = ECU[rRsp][(rAdr * 2) + i]
                            stat = stat + hex(ECU[rRsp][(rAdr * 2) + i])[2:] + "-"

                    sum = 0
                    for i in range(2, rLen + 7):
                        sum += oDat[i]
                    resum = 0xFFFF - (sum & 0xFFFF)
                    oDat[rLen + 7] = resum & 0xFF
                    oDat[rLen + 8] = resum >> 8

                    width = 0x08
                    for i in range(0, len(oDat), width):
                        chunk = oDat[i : i + width]
                        hex_repr = " ".join(f"{byte:02x}" for byte in chunk)
                        ascii_repr = "".join(
                            chr(byte) if 32 <= byte <= 126 else "." for byte in chunk
                        )
                        ser.write(oDat[i : i + width])
                    # k                        activity = idleTime

                    print(stat)
                else:
                    print("??: " + hex(rCmd) + " " + hex(rAdr) + " (" + hex(rLen) + ")")
            iDat = iDat[1:]  # make this less dumb

        if activity:
            activity -= 1
        else:
#            stall = 0
            if flipflop:
                ser.write(cmd1)
                if stall == 0:
                    ser.write(cmdZ)
            else:
                ser.write(cmd0)
                if stall == 0:
                    ser.write(cmdZ)
            flipflop = 1 - flipflop
            activity = idleTime

#        stall = 1
        if fuzTim:
            if stall == 0:
                fuzTim -= 1
        else:
            fuzTim = fuzDly
            stall = 1

            rP = bytearray(bytes(10))
            rP[0] = 0x5A
            rP[1] = 0xA5
            rP[2] = 0x01
            rP[3] = 0x23
            rP[4] = 0x16
            rP[5] = 0x01
            rP[6] = 0xFD
            rP[7] = 0x02  # bytes to read

            sum = 0
            for i in range(2, 8):
                sum += rP[i]
            resum = 0xFFFF - (sum & 0xFFFF)
            rP[8] = resum & 0xFF
            rP[9] = resum >> 8

            stat = ""
            for i in range(0, len(rP)):
                stat = stat + hex(rP[i]) + "-"

            print("set " + stat)
            #                        foo=bytearray(b'\x5a\xa5\xFF\x23\x16\x02\xfd\x01\x0a\x89\x52\x76\x7e\x10\x89\x7a\xaa\xdd\x10\xc5\x0a\x44\xac\x10\xb3\xd1\xdd\xa3\x10\xb2\xd1\xdd\xa3\x10\xad\xd1\xdd\xa3\x10\xac\xd1\xdd\xa3\x10\xaf\xd1\xdd\xa3\x10\x44\x6c\xe5\x6f\x10\xae\x48\x4a\xad\x10\x9e\xf9\x96\x90\x10\xe4\x65\x9b\x9c\x10\x92\x00\x9a\x22\x10\x64\xd0\xf6\xa4\x08\x12\x45\x6a\x61\x08\x70\xe0\xe2\xb3\x08\x8a\x38\x99\xe7\x08\xda\xb2\x89\x86\x04\xa1\x24\x01\x7e\x04\x4b\xcb\xf9\xf8\x04\x75\x1d\xbc\xa2\x04\x9f\x6f\x41\x42\x01\x98\x8d\xb2\x17\x01\x29\x0a\xbc\xcf\x01\xcc\xba\xe7\x88\x01\xe5\xe9\x99\x0c\x01\x61\xe9\x41\x18\x01\x8a\xa8\xb5\x16\x01\xb3\xd7\xef\x70\x01\x03\x26\x1a\x75\x01\x39\xb0\xbf\x06\x01\x3b\x26\xe8\x20\x01\x55\x25\x1a\x7f\x01\x2b\x71\x39\xf7\x01\x6b\x64\x4e\xac\x01\x19\xbd\x86\x00\x01\x1a\x93\x7d\x0c\x01\xFF\xFF')
            # 16:23 fe=1-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-16-0-0-0-0-0-0-0-5b-5b-5b-0-3-0-88-0-
            # 16:23 fe=1-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-0-11-0-0-0-0-0-0-0-5b-5b-5b-0-3-0-88-0-
            foo = bytearray(
                b"\x5a\xa5\xFF\x23\x16\x01\xfd\x02\x0a"
                +
                                            b'\x89\x52\x76\x7e\x10'+\
                # drive mode 92=sport, 33=race
                                            b'\x89\x7a\xaa\xdd\x10'+\
                                            b'\xc5\x0a\x44\xac\x10'+\
                                            b'\xb3\xd1\xdd\xa3\x10'+\
                                            b'\xb2\xd1\xdd\xa3\x10'+\
                                            b'\xad\xd1\xdd\xa3\x10'+\
                                            b'\xac\xd1\xdd\xa3\x10'+\
                                            b'\xaf\xd1\xdd\xa3\x10'+\
                # MPH
                b"\x44\x6c\xe5\x6f\x10"
                + b"\xae\x48\x4a\xad\x10"
                + b"\x9e\xf9\x96\x90\x10"
                + b"\xe4\x65\x9b\x9c\x10"
                + b"\x92\x00\x9a\x22\x10"
                +
                # batt percent
                b"\x64\xd0\xf6\xa4\x08"
                + b"\x12\x45\x6a\x61\x08"
                + b"\x70\xe0\xe2\xb3\x08"
                + b"\x8a\x38\x99\xe7\x08"
                +
                # mode
                b"\xda\xb2\x89\x86\x04"
                + b"\xa1\x24\x01\x7e\x04"
                + b"\x4b\xcb\xf9\xf8\x04"
                + b"\x75\x1d\xbc\xa2\x04"
                + b"\x9f\x6f\x41\x42\x01"
                + b"\x98\x8d\xb2\x17\x01"
                + b"\x29\x0a\xbc\xcf\x01"
                + b"\xcc\xba\xe7\x88\x01"
                + b"\xe5\xe9\x99\x0c\x01"
                + b"\x61\xe9\x41\x18\x01"
                + b"\x8a\xa8\xb5\x16\x01"
                + b"\xb3\xd7\xef\x70\x01"
                + b"\x03\x26\x1a\x75\x01"
                + b"\x39\xb0\xbf\x06\x01"
                + b"\x3b\x26\xe8\x20\x01"
                + b"\x55\x25\x1a\x7f\x01"
                + b"\x2b\x71\x39\xf7\x01"
                + b"\x6b\x64\x4e\xac\x01"
                + b"\x19\xbd\x86\x00\x01"
                + b"\x1a\x93\x7d\x0c\x01"
                + b"\xFF\xFF"
            )

            # fix packet length and CRC
            foo[2] = len(foo) - 9

            sum = 0
            for i in range(2, len(foo) - 2):
                sum += foo[i]
            resum = 0xFFFF - (sum & 0xFFFF)
            rLo = resum & 0xFF
            rHi = resum >> 8
            foo[len(foo) - 2] = rLo
            foo[len(foo) - 1] = rHi

            #                        foo=b'\x5a\xa5\xbb\x04\x16\x02\xfd\x01\x0a\x89\x52\x76\x7e\x10\x89\x7a\xaa\xdd\x10\xc5\x0a\x44\xac\x10\xb3\xd1\xdd\xa3\x10\xb2\xd1\xdd\xa3\x10\xad\xd1\xdd\xa3\x10\xac\xd1\xdd\xa3\x10\xaf\xd1\xdd\xa3\x10\x44\x6c\xe5\x6f\x10\xae\x48\x4a\xad\x10\x9e\xf9\x96\x90\x10\xe4\x65\x9b\x9c\x10\x92\x00\x9a\x22\x10\x64\xd0\xf6\xa4\x08\x12\x45\x6a\x61\x08\x70\xe0\xe2\xb3\x08\x8a\x38\x99\xe7\x08\xda\xb2\x89\x86\x04\xa1\x24\x01\x7e\x04\x4b\xcb\xf9\xf8\x04\x75\x1d\xbc\xa2\x04\x9f\x6f\x41\x42\x01\x98\x8d\xb2\x17\x01\x29\x0a\xbc\xcf\x01\xcc\xba\xe7\x88\x01\xe5\xe9\x99\x0c\x01\x61\xe9\x41\x18\x01\x8a\xa8\xb5\x16\x01\xb3\xd7\xef\x70\x01\x03\x26\x1a\x75\x01\x39\xb0\xbf\x06\x01\x3b\x26\xe8\x20\x01\x55\x25\x1a\x7f\x01\x2b\x71\x39\xf7\x01\x6b\x64\x4e\xac\x01\x19\xbd\x86\x00\x01\x1a\x93\x7d\x0c\x01\x56\xaf'
            cmdZ = foo
            fuzAdr += 1
# k                        activity = idleTime

handler(0, 0)   # save the ECU
ser.close()     # close port
exit()
