#!/usr/bin/python3
#
# I used this to determine which variable locations were writable; the procedure is read 2 bytes, write whatever was received back
# with "status completion" and look at the returned "write acknowledge" status
# I was interested to see that VCU and BMS have different "write ack" response packet format
#
# myBLEv2; mbt.py MIT license
# a Segway "3 series" BLE display replacement project
#
# these are functions I used while debuging, it's a big big mess
# of re-worked bugs and associated dead code.  Enjoy! ;)
#
# for demo and discussion see my livestreams, https://www.youtube.com/@MacintoshKeyboardHacking/streams

import serial
import signal

import signal
import subprocess

from time import sleep



# bridge config
mitm=0      # pass-thru external BLE
verbose=1   # don't filter realtime update packets, include verbose BLE output
debug=1     # show extended protocols
sniff=1     # display "everything" versus "only display changed data"


transmit=1  # active scanning
destECU=0x16    # target ECU to scan
destECU=0x07    # target ECU to scan
destECU=0x04    # target ECU to scan
fuzAdr = 0x00    # start addr of scanning

fuzDly = 100
fuzTim = fuzDly
idleTime = 100


doWrite=0   # enable custom packet TX (set cmdZ below)
cmdZ = b"\x5a\xa5\x00\x23\x16\x01\xfc\xc9\xfe"  # 2316:read, ECU keepalive
cmdZ=bytearray(b'\x5a\xa5\xFF\x3e\x04\x03\x51\x01\x00\x89\x52') # 04:51 =0000 (off)


heartbeat=0 # generate 7a/7b cmds OR 
cmd0 = b"\x5A\xA5\x00\x04\x16\x7A\x01\x6A\xFF"  # 0416:7a01, heartbeat
cmd1 = b"\x5A\xA5\x00\x04\x16\x7B\x02\x68\xFF"  # 0416:7b02, heartbeat

# stall=1 belongs to an old python keepalive routine, esp32 handles this now
stall=0
block=0
count=0

toggle = 0
flipflop = 0
flipper=0
activity = 0


# basic ECU emulation, used to identify updated data
ECU = {}
ECU[0x04] = bytes(bytearray(0x100 * 2))
ECU[0x06] = bytes(bytearray(0x100 * 2))
ECU[0x07] = bytes(bytearray(0x100 * 2))
ECU[0x16] = bytes(bytearray(0x100 * 2))
ECU[0x23] = bytes(bytearray(0x100 * 2))
ECU[0x3E] = bytes(bytearray(0x100 * 2))


# quit the script, backup the ECU data
def handler(signum, frame):
    with open("ecu.bin", "wb") as outL:
        outL.write(ECU[0x04])
        outL.write(ECU[0x07])
        outL.write(ECU[0x16])
        outL.write(ECU[0x23])
        outL.write(ECU[0x3E])
    print("lunch")
    exit(1)

# save on exit
signal.signal(signal.SIGINT, handler)

ser = serial.Serial("/dev/ttyUSB1")
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


if mitm:
    ser2 = serial.Serial("/dev/ttyUSB1")
    ser2.baudrate = 115_200
    ser2.bytesize = serial.EIGHTBITS  # number of bits per bytes
    ser2.parity = serial.PARITY_NONE  # set parity check: no parity
    ser2.stopbits = serial.STOPBITS_ONE  # number of stop bits
    ser2.timeout = (1 / 100)  # non-block read
    ser2.xonxoff = False  # disable software flow control
    ser2.rtscts = False  # disable hardware (RTS/CTS) flow control
    ser2.dsrdtr = False  # disable hardware (DSR/DTR) flow control
    ser2.writeTimeout = 1  # timeout for write


# populate ECU data on startup
with open("ecu.bin", mode="rb") as file:
    ECU[0x04] = bytearray(file.read(0x100 * 2))
    ECU[0x07] = bytearray(file.read(0x100 * 2))
    ECU[0x16] = bytearray(file.read(0x100 * 2))
    ECU[0x23] = bytearray(file.read(0x100 * 2))
    ECU[0x3E] = bytearray(file.read(0x100 * 2))
    file.close()

#cmdS = b'\x5a\xa5\x02\x3e\x16\x03\x71\x00\x00\x35\xff'
#cmdZ = bytearray(cmdS)


if doWrite:
    sum = 0
    stl=len(cmdZ)
    cmdZ[2]=(stl-9)
    for i in range(2, (stl-2)):
        sum += cmdZ[i]
        resum = 0xFFFF - (sum & 0xFFFF)
    cmdZ[stl-2] = resum & 0xFF
    cmdZ[stl-1] = resum >> 8

    print("going with " + cmdZ.hex())
    ser.write(cmdZ)
    sleep(1)
    cmd1=cmdZ


bleLog=open("blelog.bin", "wb")
allLog=open("alllog.bin", "wb")
with open("eculog.bin", "wb") as outLog:
    while fuzAdr < 0x120:
        iDat = ser.read(4096)

        if mitm:
            ser2.write(iDat)
            bDat = ser2.read(4096)
            ser.write(bDat)
            stat = ""
            if bDat:
                for i in range(0, len(bDat)):
                    stat = stat + hex(bDat[i]) + " "
                if verbose: print("BLE " + stat)    # this is all just a lame shortcut vs. proper RX and routing
                bleLog.write(bDat)
                allLog.write(bDat)

        if iDat:
            stall=0
#            if debug: print("RX"+str(toggle)+": "+ bytes(iDat).hex())
            stat = ""
            for i in range(0, len(iDat)):
                stat = stat + hex(iDat[i]) + "-"
#            print("RXD " + stat)

        outLog.write(iDat)
        allLog.write(iDat)

        # validate packet header
        while len(iDat) > 6:
            change=0
            if (iDat[0] == 0x5A) & (iDat[1] == 0xA5):
                rLen = iDat[2]  # data len, should be checked to see if RX is complete
                rSrc = iDat[3]  # source MCU
                rRsp = iDat[4]  # target MCU
                rCmd = iDat[5]  # CMD
                rAdr = iDat[6]  # 8bit page selection

                # once we've seen 0x7b02 from the VCU, assume its ready
                if (rSrc == 0x16) & (rRsp == 0x04) & ((rCmd == 0x7B)):
                    stall = 0

                # write, write-NR
                if (rCmd == 0x02) | (rCmd == 0x03):
                    stat=""
                    tog=0
                    if (rSrc<0x10):
                        stat=stat+"0"
                    stat=stat+hex(rSrc)[2:] + ">"
                    if (rRsp<0x10):
                        stat=stat+"0"
                    stat=stat+hex(rRsp)[2:] + " "
                    if (rAdr<0x10):
                        stat=stat+"0"
                    stat=stat+hex(rAdr)[2:] + "= "

                    # store bytes
                    for i in range(0, rLen):
                        if ((rAdr * 2) + i) < 512:
                            if (ECU[rRsp][(rAdr * 2) + i] != iDat[i + 7]):
                                ECU[rRsp][(rAdr * 2) + i] = iDat[i + 7]
                                change=1
                        if (iDat[i+7]<0x10):
                            stat = stat + "0"
                        stat = stat + hex(iDat[i + 7])[2:]
                        if (tog):
                            stat=stat+"  "
                        else:
                            stat=stat+" "
                        tog=1-tog


                    # behavior? If odd# bytes, do we assume to add MSB=0?
                    if rLen & 1:
                        if ((rAdr * 2) + rLen) < 512:
                            ECU[rRsp][(rAdr * 2) + rLen] = 0

                    # send write confirmation
                    if rCmd == 0xf2:
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

#                    if ((((rAdr !=0xfe) & (rAdr != 0xfc)) | verbose) & sniff): print(stat)
#                    elif (change): print(stat)

                # read-response
                elif rCmd == 0x04:
                    transbat=1
                    stat="stok "
                    for i in range(0, rLen+9):
                        if (iDat[i]<0x10): stat=stat+"0"
                        stat=stat+hex(iDat[i])[2:] + " "
                    print(stat)

                    if ((rLen+9)>len(iDat)):    rLen=len(iDat)-9    # partial capture workaround

                    if (iDat[2]!=2):
                        transbat=0

                    if (transbat):
                        rP = bytearray(bytes(rLen+9))
                        for i in range(0, rLen+9):
                            rP[i]=iDat[i]

                        rP[3]=iDat[4]
                        rP[4]=iDat[3]
                        rP[5]=2

                        sum=0
                        for i in range(2, rLen+7):
                            sum += rP[i]
                        resum = 0xFFFF - (sum & 0xFFFF)
                        rP[rLen+7] = resum & 0xFF
                        rP[rLen+8] = resum >> 8

                        stat="stat "
                        for i in range(0, rLen+9):
                            if (rP[i]<0x10): stat=stat+"0"
                            stat=stat+hex(rP[i])[2:] + " "
                        print(stat)
                        ser.write(rP)

#                    if ((((rAdr !=0xfe) & (rAdr != 0xfc)) | verbose) & sniff): print(stat)
#                    elif (change): print(stat)


                    # behavior? If odd# bytes, do we assume to add MSB=0?  should check actual behaviour
                    if rLen & 1:
                        if ((rAdr * 2) + rLen) < 512:
                            ECU[rSrc][(rAdr * 2) + rLen] = 0

                    if ((((rAdr !=0xfe) & (rAdr != 0xfc)) | verbose) & sniff): print(stat)  # not sure this gets hit
                    elif (change): print(stat)


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
#//nope                        if transmit: ser.write(oDat[i : i + width])
#                        if transmit: ser2.write(oDat[i : i + width])
                    # k                        activity = idleTime

                    if debug: print(stat)
                else:
                    if debug:
                      stat=""
                      if (rSrc<0x10): stat=stat+"0"
                      stat=stat+hex(rSrc)[2:]+":"
                      if (rRsp<0x10): stat=stat+"0"
                      stat=stat+hex(rRsp)[2:]+" cmd "
                      for i in range(5, (rLen+7)):
                        if (iDat[i]<0x10): stat=stat+"0"
                        stat=stat+hex(iDat[i])[2:] + " "
#                      print("??: " + hex(rCmd) + " " + hex(rAdr) + " (" + hex(rLen) + ") " +stat )
#                      stat=stat+hex(rCmd)[2:]+" "+ hex(rAdr) + " (" + hex(rLen) + ") "
                      print(stat)


            iDat = iDat[1:]  # make this less dumb

        if activity:
            activity -= 1
        else:
            flipflop = 1 - flipflop
            activity = idleTime
            if heartbeat:
                if toggle:
                    ser.write(cmd0)
                else:
                    ser.write(cmd1)
                toggle=1-toggle

        if fuzTim:
            if stall == 0:
                fuzTim -= 1
        else:
#            if debug: print("fuznow")
            fuzTim = fuzDly
#            stall = 1

        if (1):
            rP = bytearray(bytes(11))
            rP[0] = 0x5A
            rP[1] = 0xA5
            rP[2] = 0x02
            rP[3] = 0x04
            rP[3] = 0x3e
            rP[4] = 0x07    # SETUP
            rP[4] = 0x16    # SETUP
            rP[4] = destECU
            rP[5] = 0x01
            rP[6] = (fuzAdr&0xff)
            rP[7] = 0x2
            rP[8] = 0x0  # bytes to read

            sum = 0
            for i in range(2, 9):
                sum += rP[i]
            resum = 0xFFFF - (sum & 0xFFFF)
            rP[9] = resum & 0xFF
            rP[10] = resum >> 8

            stat = ""
            for i in range(0, len(rP)):
                if rP[i]<0x10:
                    stat=stat+"0"
                stat = stat + hex(rP[i]) + " "

            if transmit:
                ser.write(rP)
                fuzAdr+=0x01
                if (fuzAdr>=0x100):
                    fuzAdr-=0x100
                sleep(.25)
        flipper=1-flipper


handler(0, 0)   # save the ECU
ser.close()     # close port
exit()
