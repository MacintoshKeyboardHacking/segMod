253108: another MacintoshKeyboardHacking creation... 

myBLE.py is a Raspberry Pi based replacement for the Segway BLE display module present in GT3 series scooters; It should be easy to adapt to other models in the "3" series (F3 Pro, G3 Max, ZT3) and possible to support older models as well.

The code implements the functionality necessary to get the Segway VCU into "drive" mode and stream realtime vehicle performance parameters - speed, battery, distance, etc.  As a PoC, vehicle speed is displayed on a basic 128x32 I2C monochrome OLED.

I've used a Raspberry Pi 3A, but a Pi Zero would be a better choice as it would fit inside the stock BLE location.  A Pi Zero2 (1GHz quadcore CPU) would be equivalent to the Pi3 in performance, but even a 1st generation Pi Zero (1GHz singlecore) should be fine given how slow the UART communication involved actually is.

It should be possible to provide Segway-app compatible Bluetooth using the a Pi's built-in interface, but I'm more interested to see the code ported to ESP32 and use that bluetooth instead.

Why?  The GT3 UI is pretty basic: mode, speed and battery... underwhelming for a $2700 scooter.  More advanced features like auto-unlock via Bluetooth proximity and cell-phone callerID display are interesting but of questionable utility.  Pay navigation service?  Hard pass.  and Privacy... Segway is constantly broadcasting BLE, trackable by the FindMy network even when not enrolled or enabled.

What?  duplication of existing UI functionality, support for the standard Segway app and stock firmware update mechanisms.  Once that's working, it's time for the upgrades!

How?  The GT3 construction makes replacement of the display module a "remove 3 screws, unplug old display, plug-in new module" operation while preserving the factory waterproofing!

When?  ASAP!  I'm tired of having wires and PCBs taped all over my scooter... makes it hard to ride!

A full Android-Auto/CarPlay head unit?  Sure, let's add on-vehicle audio and rock out!  It should be "fairly trivial" for someone with Android coding experience to put together a nice gauge display app and integrate it into one of those $75 motorcycle dvr/media player units.

How about *real* GPS tracking with a built-in LTE cellular uplink?  Let's go!  Because my assessment is that the existing security mechanisms are trivial to bypass; hopefully a future firmware update improves them.

Access to detailed perforance metrics and custom tuning?  No doubt.  There are certainly ECU registers that control available power, just need to find them.

Where?  documented in the github project wiki https://github.com/MacintoshKeyboardHacking/segMod/wiki
and video live-streams at https://youtube.com/@MacintoshKeyboardHacking/streams

Enjoy! ;)
