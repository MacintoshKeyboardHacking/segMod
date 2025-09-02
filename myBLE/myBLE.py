#!/usr/bin/python3
# https://www.youtube.com/@MacintoshKeyboardHacking/streams

import serial
import signal

import Adafruit_SSD1306

from PIL import Image
from PIL import ImageDraw
from PIL import ImageFont

import signal
import subprocess


disp = Adafruit_SSD1306.SSD1306_128_32(rst=None)

disp.begin()
disp.clear()
disp.display()

width = disp.width      #128
height = disp.height    #32
image = Image.new('1', (width, height))

draw = ImageDraw.Draw(image)
#draw.rectangle((0,0,width,height), outline=0, fill=1)

count=0



ECU = {}
ECU[0x04] = bytes(bytearray(0x100 * 2))
ECU[0x06] = bytes(bytearray(0x100 * 2))
ECU[0x07] = bytes(bytearray(0x100 * 2))
ECU[0x16] = bytes(bytearray(0x100 * 2))
ECU[0x23] = bytes(bytearray(0x100 * 2))
ECU[0x3E] = bytes(bytearray(0x100 * 2))


def handler(signum, frame):
    with open("ecu2.bin", "wb") as outL:
        outL.write(ECU[0x04])
        outL.write(ECU[0x07])
        outL.write(ECU[0x16])
        outL.write(ECU[0x23])
        outL.write(ECU[0x3E])
    disp.clear()
    disp.display()
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


ser2 = serial.Serial("/dev/ttyUSB2")
ser2.baudrate = 115_200
ser2.bytesize = serial.EIGHTBITS  # number of bits per bytes
ser2.parity = serial.PARITY_NONE  # set parity check: no parity
ser2.stopbits = serial.STOPBITS_ONE  # number of stop bits
ser2.timeout = (1 / 100)  # non-block read
ser2.xonxoff = False  # disable software flow control
ser2.rtscts = False  # disable hardware (RTS/CTS) flow control
ser2.dsrdtr = False  # disable hardware (DSR/DTR) flow control
ser2.writeTimeout = 1  # timeout for write

#ser2 = ser

with open("ecu2.bin", mode="rb") as file:
    ECU[0x04] = bytearray(file.read(0x100 * 2))
    ECU[0x07] = bytearray(file.read(0x100 * 2))
    ECU[0x16] = bytearray(file.read(0x100 * 2))
    ECU[0x23] = bytearray(file.read(0x100 * 2))
    ECU[0x3E] = bytearray(file.read(0x100 * 2))
    file.close()

cmd0 = b"\x5A\xA5\x00\x04\x16\x7A\x00\x6B\xFF"  # 0416:7a00
cmd1 = b"\x5A\xA5\x00\x04\x16\x7B\x02\x68\xFF"  # 0416:7b02
# cmd0=cmd1

# keepalive ECU 23 > 16
cmdZ = b"\x5a\xa5\x00\x23\x16\x01\xfc\xc9\xfe"
cmd2 = b''
# cmdZ=b'\x5a\xa5\x00\x23\x16\x01\xfa\xcb\xfe'


idleTime = 10
activity = 0
toggle = 0
flipflop = 0

fooMe=0
block=0

stall = 1

fuzAdr = 0x10
fuzAdr = 0x0
fuzDly = 10
fuzTim = fuzDly

debug=1
transmit=0


foo=bytearray(b'\x5a\xa5\xFF\x23\x16\x02\xfd\x01\x0a\x89\x52\x76\x7e\x10\x89\x7a\xaa\xdd\x10\xc5\x0a\x44\xac\x10\xb3\xd1\xdd\xa3\x10\xb2\xd1\xdd\xa3\x10\xad\xd1\xdd\xa3\x10\xac\xd1\xdd\xa3\x10\xaf\xd1\xdd\xa3\x10\x44\x6c\xe5\x6f\x10\xae\x48\x4a\xad\x10\x9e\xf9\x96\x90\x10\xe4\x65\x9b\x9c\x10\x92\x00\x9a\x22\x10\x64\xd0\xf6\xa4\x08\x12\x45\x6a\x61\x08\x70\xe0\xe2\xb3\x08\x8a\x38\x99\xe7\x08\xda\xb2\x89\x86\x04\xa1\x24\x01\x7e\x04\x4b\xcb\xf9\xf8\x04\x75\x1d\xbc\xa2\x04\x9f\x6f\x41\x42\x01\x98\x8d\xb2\x17\x01\x29\x0a\xbc\xcf\x01\xcc\xba\xe7\x88\x01\xe5\xe9\x99\x0c\x01\x61\xe9\x41\x18\x01\x8a\xa8\xb5\x16\x01\xb3\xd7\xef\x70\x01\x03\x26\x1a\x75\x01\x39\xb0\xbf\x06\x01\x3b\x26\xe8\x20\x01\x55\x25\x1a\x7f\x01\x2b\x71\x39\xf7\x01\x6b\x64\x4e\xac\x01\x19\xbd\x86\x00\x01\x1a\x93\x7d\x0c\x01\xFF\xFF')


foo = bytearray(
                b"\x5a\xa5\xFF\x23\x16\x01\xfd\x02\x0a"+\
                # MPH
                b"\x44\x6c\xe5\x6f\x10"+\
                # drive mode 92=sport, 33=race
                b'\x89\x7a\xaa\xdd\x10'+\
                b"\xFF\xFF"
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





if debug: print("going with " + cmdZ.hex())
with open("eculog.bin", "wb") as outLog:
    while fuzAdr < 0x100:
        if toggle:
            iDat = ser.read(4096)
        else:
            iDat = ser2.read(4096)

        if iDat:
            if debug: print("RX"+str(toggle)+": "+ bytes(iDat).hex())

        outLog.write(iDat)
        toggle = 1 - toggle

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
                    if (block==0):   fooMe=5

                # write, write-NR
                if (rCmd == 0x02) | (rCmd == 0x03):
                    stat = (
                        hex(rSrc)[2:] + ":" + hex(rRsp)[2:] + " " + hex(rAdr)[2:] + "="
                    )

                    # store bytes
                    for i in range(0, rLen):
                        if ((rAdr * 2) + i) < 512:
                            if (ECU[rRsp][(rAdr * 2) + i] != iDat[i + 7]):
                                ECU[rRsp][(rAdr * 2) + i] = iDat[i + 7]
                                change=1
                        stat = stat + hex(iDat[i + 7])[2:] + "-"

                    # behavior? If odd# bytes, do we assume to add MSB=0?
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

                        if transmit: ser.write(rP)
                        if transmit: ser2.write(rP)
                    # k                        activity = idleTime
                    if change: print(stat)

                # read-response
                elif rCmd == 0x04:
                    stat = (
                        hex(rSrc)[2:] + ":" + hex(rRsp)[2:] + " " + hex(rAdr)[2:] + ":"
                    )
                    if ((rLen+9)>len(iDat)):    rLen=len(iDat)-9    # partial capture workaround
                    for i in range(0, rLen):
                        if ((rAdr * 2) + i) < 512:
                            if (ECU[rSrc][(rAdr * 2) + i] != iDat[i + 7]):
                                ECU[rSrc][(rAdr * 2) + i] = iDat[i + 7]
                                change=1
                        stat = stat + hex(iDat[i + 7])[2:] + "-"

                    # behavior? If odd# bytes, do we assume to add MSB=0?
                    if rLen & 1:
                        if ((rAdr * 2) + rLen) < 512:
                            ECU[rSrc][(rAdr * 2) + rLen] = 0
                    if change: print(stat)

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
                        if transmit: ser.write(oDat[i : i + width])
                        if transmit: ser2.write(oDat[i : i + width])
                    # k                        activity = idleTime

                    if debug: print(stat)
                else:
                    if debug: print("??: " + hex(rCmd) + " " + hex(rAdr) + " (" + hex(rLen) + ")")
            iDat = iDat[1:]  # make this less dumb

        if activity:
            activity -= 1
        else:
            if flipflop:
                if transmit:
                    ser.write(cmd1)
                    ser2.write(cmd1)
                    if stall == 0: ser.write(cmdZ)
                    if stall == 0: ser2.write(cmdZ)
            else:
                if transmit:
                    ser.write(cmd0)
                    ser2.write(cmd0)
                    if stall == 0: ser.write(cmdZ)
                    if stall == 0: ser2.write(cmdZ)
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
            rP[6] = fuzAdr
#            rP[6] = 0x10
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

            if debug: print("set " + stat)

        disEn=(ECU[0x23][(0x25 * 2)] != 0)
        transmit=disEn
        if (transmit==0):
            block=0
        if (fooMe):
            print("foo me")
            fooMe-=1
            block=1
            ser.write(foo)
            ser2.write(foo)
#        print("i decide "+hex(disEn)+" "+hex(ECU[0x23][(0x25 * 2)]))
        if disEn:
            draw.rectangle((0,0,width,height), outline=0, fill=1)


            defFont = ImageFont.load_default()
            spdFont = ImageFont.truetype("Arial_Black.ttf", 40)

            txtFont = ImageFont.truetype("arial.ttf", 24)

            xo=4
            yo=-14
            spdFont = ImageFont.truetype("Arial_Black.ttf", 40)
            spd=str(ECU[0x23][(0x1FD)])
            draw.text((xo, yo),       spd,  font=spdFont, fill=0)

    # worst clock evar
            dateCmd = "date +%H:%M:%S"
            dateTxt = subprocess.check_output(dateCmd, shell = True )

            xs=64
            ys=-2
            tinyFont = ImageFont.truetype("Courier_New_Bold.ttf", 11)
            tinyFont = ImageFont.truetype("arial.ttf", 12)
            tinyFont = ImageFont.truetype("Courier_New_Bold.ttf", 10)
            draw.text((xs, ys), str(dateTxt,'utf-8'), font=tinyFont, fill=0)

            disp.image(image)
            disp.display()
        else:
            disp.clear()
            disp.display()



handler(0, 0)   # save the ECU
ser.close()     # close port
exit()
