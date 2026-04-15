presenting myBLEv4: a modchip and development framework for Segway/Ninebot 'x3 series' scooters.

yet another complete re-write, this version uses a UART modification of the BLE display board; vehicle appearance and operation remain the same.

now wifi-enabled with online updates, simple functions() for packet transmission and made easier by a virtual ECU framework; the project uses a $5 Amazon ESP32S3 module connected inside the display.

This version joins my home WiFi and offers a web interface to Segway's 9bot protocol, displaying information on the 256+ 16-bit variables tracked by each ECU module.

still missing a lot of features but the "hard stuff" is working and stable.

various URLs are available:
/on, /off	- control power
/scan		- read ECUs to map
/refresh	- clear map 'new' flags
/clear		- clear entire map
/log		- console output page
/admin		- firmware updates


still to come: automatic parsing of
	voltages and temperatures from the cells
	realtime energy consumption
	motor controller temps
	speed, distance, drive mode, ambient light level...

other ideas...
abnormal movement detected?  add a LoRa transmitter to page, transmit coordinates, or a remote panic button to trigger the alarm
use a hidden NFC sensor to unlock or de-restrict the scooter (put a tag in your shoe!)

subtle tweaks...
turn off regen braking when in eco-mode for downhill coasting efficiency
keep 'walk mode' disabled from speed selection (workaround for mobile-app bug)
override ambient light sensor reading, to force auto-headlights on

ECU acls
a 256 byte map of allowed command opcodes, with each value being an ECU bitmask.  This allows to quickly isolate new commands and interrupt firmware updates; currently disabled

subMap
parse VCU subscription packet, generate 16bit registers for all parameters
- allow portability between different sw/hw by specify ie: SUB_CHARGING
- easy monitor and update of status fields


-------
install
-------

F3/G3/GT3 scooters share the 'D1X_Core_Mainboard MVT2' which connects the 1.8v NB-BT100 bluetooth SoC module with the 3.3v VCU via a 10pin, 2mm header.  There are 5 level converters handling the TX1, RX1, 1WIRE, IO_0, and IO_1 pins, each has either a fuse or 0 ohm resistor in series on the 3.3v side.

Removing fuses F302 and F301 takes some patience; first to scrape away the clear epoxy and then time to apply enough heat to cleanly desolder them as they melt and disintegrate.  As an alternative, it may be possible to apply an external DC current to blow the fuses without removing them.

https://github.com/MacintoshKeyboardHacking/segMod/blob/main/images/myBLEv4-one.jpg

F302: Left to ESP32 BLE TX (white), Right to ESP32 VCU RX1 pin (blue)
F301: Left to ESP32 BLE RX (orange), Right to ESP32 VCU TX1 pin (green)

with myBLEv2 I used an external 5v regulator tapping the 12v lines to provide more current for LEDs and more stability (since the stock 5v line is switched)
with myBLEv4 I've just tapped the stock 5v and it's been fine, so long as the code boots fast enough (because F3/G3 will power-reset the board if it doesn't respond in time)


------------------------
compile and use hardware
------------------------

I've used the Arduino IDE because it's easy; as long as you can enable external PSRAM for your board it should work fine once you get your UART GPIOs sorted out (see arduinoSettings.png)

myBLEv2 ran fine on the older dual-core ESP32 architecture, and v3 added basic wifi support as well.  
myBLEv4 uses the newer ESP32S3 series for additional RAM; the project is designed around a 4MB flash/2MB PSRAM module but lower capacities could work as well.
the ESP32S2 is similar, but single-core; it would probably work fine since the code is designed to minimize time spent in loop().
the ESP32C? series only has 2 serial ports; that defeats the multi-port router aspect of this project and is therefore not considered.


-------
updates
-------

once you've gotten the firmware flashed and connected to wifi, future updates can be made through the /admin interface, just "export compiled binary" from Arduino.

If you flash a bad firmware, don't panic... first, try the Segway reboot/powercycle (while the scooter is on and unlocked, hold power for 20 seconds or so until it beeps the *second* time); that way the module will boot with a clear memory map.
Failing that, you can reset or powercycle THEN hold the GPIO boot button and within a second the LED flashes white (only press *after releasing reset*, otherwise you'll be sitting in the bootloader and waiting for a USB upload flash).  This is safe-mode... the board is essentially "pass through" with the web interface active.  Note, you may need to powercycle again to get out of this mode after flashing a fixed firmware.

----------------------
vehicle specific notes
----------------------

power-reboot behavior
GT3: holding power > 15 seconds, the BLE power turns off until the button is released
G3/F3: holding power > 15 seconds, the BLE power is interrupted then restored

GT3: if problems with BLE module connection (detected by the absense of 7a/7b heartbeat packets) the scooter starts beeping.  After reconnection, everything is normal.
G3/F3: the VCU expects a response from the BLE pretty quickly at startup.  If this is delayed, the VCU will first try power-cycling the BLE, then operate as if display doesn't exist.  This can be a problem if your code doesn't start sending packets soon enough

GT3: SUB field, trip variable is miles
G3/F3: SUB field, trip variable is miles*10

--------------------

any comments, questions, ideas, concerns, open an issue
if you like this, give the repo a GITHUB STAR PLEASE! :) 

install and demo video coming eventually,
https://www.youtube.com/@MacintoshKeyboardHacking/streams

enjoy!
