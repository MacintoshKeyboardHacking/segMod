#!/usr/bin/python3
#
# this works for me to sniff the UART while bridging VCU to an external BLE
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
mitm=1      # pass-thru external BLE
verbose=1   # don't filter realtime update packets, include verbose BLE output
debug=1     # show extended protocols
sniff=1     # display "everything" versus "only display changed data"


transmit=0  # active scanning
destECU=0x07    # target ECU to scan
destECU=0x16    # target ECU to scan
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
ECU[0x20] = bytes(bytearray(0x100 * 2))
ECU[0x22] = bytes(bytearray(0x100 * 2))


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

        if flipper:
            rP = bytearray(bytes(12))
            rP[0] = 0x5A
            rP[1] = 0xA5
            rP[2] = 0x02
            rP[3] = 0x3e
            rP[4] = 0x16    # SETUP
            rP[5] = 0x01
            rP[6] = (fuzAdr&0xff)
            rP[7] = 0x02
            rP[9] = 0x0  # bytes to read

            sum = 0
            for i in range(2, 10):
                sum += rP[i]
            resum = 0xFFFF - (sum & 0xFFFF)
            rP[10] = resum & 0xFF
            rP[11] = resum >> 8

            tR=rP


        toggle=1-toggle
        if (toggle):
            iDat = ser.read(4096)
        else:
            iDat = ser2.read(4096)

        if iDat:
            stat = ""
            for i in range(0, len(iDat)):
                stat = stat + hex(iDat[i]) + "-"

            outLog.write(iDat)
            allLog.write(iDat)

        # validate packet header
            while len(iDat) > 8:
                change=0
                if (iDat[0] == 0x5A) & (iDat[1] == 0xA5):
                    rLen = iDat[2]  # data len, should be checked to see if RX is complete
                    rSrc = iDat[3]  # source MCU
                    rRsp = iDat[4]  # target MCU
                    rCmd = iDat[5]  # CMD
                    rAdr = iDat[6]  # 8bit page selection

                    if (len(iDat)>=(rLen+9)):
                      rP = bytearray(bytes(rLen+9))

                      stat = ""
                      for i in range(0, rLen+7):
                        if (i<len(rP)):
                            rP[i]=iDat[i]
                        if iDat[i]<0x10:
                            stat=stat+"0"
                        stat = stat + hex(iDat[i])[2:] + " "
                      print("a"+stat)

                      fSrc=rSrc
                      fRsp=rRsp
                      fAdr=rAdr

                      # this is where ECU remap starts
                      if (0):
                        if (rRsp==0x20):
                            fRsp=0x16
                        if (rRsp==0x22):
                            fRsp=0x7
                            if (rAdr==0x10):
                                fAdr=0xa0
                        if (rSrc==0x16):
                            fSrc=0x20
                        if (rSrc==0x07):
                            fSrc=0x22
                            if (rAdr==0xa0):
                                fAdr=0x10

                      rP[3] = fSrc
                      rP[4] = fRsp
                      rP[6] = fAdr

                      sum = 0
                      for i in range(2, rLen+7):
                        sum += rP[i]
                      resum = 0xFFFF - (sum & 0xFFFF)
                      rP[rLen+7] = resum & 0xFF
                      rP[rLen+8] = resum >> 8

                      stat = ""
                      for i in range(0, len(rP)):
                        if rP[i]<0x10:
                            stat=stat+"0"
                        stat = stat + hex(rP[i])[2:] + " "

                      print("b"+stat)
                      if (toggle):
                        ser2.write(rP)
                      else:
                        ser.write(rP)

                iDat = iDat[1:]  # make this less dumb


handler(0, 0)   # save the ECU
ser.close()     # close port
exit()
