// -------------------------------------------------------------------------------------------------------- //
//                                                                                                          //
//                                      ATmega8A 7 Segment Display                                          //
//                                       Temperature Monitor v1.4.2                                         //
//                                                                                                          //
// -------------------------------------------------------------------------------------------------------- //

// Todo List:
// Use internal clock, not external to clear port B pins (DONE)
// Move all anode output pins to port B. Pins 14, 15, 16, 17, 18, 19, 9 & 10. PB0 - 7 (DONE)
// Move all cathode pins to port D. Pins 4, 5, 6, 11, 12, 13. PD2 - 7 (Already done)
// Sort out EEPROM data overflow. Max temp is 3 digits not 2.
// Change ADC sampling rate to counter based on ISR not time.
// Pressing PEAK while IN / OUT text is shown glitches the display.
// Check what the reset eeprom function does when other text is shown.

// ***************************************** Include/Define *********************************************** //

#include <EEPROM.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// **************************************** Button Constants ********************************************** //

const byte INDOOR_SENSOR =                           A0;                         // Sensor inside input pin
const byte OUTDOOR_SENSOR =                          A1;                         // Sensor outside input pin
const byte SELECT =                                  A4;                         // Sensor select button
const byte PEAK =                                    A5;                         // Peak value display
const byte RESET =                                   A3;                         // Clear EEPROM jumper
const byte DEBOUNCE_DELAY =                         250;                         // Button debounce millis

// **************************************** EEPROM Constants ********************************************** //

const byte EEPROM_ADDRESS_SIGNITURE =                 0;                         // EEPROM initialised
const byte EEPROM_ADDRESS_IN_HIGH =                   1;                         // Indoor temp high
const byte EEPROM_ADDRESS_IN_LOW =                    3;                         // Indoor temp low
const byte EEPROM_ADDRESS_OUT_HIGH =                  5;                         // Outdoor temp high
const byte EEPROM_ADDRESS_OUT_LOW =                   7;                         // Outdoor temp low
const byte EEPROM_ADDRESS_IN_OR_OUT =                 9;                         // Last sensor displayed
const byte EEPROM_DATA_SIGNITURE =                 0x55;                         // EEPROM initialised data
const int EEPROM_DELAY =                           6000;                         // millis to prevent garbage on boot

// **************************************** Display Constants ********************************************* //

const byte NUM_DISPLAYS =                             4;
const byte NUM_SEGMENTS =                             7;
const short DISPLAY_TIME =                         2000;                         // Time to show which sensor is selected

// **************************************** Sensor Constants ********************************************** //

const byte AVERAGE_SAMPLE_TIME =                      5;                         // heartBeat count (20s)
const int SAMPLE_INTERVAL =                        4000;                         // Sample frequency ms
const byte SAMPLE_NUM =                              64;
const byte SAMPLE_SHIFT =                             5;
const byte CALIBRATION_IN =                          10;
const byte CALIBRATION_OUT =                         10;
//int V_REF =                                        2048;                         // V_REF in mV

// ***************************************** Character Data ********************************************** //

const byte ANODES[21] = {                 // DP GFEDCBA
                                             0b00111111,                         // 0
                                             0b00000110,                         // 1
                                             0b01011011,                         // 2
                                             0b01001111,                         // 3
                                             0b01100110,                         // 4
                                             0b01101101,                         // 5
                                             0b01111101,                         // 6
                                             0b00000111,                         // 7
                                             0b01111111,                         // 8
                                             0b01101111,                         // 9
                                             0b00000000,                         // 10 OFF
                                             0b01000000,                         // 11 -
                                             0b01011000,                         // 12 c
                                             0b01011001,                         // 13 c with top mark
                                             0b00110000,                         // 14 I
                                             0b00111110,                         // 15 U
                                             0b00110111,                         // 16 n
                                             0b00000001,                         // 17 Top -
                                             0b01110110,                         // 18 H
                                             0b00111000,                         // 19 L
                                             0b01111001,                         // 20 E
};
const byte CATHODES[4] = {
                                             0b10000000,                         // Display 1
                                             0b01000000,                         // Display 2
                                             0b00100000,                         // Display 3
                                             0b00010000,                         // Display 4
};
const byte DECIMAL =                         0b10000000;                         // Decimal

const byte DISPLAY_OFF =                             10;                         // Human readable display references
const byte DISPLAY_MINUS =                           11;
const byte DISPLAY_C =                               12;
const byte DISPLAY_C_MINUS =                         13;
const byte DISPLAY_I =                               14;
const byte DISPLAY_U =                               15;
const byte DISPLAY_N =                               16;
const byte DISPLAY_TOP_MINUS =                       17;
const byte DISPLAY_H =                               18;
const byte DISPLAY_L =                               19;
const byte DISPLAY_E =                               20;

// **************************************** EEPROM Variables ************************************************* //

bool eepromClear =                                false;                         // Is the EEPROM empty?
bool eepromTrashFilter =                           true;                         // EEPROM write delay at boot
int eepromDataRam[4] =                     {0, 0, 0, 0};                         // EEPROM data loaded into RAM
int eepromTrashTime =                                 0;                         // Trash filter reference time
byte eepromDataRamIndex =                             0;                         // EEPROM memory address pointer
byte nextEepromDataRamIndex =                         0;                         // 

// **************************************** Display Variables ************************************************ //

bool showPeakValues =                             false;                         // Show peak value
bool showPeakText =                               false;                         // Show which peak value to display
bool showSensorText =                             false;                         // Show which sensor has been switched
bool showReset =                                  false;
bool runNow =                                      true;
int peakDataValue =                                   0;                         // Peak value to show
int digits[4] =                            {0, 0, 0, 0};                         // What digit each display is on
byte currentDigit =                                   0;                         // Active digit on display now
unsigned long showPeakValuesStart =                   0;                         // Time when showing peak values started
unsigned long showPeakTextStart =                     0;                         // Time when showing peak text started
unsigned long showSensorTextStart =                   0;                         // Time when showing sensor info started
unsigned long showResetStart =                        0;

// **************************************** Sensor Variables ************************************************* //

bool indoorSensor =                                true;                         // Which sensor is selected
int signedIndoorTemperature =                         0;                         // Indoor temperature celcius x10
int signedOutdoorTemperature =                        0;                         // Outdoor temperature celcius x10

// ***************************************** Button Variables ********************************************* //

bool resetButtonState =                           false;
bool resetButtonStatePrev =                       false;
bool selectButtonState =                          false;
bool selectButtonStatePrev =                      false;
bool peakButtonState =                                0;
bool peakButtonStatePrev =                            0;
unsigned long buttonReferenceTime =                   0;
unsigned long buttonPreviousPushTime =                0;

// ***************************************** Global Variables ********************************************* //

unsigned long previousMicros =                        0;                         // Last update time of display micros
unsigned long previousMillis =                        0;                         // Last update time of sensor reading
volatile int heartBeat =                              0;                         // Sensor sample freq
int rollingAverageIndoor[8] =  {0, 0, 0, 0, 0, 0, 0, 0};                         // Average for stability
int rollingAverageOutdoor[8] = {0, 0, 0, 0, 0, 0, 0, 0};                         // Average for stability
int rollingAverageIndoorValue =                       0;
int rollingAverageOutdoorValue =                      0;
byte rollingAverageIndex =                            0;
bool rollingAverageFull =                         false;
int takeAverageSample =                               0;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ //

void setup(void);
void loop(void);
void eepromInitialiser(void);
void buttonHandler(byte avrPin, bool &buttonState, bool &buttonStatePrevious, unsigned long currentReferenceTime, unsigned long &buttonPushPreviousTime);
void multiplex(void);
int analogConversion(byte sensorPin);
void sensorReading(void);
void peakValues(int temperature, int &eepromData, byte EEPROM_ADDRESS, bool highCompare);
void peakDisplay(void);
void sensorDisplay(void);
void resetText(void);
void serialOutput(void);
void peakValueIndex();

// ============================================== Setup =================================================== //
void setup() {

// ADC Reference
  analogReference(EXTERNAL);

// PORTB Setup - Anodes            -     Arduino Pin -> 8, 9, 10, 11, 12, 13, PB6, PB7  -  Port Pin: PB7...0
  DDRB = 0b11111111;
  PORTB = 0b00000000;

// PORTC Setup - Sensors/Buttons   -     Arduino Pin -> A0, A1, A3, A4, A5  -  Port Pin: PC0, PC1, PC3, PC4, PC5
  DDRC = 0b00000000;
  PORTC = 0b00111100;

// PORTD Setup - Cathodes          -     Arduino Pin -> 7, 6, 5, 4   -   Port Pin: PD7...4
  DDRD = 0b11110000;
  PORTD = 0b00001111;

  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();

  for (int i = 0; i < 4; i++) {
    digits[i] = DISPLAY_MINUS;
  }
  
  eepromInitialiser();

// ------------------------------------------- Setup Interrupts ------------------------------------------ //
// ATmega328 - CTC interrupt
/* 
 * 1. TCCR2A register all 8 bits to 0 to known state.
 * 2. TCCR2B register all 8 bits to 0 to known state.
 * 3. TCNT2 register is set to 0 to ensure state is known. TCNT2 is 8-bit reg and counts up to OCR2A value.
 * 4. OCR2A register value is set to 249.
 * 5. WGM21 bit, in TCCR2A register, set to 1 = CTC mode. Clear timer on compare.
 * 6. CS20 -> CS22 bits in TCCR2B set prescaler value. Number used to divide system clock by.
 *    Each clock cycle out of the prescaler increments TCNT2 by 1. Fast clock, faster CTC.
 *      CS21 = 1 is 8. 16MHz/8 prescaler = 2MHz / 250 = 8000Hz → 125µs - @8MHz 250µs
 *      CS20 & CS21 = 1 is 64. 16MHz/64 prescaler = 500KHz / 250 = 2000Hz → 1ms - @8MHz 2ms
 *      CS22 = 1 is 256. 16MHz/256 prescaler = 62.5kHz / 250 = 250Hz → 8ms - @8MHz 16ms
 * 7. OCIE2A bit in TIMSK register set to 1 enables the CTC interrupt so the above settings work.
 */
// ATmega8 - CTC interrupt
/*
 * 1. TCCR2 register all 8 bits to 0 to known state.
 * 2. TCNT2 register is set to 0 to ensure state is known. TCNT2 is 8-bit reg and counts up to OCR2A value.
 * 3. OCR2 register value is set to 249.
 * 4. WGM21 bit, in TCCR2 register, set to 1 = CTC mode. Clear timer on compare.
 * 5. CS20 -> CS22 bits in TCCR2 set prescaler value. Number used to divide system clock by.
 *    Each clock cycle out of the prescaler increments TCNT2 by 1. Fast clock, faster CTC.
 *      CS21 = 1 is 8. 16MHz/8 prescaler = 2MHz / 250 = 8000Hz → 125µs - @8MHz 250µs
 *      CS20 & CS21 = 1 is 32. 16MHz/8 prescaler = 500KHz / 250 = 2000Hz → 500µs - @8MHz 1ms
 *      CS22 = 1 is 64. 16MHz/64 prescaler = 250kHz / 250 = 1000Hz → 1ms - @8MHz 2ms
 * 6. OCIE2 bit in TIMSK register set to 1 enables the CTC interrupt so the above settings work.
*/
// ATmega328 @ 1MHz ~1ms Interrupt
/*
  cli();
  TCCR2A = 0;
  TCCR2B = 0;
  TCNT2 = 0;
  OCR2A = 62;
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (1 << CS21);
  TIMSK2 |= (1 << OCIE2A);
  sei();
*/
// ATmega8 @ 1MHz ~1ms Interrupt

  cli();
  TCCR2 = 0;
  TCNT2 = 0;
  OCR2 = 124;
  TCCR2 |= (1 << WGM21)|(1 << CS21);
  TIMSK |= (1 << OCIE2);
  sei();
  
}
// ======================================== Timer2 Interrupts ============================================= //
/* Added interrupt for updating the screen. Removed all flicker from the display when doing math takes a
 * little longer than normal, or just badly timed.
 */

// ATmega328
/*
ISR(TIMER2_COMPA_vect) {
  multiplex();
*/

// ATmega8
ISR(TIMER2_COMP_vect) {
  heartBeat++;
  multiplex();

}
// ============================================== Main ==================================================== //
/* Standard stuff. mostly just calling other functions and some having data passed to them. The sensor sample
 * rate is controlled in here with SAMPLE_INTERVAL. This will need some adjustment when I get around to doing
 * oversampling. Current analog values are easily converted from the input values with just a -500 and faking
 * a decimal place. To get 11 bit samples that wont work. Max value 1023 will then be 4095. So maybe -2000?
 * eg 4095 - 2000 = 2095. 2095 / 4 = 523. Which doesnt give anymore accuracy. Hmm. 4095 x 10 = 40950. Then
 * 40950 - 20000 = 20950. 20950 / 4 = 5237. That would be interpreted as 52.37c. Maybe upgrade to a 6 segment
 * display for a future version.
 */
void loop() {

  buttonReferenceTime = millis();

  buttonHandler(SELECT, selectButtonState, selectButtonStatePrev, buttonReferenceTime, buttonPreviousPushTime);
  buttonHandler(PEAK, peakButtonState, peakButtonStatePrev, buttonReferenceTime, buttonPreviousPushTime);
  buttonHandler(RESET, resetButtonState, resetButtonStatePrev, buttonReferenceTime, buttonPreviousPushTime);

  cli();
  int hb = heartBeat;
  sei();

  if (hb >= SAMPLE_INTERVAL) {
    heartBeat = 0;
    takeAverageSample++;
    sensorReading();
    peakValues(rollingAverageIndoorValue, eepromDataRam[0], EEPROM_ADDRESS_IN_HIGH, true);
    peakValues(rollingAverageIndoorValue, eepromDataRam[1], EEPROM_ADDRESS_IN_LOW, false);
    peakValues(rollingAverageOutdoorValue, eepromDataRam[2], EEPROM_ADDRESS_OUT_HIGH, true);
    peakValues(rollingAverageOutdoorValue, eepromDataRam[3], EEPROM_ADDRESS_OUT_LOW, false);
  }
  
  if (showPeakValues || showPeakText) {
    peakDisplay();
  }
  
  if (showSensorText) {
    sensorDisplay();
  }
  
  if (showReset) {
    resetText();
  }
  sleep_cpu();
}
// ========================================= EEPROM Initialiser =========================================== //
/* Sets up EEPROM to a known state. If an old chip is used or it has become corrupted. Runs eavery startup
 * and checks if it has been been run already. Only other time it is called is when the reset input jumper
 * has been connected to clear. Sometimes I guess that youll want to reset the peak temperatures. 
 */
void eepromInitialiser() {

  if ((EEPROM.read(EEPROM_ADDRESS_SIGNITURE) != EEPROM_DATA_SIGNITURE) || (eepromClear)) {
    eepromClear = false;
//    Serial.println("EEPROM Corrupt!");
//    delay(500);
    
    for (byte i = 1; i < 3; i++) {
      EEPROM.write(i, -50);
    }
    for (byte i = 3; i < 5; i++) {
      EEPROM.write(i, 50);
    }
    for (byte i = 5; i < 7; i++) {
      EEPROM.write(i, -50);
    }
    for (byte i = 7; i < 9; i++) {
      EEPROM.write(i, 50);
    }
    EEPROM.write(EEPROM_ADDRESS_IN_OR_OUT, 0);    
    EEPROM.write(EEPROM_ADDRESS_SIGNITURE, EEPROM_DATA_SIGNITURE);
  }

  if (EEPROM.read(EEPROM_ADDRESS_SIGNITURE) != EEPROM_DATA_SIGNITURE) {
//    Serial.println("EEPROM Bad :(");
    for (int i = 0; i < 4; i++) {
      eepromDataRam[i] = 99;
    }
  }
  if (EEPROM.read(EEPROM_ADDRESS_SIGNITURE) == EEPROM_DATA_SIGNITURE) {
//    Serial.println("EEPROM Good :)");
    EEPROM.get(EEPROM_ADDRESS_IN_HIGH, eepromDataRam[0]);
    EEPROM.get(EEPROM_ADDRESS_IN_LOW, eepromDataRam[1]);
    EEPROM.get(EEPROM_ADDRESS_OUT_HIGH, eepromDataRam[2]);
    EEPROM.get(EEPROM_ADDRESS_OUT_LOW, eepromDataRam[3]);
  }
//  eepromTrashTime = millis();
}
// ========================================== Button Handler ============================================== //
/* Standard button handler function. It's called each time a button is pressed to se what to do with it.
 * Uses millis and various bools to figure out how long to ignore the buttons for after pressing to stop
 * stop button bounce. It also tracks the states of the buttons to prevent holding of the button causing 
 * repeated triggers.
 */
void buttonHandler(byte avrPin, bool &buttonState, bool &buttonStatePrevious, unsigned long currentReferenceTime, unsigned long &buttonPushPreviousTime) {

  buttonState = !digitalRead(avrPin);

  if (buttonState && !buttonStatePrevious && (currentReferenceTime - buttonPushPreviousTime > DEBOUNCE_DELAY)) {
    switch (avrPin) {
      case SELECT:
        indoorSensor = !indoorSensor;
        showSensorText = true;
        showSensorTextStart = currentReferenceTime;
        break;
      case PEAK:
        peakValueIndex();
        break;
      case RESET:
        eepromClear = true;
        eepromInitialiser();
        showReset = true;
        showResetStart = currentReferenceTime;
        resetText();
        break;
    }
    buttonPushPreviousTime = currentReferenceTime; 
  }
  buttonStatePrevious = buttonState;
}
// ============================================ Multiplex ================================================= //
/* This is the display driver. This pulls data from an array on what to display on the screen. Don't touch
 * this as it works fine. Clears only the last digit displayed instead of all four, it is much faster.
 * Currentley cathodes are inverted due to running out of NPN transistors for testing. Needs inverting for
 * when using NPN transistors. When digit 2 is displayed, the function always switches on the decimal place
 * so long as another display function isn't currently running. Need to add blocking for peak display function
 * also when its tested.
 */

void multiplex() {

  byte num = digits[currentDigit];
  byte anodes = ANODES[num];
  byte cathodes = CATHODES[currentDigit];

  if ((currentDigit == 2) && !showPeakText && !showSensorText && !showReset) {
    anodes |= DECIMAL;
  }

  PORTD = 0;
  PORTB = anodes;
  PORTD = cathodes;
  currentDigit++;

  if (currentDigit >= NUM_DISPLAYS) {
    currentDigit = 0;
  }
}
// ========================================= Celcius Conversion =========================================== //
/* This converts the temperature sensor values on the input pins to a human readable format. I chose to not
 * mess with floats and fake the final displayed number. This returns a value that is x10 higher than the
 * real number. This avoids using floating point numbers and the decimal place is added in manually
 * on the screen.
 */
int analogConversion(byte sensorPin) {

  long sum = 0;
  long voltage = 0;
  long temp = 0;
  analogRead(sensorPin);
                                      
  for (byte i = 0; i < SAMPLE_NUM; i++) {
    sum += analogRead(sensorPin);                // 254 + 255 + 253 + 255 = 1017
  }

  voltage = sum >> SAMPLE_SHIFT;            // 1017 / 2 = 509 (508.5)
  if (sensorPin == INDOOR_SENSOR) {
    temp = (voltage - (500 + CALIBRATION_IN));    // 509 - 500 = 9
  } else {
    temp = (voltage - (500 + CALIBRATION_OUT));
  }

  return temp;
}
// =========================================== Sensor Reading ============================================= //
/* Poor name for a function. This is what assigns the numbers to the array for multiplex() to use. The signed
 * value is saved to a variable to be used elsewhere. Depending on which sensor reading is to be displayed,
 * the value passed to the changes. The signed value is check if it is positive or negative. That is then
 * converted to an unsigned number, so the values can reference the character index properly. Negative numbers
 * are tracked manually like the decimal point.
 */
void sensorReading() {

  int displayedTemperature = 0; 
  int celsius = 0;
  int sumIn = 0;
  int sumOut = 0;
  
  if (!rollingAverageFull) {
    signedIndoorTemperature = analogConversion(INDOOR_SENSOR);  
    signedOutdoorTemperature = analogConversion(OUTDOOR_SENSOR);
  } else if (rollingAverageFull && (takeAverageSample >= AVERAGE_SAMPLE_TIME)) {
    takeAverageSample = 0;
    signedIndoorTemperature = analogConversion(INDOOR_SENSOR);  
    signedOutdoorTemperature = analogConversion(OUTDOOR_SENSOR);
    rollingAverageIndoor[rollingAverageIndex] = signedIndoorTemperature;
    rollingAverageOutdoor[rollingAverageIndex] = signedOutdoorTemperature;
      for (byte i = 0; i < 8; i++) {
        sumIn += rollingAverageIndoor[i];
        sumOut += rollingAverageOutdoor[i];
      }
    rollingAverageIndoorValue = sumIn >> 3;                 // This is a mess.
    rollingAverageOutdoorValue = sumOut >> 3;
    rollingAverageIndex++;
    if (rollingAverageIndex >= 8) {
      rollingAverageIndex = 0;
      rollingAverageFull = 1;
    }

  
  if (!rollingAverageFull) {
    if (indoorSensor) {
      displayedTemperature = signedIndoorTemperature;    
    } else if (!indoorSensor) {
      displayedTemperature = signedOutdoorTemperature;
    }
  } else {
    if (indoorSensor) {
      displayedTemperature = rollingAverageIndoorValue;    
    } else if (!indoorSensor) {
      displayedTemperature = rollingAverageOutdoorValue;
    }
  }
  celsius = abs(displayedTemperature);
  
  digits[3] = (celsius / 100) % 10;
  digits[2] = (celsius / 10) % 10;
  digits[1] = celsius % 10;

  if (displayedTemperature >= 0) {
    digits[0] = DISPLAY_C;
  } else {
    digits[0] = DISPLAY_C_MINUS;
  }
}
// ========================================= Peak Temperatures ============================================ //
/*  */
void peakValues(int temperature, int &eepromData, byte EEPROM_ADDRESS, bool highCompare) {

  if (rollingAverageFull && ((highCompare && temperature > eepromData) || (!highCompare && temperature < eepromData))) {
    eepromData = temperature;
    EEPROM.put(EEPROM_ADDRESS, eepromData);
  }    
}
// ======================================= Peak Value Selected ============================================ //
/* Used in conjunction the peakDisplay() function. Tracks the index pointer and increments it. Stores current
 * pointer values into variable before incrementing it for the next call. Didn't bother to reset the index
 * when it is done with. Might come back later and do that so it is in a known state each time.
 */
void peakValueIndex() {

  showPeakText = true;
  showPeakValues = false;
  showPeakTextStart = millis();
  peakDataValue = eepromDataRam[nextEepromDataRamIndex];
  eepromDataRamIndex = nextEepromDataRamIndex;
  nextEepromDataRamIndex++;
  
  if (nextEepromDataRamIndex >= 4) nextEepromDataRamIndex = 0;
}
// ========================================= Show Peak Temps ============================================== //
/* Dual purpose function to display first what data is going to be displayed, then followed up by the data
 * itself. Using the same DISPLAY_TIME constant for each, I think different times would look better. The
 * second part of this function isn't run until the first section has finished and sets the flag to run the
 * second section.
 */
void peakDisplay() {

  if (showPeakText && ((millis() - showPeakTextStart) >= DISPLAY_TIME)) {
    showPeakText = false;
    showPeakValues = true;
    showPeakValuesStart = millis();
  } else if (showPeakText) {
    switch (eepromDataRamIndex) {
      case 0:
        digits[3] = DISPLAY_I;
        digits[2] = DISPLAY_N;
        digits[1] = DISPLAY_OFF;
        digits[0] = DISPLAY_H;
        break;
      case 1:
        digits[3] = DISPLAY_I;
        digits[2] = DISPLAY_N;
        digits[1] = DISPLAY_OFF;
        digits[0] = DISPLAY_L;
        break;
      case 2:
        digits[3] = 0;
        digits[2] = DISPLAY_U;
        digits[1] = 7;
        digits[0] = DISPLAY_H;
        break;
      case 3:
        digits[3] = 0;
        digits[2] = DISPLAY_U;
        digits[1] = 7;
        digits[0] = DISPLAY_L;
        break;
    }
  }
  if (showPeakValues && (millis() - showPeakValuesStart > DISPLAY_TIME)) {               // Decide when time is up
    showPeakValues = false;
    
  } else if (showPeakValues) {
    int temp = abs(peakDataValue);
    
    digits[3] = (temp / 100) % 10;
    digits[2] = (temp / 10) % 10;
    digits[1] = temp % 10;
    
      if (peakDataValue < 0) {
        digits[0] = DISPLAY_C_MINUS;
      } else {
        digits[0] = DISPLAY_C;
      }
   }
}
// =========================================== Text/Display =============================================== //
/* When the sensor value being displayed is switched between sensors, the screen shows which sensor is being
 * shown next. Simply takes a bool that is inverted when the button is pressed, then uses it to decide on the
 * text to be shown.
 */
void sensorDisplay() {

  if (showSensorText && (millis() - showSensorTextStart > DISPLAY_TIME)) {         // Decide when time is up
    showSensorText = false;
    sensorReading();
    return;
  }
  if (indoorSensor) {
    digits[3] = DISPLAY_OFF;
    digits[2] = DISPLAY_I;
    digits[1] = DISPLAY_N;
    digits[0] = DISPLAY_OFF;
  } else {
    digits[3] = 0;
    digits[2] = DISPLAY_U;
    digits[1] = 7;
    digits[0] = DISPLAY_TOP_MINUS;
  }
}
// =========================================== Show Reset Text ============================================ //

void resetText() {
  
  if (showReset && (millis() - showResetStart > DISPLAY_TIME)) {         // Decide when time is up
    showReset = false;
    return;
  } else {
    digits[3] = 5;
    digits[2] = DISPLAY_E;
    digits[1] = 7;
    digits[0] = DISPLAY_TOP_MINUS;
  }
}
