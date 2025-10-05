myBLE2, a ninebot protocol router and BLE replacement for Segway "3 series"

this project provides a framework for scooter customization and control.  An ESP32 is connected between the UART serial interfaces of the bluetooth display and the vehicle control units; Communications are relayed transparently between interfaces except as configured by an ECU "map" file.

packets are defragmented and validated, then patched as desired and placed in output queues, currently designed to support 4-5 simultaneous interfaces

If a BLE or serial controller is connected and active, the ESP remains transparent; otherwise it performs the ECU heartbeat functions necessary for the vehicle to start and operate normally.  


example functionality:
allow changing vehicle settings beyond in-app restriction
	ie: set speeds +10mph over app setting
set drive speeds and power limits beyond simple mode defaults
create macros:
	ie: always startup in 2WD sport mode with headlights on
	ie: race mode always boost
selectively block updates
re-enable old apps (remap ECU parameters for compatibility)
extended parameter monitoring
	ie: idle bandwidth is used to continuously read large ranges of variables, optionally delivering updates when changes are detected

extreme functionality:
add a second battery and fully integrate the BMS
add a second ESC for 2WD


additional functionality:
analog input front and rear brakes, throttle, ambient light?, battery voltage?
PWM LED control with dynamic effects and ultra-obnoxious strobe modes
2-port GPIO (currently power button and reserved)
I2C bus for IMU

coming soon:
native bluetooth replacement for Segway app usage (for now, the stock BLE module must be connected during app use)
some kind of display, maybe as an app on a USB connected dashcam/audio headunit
a working anti-theft alarm system

future ideas:
STlink emulation (built-in and always connected ;)
local, flash-based logging

----------------------

the hardware:
there are two 12vdc lines in the control cable, presumably one each for the BLE and VCU, although the BLE power supplied is regulated to 5vdc by the VCU.  I've connected both 12vdc lines via schottky diodes (1n5157?) to the DC-in of a 5v3amp converter module, intending to provide enough extra current to power a small Raspberry Pi system for development purposes.  The BLE 12v supply is additionally used to power an LED driver board controlled by ESP32 PWM'd GPIOs.

the ESP32 primary UART remains dedicated to the serial USB port, and could be used to back-feed power into a Pi if you bypass a diode.  if you do this, choose a Zero2W, it can fit nicely inside the stalk)
the secondary UART connects to the VCU via GPIO16-RX2/17-TX2 pins
two GPIOs are routed to VCU pins IO0 and IO1 as inputs; IO0 is the vehicle power button

(not yet implemented)
a resistive voltage divider (4.7k+4.7k?) connecting the ESP32 to the 5v VCU Hall sensor/GND for left brake, right brake and throttle.

the I2C port remains free for accelerometer, compass and ambient temperature sensors
the SPI port remains free for a CANbus or ethernet module, an LCD display, or a lot of WS2811 LED pixels
one UART remains available (tested working on GPIO25/26) for direct BLE pass-thru

---------------

for this POC, the BLE module will be connected externally via the Raspberry Pi UART when required but it could also be cabled directly to the ESP32.  a future software revision will move more of the configuration from source-code to a "dedicated virtual ECU"


----------------

functionality

it helps to understand the basic ninebot serial protocol first, basically a per-module collection of 16-bit read/write variables for status and control.

starting with a blank (all 0) map, non-corrupt packets are forwarded between interfaces verbatim.
as variables are read and written, their values are stored in the ECUmap along with status flags.

To manipulate a variable, add an entry to the map by changing the command from 0x00 to (for example) 0x10 (linear add) and set a value as modifier (for example) 0x0a (ten).  Storing this entry in the map at (for example) ECU:0x16 Addr:0x5A (speed limit in ECO mode) will have the following effect:
Setting the in-app ECO speed slider to 15 will store a value of 25 (15+10) in the vehicle's speed register.  if the app later reads this register, this offset can be automatically removed, returning the expected value.  The effect is that the control operates normally but with an offset range.

Basic commands are included for adjusting both 8 and 16 bit variables.  Read/write functions do not need to be symmetrical and can include remapping functions to access variables from different ECU and memory addresses.  Commands can be designed to block transmission of their trigger packet and instead return arbitrary command strings, ala macros.  With 255 commands * 256 modifiers virtually any operation can be accomodated.

-------------------

What's next?  the tool is ready, let's explore!  it will take some time to identify register functions and debug applications, but my hope is we'll see some updates soon.  I'm sure we'll unlock some performance on Segway's high-end models, but there might be a lot more to unlock on the lower-tiers!

I would like this backend code support all Segway Ninebot scooters, please open a github issue if you'd like help getting your model supported.

If you just want a "modchip" for general performance or vehicle usage tweaks, a much simpler installation is possible using the existing 5vdc and without removing the display.  You don't need a Raspberry Pi, or any components, just a basic ESP32 module and a way to connect to 2mm pitch headers (slightly smaller than traditional DuPont headers; I had success "wire wrapping" some 30gauge Kynar wire to the pins on the BLE module, but in this revision I soldered wires to individual right-angle male-male header pins, folded in Kapton tape, and secured with a couple cat5 conductor "twist-ties"; optionally a tiny hot-glue could be added if more rigidity is required.)




How can YOU help?
Q: What would it take to drive the stock LCD with an ESP32, anybody up to the challenge?
A: I'm not sure it's worth it... for my use case I'm thinking a USB connected dashboard app on a cheap motorcycle stereo.  In the mean-time, I've got this RGB LED ;)

Q: Make use of the ESP32 wifi functions, put a user-interface on the ECUmap configuration, load/save presets, OTA firmware updates?
A: YES PLEASE!  Again, not my cup of tea; at this stage I plan to continue development on the Raspberry Pi via SSH as much as possible.

Q: your code is stoopid!
A: YES, please tell me how I can improve it!  It would be great to see more use of functions, better flow, cleaner memory layout, or whatever...  any tweaks?

Q: this is dumb, I obviously want to keep the stock display!
A: YES, let's turn this into an easy-to-install modchip then!  To avoid breaking pins, cutting traces, etc., while retaining use of the existing display!  Perhaps connect through the otherwise unused "1wire" pin and avoid the stock BLE connection entirely?  You won't get all the variable-modification-trickery but maybe that's overkill?




-----------------


	3V	GND
	EN	GPIO1	TX
	GPIO36	GPIO3	RX
	GPIO39	3V
RED	GPIO32	GPIO22		(rts)
BLUE	GPIO33	GPIO21
IN0(PB)	GPIO34	GND
IN1(??)	GPIO35	GND
TX3	GPIO25	GPIO19		(cts)
RX3	GPIO26	GPIO23
GREEN	GPIO27	GPIO18
	GPIO14a	GPIO5d
	GPIO12c	3V
	GPIO13	GPIO17	TX2
	5V	GPIO16	RX2
	GND	GPIO4
		GPIO0e
		GND
		GPIO2f
		GPIO15b

a outputs PWM signal at boot
b outputs PWM signal at boot, strapping pin
c boot fails if pulled high, strapping pin
d outputs PWM signal at boot, strapping pin
e outputs PWM signal at boot, must be LOW to enter flashing mode
f sometimes connected to on-board LED, must be left floating or LOW to enter flashing mode
GPIO 6 to GPIO 11 (connected to the ESP32 integrated SPI flash memory – not recommended to use).
* ADC2 pins cannot be used when Wi-Fi is used.

