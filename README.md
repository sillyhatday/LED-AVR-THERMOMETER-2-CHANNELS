# LED-AVR-THERMOMETER-2-CHANNELS
7 segment display thermometer using an AVR. 2 channel with single display. Single display with channel switching and peak monitoring.

# Introduction

In my current obsession with AVRs, I wanted to try use what I learned creating the nixie clock project for something else. 

I have been wanting a temperature monitor that displays the inside and outside temperature. So instead of just buying something, I figured it would be more fun to start another project and try to program it myself.

As I have a pile of ATmega8A and Atmega328P, along with 7 segment displays, so I decided to just use some of those. I'm just missing something to measure the temperature.

For the sensor, I chose the MCP9700 as it looks to be a nice all in one package. Using something like a thermocouple or PT100 type sensor requires signal conditioning to make the signals usable. The MCP9700 has all this built into it. It does all the sensing and signal conditioning.

The 0c voltage from the sensor is 500mV. This means that negative temperatures can be measured easily. Other options out there have 0c at 0v, so negative voltages have to be dealt with. This 500mV offset makes things so much simpler and also suitable for use outdoors.

A range of temperatures that would be suitable, as a minimum, would be between -20c to 50c. This would easily encompass the temperature extremes of where I live. I have only seen -18c many years ago and 42c just a few years ago. Mostly the temperature stays between -3c and 30c.

I'd like the device to be simple in its interface. A single screen only to avoid complexity. Some kind of peak value recording and ability to display them.

PROJECT PHOTOS & PCB RENDERS

## Project 
