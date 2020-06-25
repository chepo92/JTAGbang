# JTAGbang
JTAGbang Arduino

This is a mirror for JTAGbang code from Karl Hans Janke's blog http://www.khjk.org/log/2013/aug/jtagbang.html

## Description (by the author)
This is a very simple Arduino program that allows direct interaction with the JTAG interface from a serial terminal. It is called jtagbang because it is essentially bit-banging on the JTAG pins.

## Board Compatibility

Tested with:

- Arduino UNO

## Wiring



## Usage (by the author)
So, long story short: Upload the attached sketch to an Arduino, take a peek at the top of the file maybe, and connect to it with a terminal emulator (read minicom) or the Arduino IDE's serial monitor (set to line-ending "Newline"). Enter a capital X and it will interrogate the JTAG interface to find all the connected devices (chips). It lists their built-in identification codes which take the form of 32 bits in four groups:

59604093  [0101 1001011000000100 00001001001 1]

The groups are, from most to least significant bit: 4-bit product version (5), 16-bit product code (9604 is the XC9572XL), 11-bit manufacturer code (00001001001 is Xilinx), and one bit that is always 1 for thaumaturgic reasons. 




## License
Licensed by Karl Hans Janke under ISC
https://opensource.org/licenses/ISC
