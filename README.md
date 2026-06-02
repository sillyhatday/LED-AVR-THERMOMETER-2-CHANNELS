# LED-AVR-THERMOMETER-2-CHANNELS
7 segment display thermometer using an AVR. 2 channel with single display. Single display with channel switching and peak monitoring.

# Introduction

In my current obsession with AVRs, I wanted to try use what I learned creating the nixie clock project for something else. 

I have been wanting a temperature monitor that displays the inside and outside temperature. So instead of just buying something, I figured it would be more fun to start another project and try to program it myself.

I have a pile of ATmega8A and Atmega328P, along with 7 segment displays. I'm just missing something to measure the temperature.

For the sensor, I chose the MCP9700 as it looks to be a nice all in one package. Using something like a thermocouple or PT100 type sensor requires signal conditioning. The MCP9700 has all this built into it. It does all the sensing and signal conditioning.

The 0c voltage from the MCP9700 is 500mV. This means that negative temperatures can be measured easily. Other options out there have 0c at 0v, so negative voltages have to be dealt with. This 500mV offset makes things so much simpler and also suitable for use outdoors.

A range of temperatures that would be suitable, as a minimum, would be between -20c to 50c. This would easily encompass the temperature extremes of where I live. I have only seen -18c many years ago and 42c just a few years ago. Mostly the temperature stays between -3c and 30c. The range available for my setup is -50c to 154.6c. A temperature range of 204.6c.

The device has a single screen displaying temperature. It has two buttons. One for selecting which sensor is being read and the other to cycle through peak readings. These being: indoor high, indoor low, outdoor high & outdoor low. These values can be reset by shorting the jumper connections on the back of the board. This can also be connected to a button if desired.

I ended up using a mini 3 pin XLR conector for the outdoor temperature sensor, with the idea of it being removable should I wish to just have it as an indoor temperature monitor. I had the indoor sensor inside the case thinking that it would help filter out any drafts that could cause wild temperature reading changes. This did not work as the heat from the MCU was enough to make everything read higher than it should. I then moved it to the back of the case, this helped a little but drafts and room position were not optimal. In the end I soldered a wire to the PCB to run the sensor to a better room position. This made the biggest change, making things more stable. I should have used a mini XLR for this sensor too.

There is no need for the NPN transistors on the screen cathodes, they can just as well be controlled by the MCU directly, bnut I like haveing all logic in code the same for the moment.

The project code is honestly a mess. I learned a lot doing this and have since learned more. It's currently half rewritten in a better way using state machines. For now, this works well enough. 

PROJECT PHOTOS & PCB RENDERS

## Project Parts

| Part Description | Part Number | Package | Quantity |
| --------------------- | ------------------ | ----------- | ----------- |
| Microcontroller | ATmega8A | DIP-28 | 1 |
| 4x 7 Segment Display | SR420561N | ?? | 1 |
| Voltage Reference | TL431 | TO-92-3 | 1 |
| Temperature Sensor | MCP9700 | TO-92 | 1 |
| Resistors | 470R | L6.3xD2.5xP10.16 | 8 |
| Resistors | 1K | L6.3xD2.5xP10.16 | 1 |
| Resistors | 10K | L6.3xD2.5xP10.16 | 7 |
| Capacitors | 20pF | D3.4xW2.1xP2.5 | 2 |
| Capacitors | 100nF | D3.0xW2.0xP2.5 | 3 |
| Capacitors | 1uF | ?? | 2 |
| Capacitors | 10uF | ?? | 1 |
| Buttons | PS-7054DVB-6PN | ?? | 2 |
