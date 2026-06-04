# 2 Channel Temperature Monitor

## Indoor & Outdoor Temperature Display

In my current obsession with AVRs, I wanted to try use what I learned creating the nixie clock project for something else. 

I have been wanting a temperature monitor that displays the inside and outside temperature. So instead of just buying something, I figured it would be more fun to start another project and try to program it myself.

I have a pile of ATmega8A and Atmega328P, along with 7 segment displays. I'm just missing something to measure the temperature.

I decided to make my own one of these rather than buy a solution. It’s been a great way for me to learn to code. This projects code is a mess though, since doing this, I’ve learned more and have begun a rewrite to a state machine based format.

---

## Hardware

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

You’ll probably want an enclosure for it too. The 3D print files I made for myself are also attached.
You will likely need some kind of screen for the outdoor temperature sensor to keep readings more accurate. The sunlight will heat things up far beyond the ambient air temperature.

---

## Functional Description

For the sensor, I chose the MCP9700 as it looks to be a nice all in one package. Using something like a thermocouple or PT100 type sensor requires signal conditioning. The MCP9700 has all this built into it. It does all the sensing and signal conditioning.

The 0c voltage from the MCP9700 is 500mV. This means that negative temperatures can be measured easily. Other options out there have 0c at 0v, so negative voltages have to be dealt with. This 500mV offset makes things so much simpler and also suitable for use outdoors.

A range of temperatures that would be suitable, as a minimum, would be between -20c to 50c. This would easily encompass the temperature extremes of where I live. I have only seen -18c many years ago and 42c just a few years ago. Mostly the temperature stays between -3c and 30c. The range available for my setup is -50c to 154.6c. A temperature range of 204.6c.

The accurate voltage reference used for analogue measurements is the TL431. The output is 2.495v. The output of this is fed through a voltage divider to bring the voltage inline with the 10-bit ADC counts. The ADC can count 0 – 1023. Setting the reference voltage to 2048mV is an almost exact multiple. It’s considered to be double that of the ADC for the achievable accuracy of this setup. 2mV = 1 ADC. This setup doubles as a calibration for sensor and MCU tolerances.

At 0c the MCP9700 outputs 500mV. That is an ADC value of 250. You may see that the maximum resolution here is 0.2c. The MCP9700 output is described as 10mV / 1c. An increase of 0.1c is 1mV, but 2mV are required for a change in the ADC count. The 0.1c resolution that the project has is done through oversampling. This simplified maths helps keep oversampling super simple too.

The device has a single screen displaying temperature. It has two buttons. One for selecting which sensor is being read and the other to cycle through peak readings. These being: indoor high, indoor low, outdoor high & outdoor low. These values can be reset by shorting the jumper connections on the back of the board. This can also be connected to a button if desired.

I ended up using a mini 3 pin XLR connector for the outdoor temperature sensor, with the idea of it being removable should I wish to just have it as an indoor temperature monitor. I had the indoor sensor inside the case thinking that it would help filter out any drafts that could cause wild temperature reading changes. This did not work as the heat from the MCU was enough to make everything read higher than it should. I then moved it to the back of the case, this helped a little but drafts and room position were not optimal. In the end I soldered a wire to the PCB to run the sensor to a better room position. This made the biggest change, making things more stable. I should have used a mini XLR for this sensor too.

There is no need for the NPN transistors on the screen cathodes, they can just as well be controlled by the MCU directly, but I like having all logic in code the same. That being on is one and off is zero.

The project code is honestly a mess. I learned a lot doing this and have since learned more. It's currently half rewritten in a better way using state machines. For now, this works well enough.

## Progress

It’s actively being used and keeping an eye out for bugs.

### Done
- Build a functional temperature monitor that can read two channels
- Implement peak temperature readings and store them into EEPROM
- Add EEPROM check for knowing if to initialise memory space.
- Display text on screen when interacting with the button functions.

### Working On
- Rewriting the whole code to use a traditional state machine based structure.
- Removing known bugs.

### Next
- Nothing. A whole new project expanding on this one.

### Known Bugs
- Temperatures over 100c corrupt adjacent memory space. Needs memory allocations moving.
- Flickering display when using the button functions. Race conditions, the rewrite will fix.
---

## Photos

### Final Concept
<img width="800" height="618" alt="Temperature Monitor Enclosure angled front smol" src="https://github.com/user-attachments/assets/1f834fec-2958-40e3-94c4-f130e7e43848" />
<img width="800" height="618" alt="Temperature Monitor Enclosure angled back smol" src="https://github.com/user-attachments/assets/3b60185c-c05b-4c6b-b623-e7b32655ed60" />

### Current Build

TO DO

---

## Code

- Tools: Arduino IDE 1.8.19 – MiniCore for ATMEGA8

- I’ll refrain from doing any code breakdown here as it will need a rewrite when the new version is complete.
---

## Resources
- https://danyk.cz/avr_tep2_en.html
- https://danyk.cz/avr_tep_en.html

---
