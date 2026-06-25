# 2 Channel Temperature Monitor

## 📢 Introduction

This came about as a break from the [nixie clock](https://github.com/sillyhatday/IN-12-NIXIE-CLOCK) project that was taking a long time to progress.

It is a 2 channel temperature monitor that is intended for inside and outside temperature monitoring.

>[!NOTE]
>While this project is functional, the code is a mess. It requires a rewrite.

---

## 🔨 Hardware

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

---

## 📔 Functional Description

* The sensors used for temperature sensing are two MCP9700 packages. These are easy to use, not requiring external circuit conditioning. These can do negative temperatures without needing negative voltages, using a 500mV offset for 0c.

* For an air temperature monitor, the range does not need to be extreme. For the temperatures expected in Europe, -20c to 50c would be more than enough. For the most part things are between -3c and 35c. The range available for this project is -50c to 154.6c. A span of 204.6c.

* An accurate voltage refernce is needed to compare the sensor voltage with. For this the TL431 is used, it is another easy to use soloution. It has an output of 2.495v. For easy maths (and a large shortcut), having a reference voltage a multiple of the maximum ADC bits makes things simple. The 10-bit ADC is 0 - 1023. Setting the refrence voltage to 2.048v, through the use of a voltage divider network, is close enough to double.

* You may see that the finest change of temperature is 0.2c. That is 2048mV / 1023ADC = 2mV. The MCP9700 sensor output changes 10mV / 1c. As this setup requires 2mV per 1 ADC, 10mV / 2mV = 5. Finally, 5 ADC changes per 1c means, 1c / 5 = 0.2c. Should there not be a nice relation between voltage and ADC, the maths become more involved. Along the lines of: ADC reading > convert ADC to voltage > convert voltage to temperature. This can involve the use of floating point numbers. When getting to oversamlping, this complicates things further.

* The device has a single screen displaying temperature. It has two buttons. One for selecting which sensor is being displayed and the other to cycle through peak readings. These being: indoor high, indoor low, outdoor high & outdoor low. Peak readings are always being updated regardless of what is actively displayed on screen. These values can be reset by shorting the jumper connections on the back of the board. This can also be connected to a button if desired.

* I ended up using a mini 3 pin XLR connector for the outdoor temperature sensor, with the idea of it being removable should I wish to not use it, or I need to disconnect my outdoor sensor that isn't easily movable. I had the indoor sensor inside the case, the thought being it would help filter out any air current that could cause wild temperature reading changes. This did not work as the heat from the MCU was enough to ruin the accuracy. For accurate readings, the sensor should also be on a wire and positioned in the rood so that it is away from strong air currents. There is software averaging to reduce wild display changes.

>[!NOTE]
>The project code is honestly a mess. It's currently half rewritten in a better way using state machines. For now, this works well enough.

>[!TIP]
>Direct sunlight will heat up the outdoor sensor. A screen is required to help reduce this effect.

## ⚫ Progress

### 🟢 Done
- Build a functional temperature monitor that can read two channels
- Implement peak temperature readings and store them into EEPROM
- Add EEPROM check for knowing if to initialise memory space.
- Display text on screen when interacting with the button functions.

### 🟡 Working On
- Rewriting the whole code to use a traditional state machine based structure.
- Removing known bugs.

### ⚪ Next
- Nothing. A whole new project expanding on this one.

### 🔴 Known Bugs
- Temperatures over 100c corrupt adjacent memory space. Needs memory allocations moving.
- Flickering display when using the button functions. Race conditions, the rewrite will fix.
  
---

## Photos

### Final Concept
<img width="500" height="386" alt="Temperature Monitor Enclosure angled front smol" src="https://github.com/user-attachments/assets/1f834fec-2958-40e3-94c4-f130e7e43848" />
<img width="500" height="386" alt="Temperature Monitor Enclosure angled back smol" src="https://github.com/user-attachments/assets/3b60185c-c05b-4c6b-b623-e7b32655ed60" />

### Current Build

TO DO

---

## Tools

- Tools: Arduino IDE 1.8.19 – MiniCore for ATMEGA8

---

## Links

🛍️ Inside Gadgets Shop: https://shop.insidegadgets.com <br>
📎 Inside Gadgets GitHub: https://github.com/insidegadgets <br>
📷 Inside Gadgets Insta: https://www.instagram.com/inside.gadgets <br>
🖇️ Bytendo Mods GitHub: https://github.com/bytendomods <br>
🐭 Bucket Mouse: https://github.com/Bucket-Mouse <br>
🛒 Natalie Shop: https://nataliethenerd.com/ <br>
🤓 Natalie GitHub: https://github.com/natalie-lang/natalie <br>
💡 Danyk Project 1: https://danyk.cz/avr_tep2_en.html <br>
🌡️ Danyk Project 2: https://danyk.cz/avr_tep_en.html <br>

---
