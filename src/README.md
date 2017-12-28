# EmontxV2_170220
By Patrik Hermansson

Latest version: https://github.com/bphermansson/emontxv2

Sensor for Emoncms network. This device is mounted outdoor at the electricity meter pole.

Sensors used:

-BMP180, air pressure/temperature, I2C.
   Code uses Sparkfuns lib for BMP180,
   https://learn.sparkfun.com/tutorials/bmp180-barometric-pressure-sensor-hookup-?_ga=1.148112447.906958391.1421739042

-HTU21D, humidity/temperature, I2C. Mounted at a good(external) location.
   https://learn.sparkfun.com/tutorials/htu21d-humidity-sensor-hookup-guide/htu21d-overview

- PT333-3C Phototransistor, light sensor. C to Vcc, E to A3. A3 via 470k to ground.

-UVI-01 UV sensor. http://www.dreamgreenhouse.com/datasheets/UVI-01-E/index.php

-Voltage divider on A0 reading the solar cell voltage.

-Battery voltage monitored internally.

   http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/)


Uses a Atmega328 bootloaded as a Arduino Mini Pro. Uses a 8 MHz crystal to
be able to run down to 2.4 volts.
Power supply is a LiFePo4 charged by a solar cell, parts taken from a garden solar
cell lamp.
Debug messages can be enabled via serial port, 115200 baud.


Based on "emonTX LowPower Temperature Example"
Part of the openenergymonitor.org project
Licence: GNU GPL V3

Authors: Glyn Hudson, Trystan Lea
Builds upon JeeLabs RF12 library and Arduino

THIS SKETCH REQUIRES:

Libraries in the standard arduino libraries folder:
- JeeLib		https://github.com/jcw/jeelib


D2 - RFM69CW IRQ
D10          SEL
D11          SDI
D12          SDO
D13          SCK

BMP180 - I2C on A4 and A5, power + on D4
D3 - ?
D5 - NC
D6 - Red Led
D7 - NC
D8 - NC
D9 - NC

Current sensor (removed) was on A1


 Reminder for Github:
 git commit EmontxV2.ino
 git push origin master
