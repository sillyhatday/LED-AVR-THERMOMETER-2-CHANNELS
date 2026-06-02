// -------------------------------------------------------------------------------------------------------- //
//                                                                                                          //
//                                      ATmega8A 7 Segment Display                                          //
//                                       Temperature Monitor v2.0                                           //
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
// Look up table to replace large switches.

// ***************************************** Include/Define *********************************************** //

#include <EEPROM.h>
//#include <avr/string.h>
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
const uint16_t DISPLAY_TIME =                      2000;                         // Time to show which sensor is selected
const uint16_t DISPLAY_TIME_SHORT =                1000;

// **************************************** Sensor Constants ********************************************** //

const byte AVERAGE_SAMPLE_TIME =                      5;                         // heartBeat count (20s)
const int SAMPLE_INTERVAL =                        4000;                         // Sample frequency ms
const byte SAMPLE_NUM =                              64;
const byte SAMPLE_SHIFT =                             5;
const byte CALIBRATION_IN =                          10;
const byte CALIBRATION_OUT =                         10;

// ***************************************** Character Data ********************************************** //

const byte ANODES[22] = {                 // DP GFEDCBA
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
                                             0b01111000                          // 21 t
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
const uint8_t DISPLAY_T =                            21;

// **************************************** EEPROM Variables ************************************************* //

uint8_t eepromClear =                             false;                         // Is the EEPROM empty?
int16_t eepromDataRam[4] =                 {0, 0, 0, 0};                         // EEPROM data loaded into RAM

// **************************************** Display Variables ************************************************ //

uint16_t peakDataValue =                              0;                         // Peak value to show
uint8_t digits[4] =                        {0, 0, 0, 0};                         // Main display buffer
uint8_t live[4] =                          {0, 0, 0, 0};                         // Live temperature reading
uint8_t text[4] =                          {0, 0, 0, 0};                         // Text buffer
uint8_t peak[4] =                          {0, 0, 0, 0};                         // Peak value buffer
uint8_t currentDigit =                                0;                         // Active digit on display now
uint32_t showTextStart =                              0;                         // Time when showing sensor info started
uint32_t showValuesStart =                            0;

// **************************************** Sensor Variables ************************************************* //

int16_t signedIndoorTemperature =                     0;                         // Indoor temperature celcius x10
int16_t signedOutdoorTemperature =                    0;                         // Outdoor temperature celcius x10
volatile uint16_t heartBeat =                         0;                         // Sensor sample freq

// ***************************************** Button Variables ********************************************* //

uint8_t resetButtonState =                           false;
uint8_t resetButtonStatePrev =                       false;
uint8_t selectButtonState =                          false;
uint8_t selectButtonStatePrev =                      false;
uint8_t peakButtonState =                            false;
uint8_t peakButtonStatePrev =                        false;
uint32_t buttonReferenceTime =                           0;
uint32_t buttonPreviousPushTime =                        0;

// ***************************************** Global Variables ********************************************* //

uint32_t previousMicros =                                  0;                         // Last update time of display micros
uint32_t previousMillis =                                  0;                         // Last update time of sensor reading
uint16_t rollingAverageIndoor[8] =  {0, 0, 0, 0, 0, 0, 0, 0};                         // Average for stability
uint16_t rollingAverageOutdoor[8] = {0, 0, 0, 0, 0, 0, 0, 0};                         // Average for stability
uint16_t rollingAverageIndoorValue =                       0;
uint16_t rollingAverageOutdoorValue =                      0;
uint8_t rollingAverageIndex =                              0;
uint8_t rollingAverageFull =                           false;
uint16_t takeAverageSample =                               0;
uint8_t modeChangeBlock =                              false;

// ========================================== Enumerations ================================================ //

  typedef enum {
    MODE_LIVE,
    MODE_TEXT,
    MODE_PEAK
  } SystemState;

  typedef enum {
    TEXT_IN,
    TEXT_OUT,
    TEXT_IN_LOW,
    TEXT_IN_HIGH,
    TEXT_OUT_LOW,
    TEXT_OUT_HIGH,
    TEXT_RESET,
    TEXT_BLANK
  } Text;

  typedef enum {
    SENSOR_IN,
    SENSOR_OUT
  } Transducers;

  SystemState modeCurrent = MODE_LIVE;
  SystemState modeRequest = MODE_LIVE;
  SystemState modeReturn = MODE_LIVE;
  Text textCurrent = TEXT_BLANK;
  Text peakMenuPosition = TEXT_IN_LOW;
  Transducers sensorCurrent = SENSOR_IN;
  Transducers sensorRequest = SENSOR_IN;

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Functions ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ //

void setup(void);
void loop(void);
void eepromInitialiser(void);


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

  cli();
  TCCR2 = 0;
  TCNT2 = 0;
  OCR2 = 124;
  TCCR2 |= (1 << WGM21)|(1 << CS21);
  TIMSK |= (1 << OCIE2);
  sei();
  
}
// ======================================== Timer2 Interrupt ============================================== //

ISR(TIMER2_COMP_vect) {
  heartBeat++;
  multiplex();

}
// ============================================== Main ==================================================== //

void loop() {
  /*
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
  */
  systemState();
  displayBufferSelect();

  if (modeCurrent == MODE_TEXT) {
    if (millis() - showTextStart >= DISPLAY_TIME) {
      modeRequest = modeReturn;
      modeChangeBlock = false;
    }
  }

  if (modeCurrent == MODE_PEAK) {
    if (millis() - showValuesStart >= DISPLAY_TIME_SHORT) {
      modeRequest = modeReturn;
      modeChangeBlock = false;
    }
  }
}
// =========================================== Main State ================================================= //

void systemState() {

  if (modeCurrent != modeRequest) {
    switch (modeRequest) {
      case MODE_LIVE:
        modeCurrent = MODE_LIVE;
        // do a thing
        break;
      case MODE_TEXT:
        modeCurrent = MODE_TEXT;
        modeChangeBlock = true;
        break;
      case MODE_PEAK:
        modeCurrent = MODE_PEAK;
        modeChangeBlock = true;
        break;
    }
  }
}
// =========================================== Display Buffer ============================================= //

void displayBufferSelect() {

  switch (modeCurrent) {
    case MODE_LIVE:
      liveDisplayBuffer();
      memcpy(digits, live, 4);
      break;
    case MODE_TEXT:
      textDisplayBuffer(textCurrent);
      memcpy(digits, text, 4);
      break;
    case MODE_PEAK:
      peakDisplayBuffer();
      memcpy(digits, peak, 4);
      break;
  }
}
// ========================================== Button Handler ============================================== //

void buttonHandler(byte avrPin, bool &buttonState, bool &buttonStatePrevious, unsigned long currentReferenceTime, unsigned long &buttonPushPreviousTime) {

  buttonState = !digitalRead(avrPin);

  if (buttonState && !buttonStatePrevious && (currentReferenceTime - buttonPushPreviousTime > DEBOUNCE_DELAY)) {
    switch (avrPin) {
      case SELECT:
        selectButton();
        break;
      case PEAK:
        peakButton();
        break;
      case RESET:
        resetButton();
        break;
    }
    buttonPushPreviousTime = currentReferenceTime; 
  }
  buttonStatePrevious = buttonState;
}
// =========================================== Select Button ============================================== //

void selectButton() {

  if (modeChangeBlock) {
    return;
  }
  
  sensorCurrent = (sensorCurrent == SENSOR_IN) ? SENSOR_OUT : SENSOR_IN;           // toggle sensor

  if (sensorCurrent == SENSOR_IN) {
    textCurrent = TEXT_IN;
  } else {
    textCurrent = TEXT_OUT;
  }
  modeReturn = MODE_LIVE;
  modeRequest = MODE_TEXT;
  showTextStart = millis();
}
// ============================================ Peak Button =============================================== //

void peakButton() {

  if (modeChangeBlock) {
    return;
  }
  
  peakMenuPosition = nextPeakState(peakMenuPosition);
  textCurrent = peakMenuPosition;
  modeReturn = MODE_PEAK;
  modeRequest = MODE_TEXT;
  showTextStart = millis();
}
// ========================================== Reset Button ================================================ //

void resetButton() {

  
}
// ========================================== Peak Menu Index ============================================= //

Text nextPeakState(Text index) {
  
  switch (index) {
    case TEXT_IN_LOW:
      return TEXT_IN_HIGH;
    case TEXT_IN_HIGH:
      return TEXT_OUT_LOW;
    case TEXT_OUT_LOW:
      return TEXT_OUT_HIGH;
    case TEXT_OUT_HIGH:
      return TEXT_IN_LOW;
    default:
      return TEXT_IN_LOW;
  }
}
// ========================================= EEPROM Initialiser =========================================== //

void eepromInitialiser() {

  if ((EEPROM.read(EEPROM_ADDRESS_SIGNITURE) != EEPROM_DATA_SIGNITURE) || (eepromClear)) {
    eepromClear = false;
    
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
    for (int i = 0; i < 4; i++) {
      eepromDataRam[i] = 99;
    }
  }
  if (EEPROM.read(EEPROM_ADDRESS_SIGNITURE) == EEPROM_DATA_SIGNITURE) {
    EEPROM.get(EEPROM_ADDRESS_IN_HIGH, eepromDataRam[0]);
    EEPROM.get(EEPROM_ADDRESS_IN_LOW, eepromDataRam[1]);
    EEPROM.get(EEPROM_ADDRESS_OUT_HIGH, eepromDataRam[2]);
    EEPROM.get(EEPROM_ADDRESS_OUT_LOW, eepromDataRam[3]);
  }
}

// ============================================ Multiplex ================================================= //

void multiplex() {

  byte num = digits[currentDigit];
  byte anodes = ANODES[num];
  byte cathodes = CATHODES[currentDigit];

  PORTD = 0;
  PORTB = anodes;
  PORTD = cathodes;
  currentDigit++;

  if (currentDigit >= NUM_DISPLAYS) {
    currentDigit = 0;
  }
}
// ========================================= Live Display Buffer ========================================== //

void liveDisplayBuffer() {

  
}
// ========================================= Text Display Buffer ========================================== //

void textDisplayBuffer(Text textToDisplay) {

  switch (textToDisplay) {
    case TEXT_IN:
      text[3] = DISPLAY_OFF;
      text[2] = DISPLAY_I;
      text[1] = DISPLAY_N;
      text[0] = DISPLAY_OFF;
      break;
    case TEXT_OUT:
      text[3] = 0;
      text[2] = DISPLAY_U;
      text[1] = DISPLAY_T;
      text[0] = DISPLAY_OFF;
      break;
    case TEXT_IN_LOW:
      text[3] = DISPLAY_I;
      text[2] = DISPLAY_N;
      text[1] = DISPLAY_OFF;
      text[0] = DISPLAY_L;
      break;
    case TEXT_IN_HIGH:
      text[3] = DISPLAY_I;
      text[2] = DISPLAY_N;
      text[1] = DISPLAY_OFF;
      text[0] = DISPLAY_H;
      break;
    case TEXT_OUT_LOW:
      text[3] = 0;
      text[2] = DISPLAY_U;
      text[1] = DISPLAY_T;
      text[0] = DISPLAY_L;
      break;
    case TEXT_OUT_HIGH:
      text[3] = 0;
      text[2] = DISPLAY_U;
      text[1] = DISPLAY_T;
      text[0] = DISPLAY_H;
      break;
    case TEXT_RESET:
      text[3] = 5;
      text[2] = DISPLAY_E;
      text[1] = DISPLAY_T;
      text[0] = DISPLAY_OFF;
      break;
    case TEXT_BLANK:
      text[3] = DISPLAY_OFF;
      text[2] = DISPLAY_OFF;
      text[1] = DISPLAY_OFF;
      text[0] = DISPLAY_OFF;
      break;
  }
}
// ========================================= Peak Display Buffer ========================================== //

void peakDisplayBuffer() {

  switch (textCurrent) {
    case TEXT_IN_LOW:
      tempToDigitsPeak(eepromDataRam[1]);
      break;
    case TEXT_IN_HIGH:
      tempToDigitsPeak(eepromDataRam[0]);
      break;
    case TEXT_OUT_LOW:
      tempToDigitsPeak(eepromDataRam[3]);
      break;
    case TEXT_OUT_HIGH:
      tempToDigitsPeak(eepromDataRam[2]);
      break;
  }
}
// ============================================ Data Divider ============================================== //

void tempToDigitsPeak(int16_t dataIn) {

  int16_t x = dataIn;

  peak[3] = (x / 100) % 10;
  peak[2] = ((x / 10) % 10) | DECIMAL;
  peak[1] = x % 10;

  if (dataIn < 0) {
    peak[0] = DISPLAY_C_MINUS;
  } else {
    peak[0] = DISPLAY_C;
  }
}
// ============================================ Data Divider ============================================== //

void tempToDigitsLive(uint16_t dataIn) {
  
  int16_t x = dataIn;

  live[3] = (x / 100) % 10;
  live[2] = ((x / 10) % 10) | DECIMAL;
  live[1] = x % 10;

  if (dataIn < 0) {
    live[0] = DISPLAY_C_MINUS;
  } else {
    live[0] = DISPLAY_C;
  } 
}
// ========================================= Celcius Conversion =========================================== //

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
/*
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
} */
// ========================================= Peak Temperatures ============================================ //
/*
void peakValues(int temperature, int &eepromData, byte EEPROM_ADDRESS, bool highCompare) {

  if (rollingAverageFull && ((highCompare && temperature > eepromData) || (!highCompare && temperature < eepromData))) {
    eepromData = temperature;
    EEPROM.put(EEPROM_ADDRESS, eepromData);
  }    
} */
