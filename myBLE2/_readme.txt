#update 251018

myBLE2, a ninebot protocol router and BLE replacement for Segway "3 series" electric kickscooters

this project provides a framework for scooter customization and control.  An ESP32 is connected between the UART serial interfaces of the bluetooth display (BLE) and the vehicle control units (VCU); Communications are relayed transparently between interfaces or patched as desired, read and write variable access are cached in RAM for later use.

this implementation focuses on physically replacing the scooter display, adding new functionality and increasing security.  The stock screen looks nice, but actually provides very limited information... a large digital speedometer and a smaller readout of battery charge and estimated distance-to-empty; The color changes depending on which "speed mode" is selected: race, sport, eco or walk.  The clock and trip-odometer are nice features.

my ESP32 replacement drives 2x 8LED ws2812 strips as battery and speed bar-graphs, with color-changing status pixels offering additional information.

thanks to this innovation, I claim the following *NEW* features
1. automatic, always-on headlight!  the DRL is not enough visibility (IMO) and the light-control knob is stupid and hard to use (also IMO), so this hack makes sure the scooter always starts up safely.
2. high-visibility RBS (regen brake system) status light!
3. 2WD walk-mode!  Not quite sure how this happened but previously it didn't work; there have been no firmware or app updates so I'm taking credit!

along the way I also managed to remotely turn on the scooter, turn off the DRL and also turn off the headlight while in walk mode; I'll share those when I can reliably reproduce them.

the entire VCU/BLE display assembly (including plastic) is available from Segway service parts for $65, if anything goes wrong or you just want a spare.  The replacement VCU aparantly requires activation with the app to register the scooter's serial number along with a mileage/usage data transfer from your account; more information is needed.

removing the stock BLE display module from the VCU is straightforward, remove 3 screws and unplug the 2x5pin 2mm header; We only need 3 pins: TX, RX and GND; 5V is also available here though I've chosen to use an external regulator.

the module I used is the Wemos LoLin32 because I own several; at the time it was the only full-sized ESP32 module with Bluetooth that I could find on Amazon that didn't have pin headers already soldered.  Much smaller modules are available however, and it should be fairly easy to fit one without removing the stock display.  An ESP8266 should be fine as well, with the exception of future myBLE ESP32 Bluetooth support.

coming soon:
native bluetooth replacement for Segway app usage (for now, the stock BLE module must be connected while using the app)
a more detailed display, maybe as an app on a USB connected dashcam/audio headunit
a working anti-theft alarm system

future ideas:
web-page configuration of scooter settings
local, flash-based logging and performance monitoring
STlink emulation (built-in and always connected ;)

---------------

the hardware:
there are two 12vdc lines in the control cable, presumably one for the VCU and another for the BLE/lights, although the BLE power supplied is regulated to 5vdc by the VCU.  I've connected both 12vdc lines via schottky diodes (1n5157?) to the DC-in of a 5v3amp converter module, to provide extra current during development and maximize reliability.  The BLE 12v connection could be used to power an LED driver board controlled by ESP32 PWM'd GPIOs.

the ESP32 primary UART is available as 3V3 TTL or via the onboard USB serial interface, which could be used to back-feed power into a Pi if you bypass a diode.  (if you do this, choose a Zero2W, it can fit nicely inside the stalk)
the secondary UART connects to the VCU via GPIO16-RX2/17-TX2 pins
two GPIOs are routed to VCU pins IO0 and IO1 as inputs; IO0 is the vehicle power button.  I have not tested these.

(not yet implemented)
a resistive voltage divider (18k+30k?) connecting the ESP32 to the 5v VCU Hall sensor/GND for left brake, right brake and throttle.  Planned for GPIO 36,39,?

the I2C port remains free for accelerometer, compass and ambient temperature sensors
the SPI port remains free for a CANbus or ethernet module, an LCD display, or a lot of WS2811 LED pixels
one UART remains available (tested working on GPIO25/26) for direct BLE pass-thru

---------------

for this POC, the BLE module will be connected externally via the Raspberry Pi UART when required but it could also be cabled directly to the ESP32.  a future software revision will move more of the configuration from source-code to a "dedicated virtual ECU"

---------------

development functionality

it helps to understand the basic ninebot serial protocol first, basically a per-module collection of 16-bit read/write variables for status and control.

starting with a blank (all 0) map, non-corrupt packets are forwarded between interfaces verbatim.
as variables are read and written, their values are stored in the ECUmap along with status flags.

To manipulate a variable, add an entry to the map by changing the command from 0x00 to (for example) 0x10 (linear add) and set a value as modifier (for example) 0x0a (ten).  Storing this entry in the map at (for example) ECU:0x16 Addr:0x5A (speed limit in ECO mode) will have the following effect:
Setting the in-app ECO speed slider to 15 will store a value of 25 (15+10) in the vehicle's speed register.  if the app later reads this register, this offset can be automatically removed, returning the expected value.  The effect is that the control operates normally but with an offset range.

Basic commands are included for adjusting both 8 and 16 bit variables.  Read/write functions do not need to be symmetrical and can include remapping functions to access variables from different ECU and memory addresses.  Commands can be designed to block transmission of their trigger packet and instead return arbitrary command strings, ala macros.  With 255 commands * 256 modifiers virtually any operation can be accomodated.

---------------

What's next?  the tool is ready, let's explore!  it will take some time to identify register functions and debug applications, but my hope is we'll see some updates soon.  I'm sure we'll unlock some performance on Segway's high-end models, but there might be a lot more to unlock on the lower-tiers!

I would like this backend code support all Segway Ninebot scooters, please open a github issue if you'd like help getting your model supported.

If you just want a "modchip" for general performance or vehicle usage tweaks, a much simpler installation is possible using the existing 5vdc and without removing the display.  You don't need a Raspberry Pi, or any components, just a basic ESP32 module and a way to connect to 2mm pitch headers (slightly smaller than traditional DuPont headers; I had success "wire wrapping" some 30gauge Kynar wire to the pins on the BLE module, but in this revision I soldered wires to individual right-angle male-male header pins, folded in Kapton tape, and secured with a couple cat5 conductor "twist-ties"; optionally a tiny hot-glue could be added if more rigidity is required.)

---------------

How can YOU help?
Q: What would it take to drive the stock LCD with an ESP32, anybody up to the challenge?
A: I'm not sure it's worth it... for my use case I'm thinking a USB connected dashboard app on a cheap motorcycle stereo.  In the mean-time, I've got this RGB LED ;)

Q: Make use of the ESP32 wifi functions, put a user-interface on the ECUmap configuration, load/save presets, OTA firmware updates?
A: YES PLEASE!  Again, not my cup of tea; at this stage I plan to continue development on the Raspberry Pi via SSH as much as possible.

Q: your code is stoopid!
A: YES, please tell me how I can improve it!  It would be great to see more use of functions, better flow, cleaner memory layout, or whatever...  any tweaks?

Q: this is dumb, I obviously want to keep the stock display!
A: YES, let's turn this into an easy-to-install modchip then!  To avoid breaking pins, cutting traces, etc., while retaining use of the existing display!  Perhaps connect through the otherwise unused "1wire" pin and avoid the stock BLE connection entirely?  You couldn't do all the variable-modification-trickery but maybe that's overkill?

---------------

	3V	GND
	EN	GPIO1	TX
	GPIO36	GPIO3	RX
	GPIO39	3V
RED	GPIO32	GPIO22		(rts)
BLUE	GPIO33	GPIO21
IN0(PB)	GPIO34	GND
IN1(??)	GPIO35	GND
TX3	GPIO25	GPIO19		(cts)
RX3	GPIO26	GPIO23g		ws2811
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
g gpio23 is the fastLED default, but was disabled on the wemos lolin32 board; I had to patch something, I forget where...

GPIO 6 to GPIO 11 (connected to the ESP32 integrated SPI flash memory – not recommended to use).
* ADC2 pins cannot be used when Wi-Fi is used.

