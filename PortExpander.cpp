/* Arduino HAL Library - Port Expansion
 * See the README file for author and licensing information. In case it's
 * missing from your distribution, use the one here as the authoritative
 * version: https://github.com/csdexter/HAL/blob/master/README
 *
 * This library deals with expanding the I/O capabilities of the Arduino using
 * external devices such as shift registers, multiplexers and port expanders.
 * See the example sketches to learn how to use the library in your code.
 *
 * This is the main code file for the library.
 * See the header file for better function documentation.
 * ---------------------------------------------------------------------------
 */

#include "PortExpander.h"

#include <SPI.h>
#include <Wire.h>

TExpanderEntry *PortExpanders;

void digitalWriteEx(word pin, boolean value) {
  if(pin < PORTEXPANDER_FIRSTOUTPUT) digitalWrite(lowByte(pin), value);
  else PortExpanders[pin - PORTEXPANDER_FIRSTOUTPUT].expander->digitalWrite(PortExpanders[pin - PORTEXPANDER_FIRSTOUTPUT].realPin, value);
};

boolean digitalReadEx(word pin) {
  return (pin < PORTEXPANDER_FIRSTOUTPUT ? digitalRead(lowByte(pin)) : PortExpanders[pin - PORTEXPANDER_FIRSTOUTPUT].expander->digitalRead(PortExpanders[pin - PORTEXPANDER_FIRSTOUTPUT].realPin));
};

void pinModeEx(word pin, byte mode) {
  if(pin < PORTEXPANDER_FIRSTOUTPUT) pinMode(lowByte(pin), mode);
  else PortExpanders[pin - PORTEXPANDER_FIRSTOUTPUT].expander->pinMode(PortExpanders[pin - PORTEXPANDER_FIRSTOUTPUT].realPin, mode);
};


void TEC_SR_74595::begin() {
  //Make it go quiet
  if(_pinG != PEC_74595_PIN_G_HW) {
    pinModeEx(_pinG, OUTPUT);
    digitalWriteEx(_pinG, HIGH);
  };
  //Prepare the latching clock
  if(!(_useSPI && _pinRCK == SS)) pinModeEx(_pinRCK, OUTPUT);
  digitalWriteEx(_pinRCK, LOW);
  //Prepare interface
  if(!_useSPI) {
    pinModeEx(_pinSER, OUTPUT);
    pinModeEx(_pinSRCK, OUTPUT);
    digitalWriteEx(_pinSRCK, LOW);
  } else SPI.begin(); //TODO: use Arbitration
  
  //Clear by external reset line
  if(_pinSRCLR != PEC_74595_PIN_SRCLR_HW) {
    pinModeEx(_pinSRCLR, OUTPUT);
    digitalWriteEx(_pinSRCLR, LOW);
    //Dummy delay, the 74595 needs only 40ns here
    delayMicroseconds(1);
    digitalWriteEx(_pinSRCLR, HIGH);
  } else updateContents(); //Or fill with zeros

  pushAndLatch();
  
  //Enable outputs
  if(_pinG != PEC_74595_PIN_G_HW) digitalWriteEx(_pinG, LOW);
};

void TEC_SR_74595::end() {
  if(_pinG != PEC_74595_PIN_G_HW) digitalWriteEx(_pinG, HIGH);
  if(_useSPI) SPI.end(); //TODO: Use arbitration
};

void TEC_SR_74595::digitalWrite(word pin, boolean value) {
  bitWrite(_contents[pin / 8], pin % 8, value);

  updateContents();
  pushAndLatch();
};

void TEC_SR_74595::updateContents() {
  for(byte i = 0; i < _width; i++)
    if(_useSPI) SPI.transfer(_contents[i]); //TODO: use Arbitration
    else shiftOut(_pinSER, _pinSRCK, LSBFIRST, _contents[i]); //TODO: shiftOut() is not Expander-aware
};

void TEC_SR_74595::pushAndLatch() {
  digitalWriteEx(_pinRCK, HIGH);
  //Dummy delay, the 74595 needs only 20ns here
  delayMicroseconds(1);
  digitalWriteEx(_pinRCK, LOW);
};


void TEC_SR_74165::begin() {
  if(!(_useSPI && _pinSHLD == SS)) pinModeEx(_pinSHLD, OUTPUT);
  if(!_useSPI) {
    pinModeEx(_pinCLK, OUTPUT);
    pinModeEx(_pinQH, INPUT);
  } else SPI.begin(); //TODO: Use arbitration
};

void TEC_SR_74165::end() {
  if(_useSPI) SPI.end(); //TODO: Use arbitration
};

boolean TEC_SR_74165::digitalRead(word pin) {
  sampleAndLatch();
  fetchContents();
  
  return bitRead(_contents[pin / 8], pin % 8);
};

void TEC_SR_74165::sampleAndLatch() {
  digitalWriteEx(_pinSHLD, LOW);
  //Dummy delay, the 74165 only needs 15ns here
  delayMicroseconds(1);
  digitalWriteEx(_pinSHLD, HIGH);
};

void TEC_SR_74165::fetchContents() {
  for(byte i = 0; i < _width; i++)
    if(_useSPI) _contents[i] = SPI.transfer(0x00); //TODO: use Arbitration
    else _contents[i] = shiftIn(_pinQH, _pinCLK, LSBFIRST); //TODO: shiftIn() is not Expander-aware
};


void TEC_PE_PCF8575C::begin() {
  if(_pinINT != PEC_PCF8575C_PIN_INT_HW) pinModeEx(_pinINT, INPUT);
  if(_dialAddress) {
    pinModeEx(_pinA0, OUTPUT);
    pinModeEx(_pinA1, OUTPUT);
    pinModeEx(_pinA2, OUTPUT);
  };
  Wire.begin(); //TODO: use Arbitration
  
  //Do an initial read
  fetchContents();
};

void TEC_PE_PCF8575C::digitalWrite(word pin, boolean value) {
  bitWrite(_contents[pin / 8], pin % 8, value);

  if(_dialAddress) dialAddress();
  Wire.beginTransmission(_i2caddr);
  if(_wide) {
    Wire.write(highByte(*((word *)_contents)));
    Wire.write(lowByte(*((word *)_contents)));
  } else Wire.write(*_contents);
  Wire.endTransmission();
};

boolean TEC_PE_PCF8575C::digitalRead(word pin) {
  fetchContents();

  return bitRead(_contents[pin / 8], pin % 8);
};

void TEC_PE_PCF8575C::pinMode(word pin, boolean direction) {
  if(direction == INPUT) this->digitalWrite(pin, HIGH); //According to the datasheet
};

void TEC_PE_PCF8575C::dialAddress() {
  digitalWriteEx(_pinA0, bitRead(_i2caddr, 0));
  digitalWriteEx(_pinA1, bitRead(_i2caddr, 1));
  digitalWriteEx(_pinA2, bitRead(_i2caddr, 2));
  //Dummy delay, the datasheet is inconclusive, try 1 SCL period
  delayMicroseconds(2);
};

void TEC_PE_PCF8575C::fetchContents() {
  byte dummy;
  
  if(_dialAddress) dialAddress();
  Wire.requestFrom(_i2caddr, (byte)(_wide ? 2 : 1));
  while(Wire.available() != (_wide ? 2 : 1)) delayMicroseconds(16); //8 bits
  if(_wide) {
    //Make sure avr-gcc doesn't play smart on us, force execution order
    dummy = Wire.read();
    *((word *)_contents) = word(dummy, Wire.read());
  }
  else *_contents = Wire.read();
};
