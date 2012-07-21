/* Arduino HAL Library - Port Expansion
 * See the README file for author and licensing information. In case it's
 * missing from your distribution, use the one here as the authoritative
 * version: https://github.com/csdexter/HAL/blob/master/README
 *
 * This library deals with expanding the I/O capabilities of the Arduino using
 * external devices such as shift registers, multiplexers and port expanders.
 * See the example sketches to learn how to use the library in your code.
 *
 * This is the main include file for the library.
 * ---------------------------------------------------------------------------
 */

#ifndef _PORTEXPANDER_H_INCLUDED
#define _PORTEXPANDER_H_INCLUDED

#if defined(ARDUINO) && ARDUINO >= 100
# include <Arduino.h>  
#else
# include <WProgram.h>  
#endif

#define PORTEXPANDER_DIRECTION_IN 0x01
#define PORTEXPANDER_DIRECTION_OUT 0x02

// Chosen to be higher than any other Arduino pin
#define PORTEXPANDER_FIRSTOUTPUT 0x0100

void digitalWriteEx(word pin, boolean value);
boolean digitalReadEx(word pin);
void pinModeEx(word pin, byte mode);

class TExpanderChip {
  public:
    virtual void begin() = 0;
    virtual void end() = 0;
    virtual void digitalWrite(word pin, boolean value) = 0;
    virtual boolean digitalRead(word pin) = 0;
    virtual void pinMode(word pin, boolean direction) = 0;
};

typedef struct {
  word realPin;
  TExpanderChip *expander;
} TExpanderEntry;

extern TExpanderEntry *PortExpanders;


/* 74*595 family. Also works with exotic versions, e.g. TPIC6B595 */
#define PEC_74595_PIN_SRCLR_HW 0xFF
#define PEC_74595_PIN_G_HW 0xFF

class TEC_SR_74595: public TExpanderChip {
  public:
    TEC_SR_74595(byte pinSRCLR, byte pinG, byte pinRCK = SS, byte pinSER = MOSI,
                 byte pinSRCK = SCK, byte width = 1, boolean useSPI = true) {
      _useSPI = useSPI;
      _pinSRCLR = pinSRCLR;
      _pinG = pinG;
      _pinRCK = pinRCK;
      _pinSER = pinSER;
      _pinSRCK = pinSRCK;
      _width = width;
      _contents = (byte *)calloc(_width, sizeof(byte));
    };

    TEC_SR_74595(byte pinSRCLR, byte pinG, byte pinRCK = SS, byte width = 1) {
      TEC_SR_74595(pinSRCLR, pinG, pinRCK, MOSI, SCK, width, true);
    };

    void begin();
    void end();
    void digitalWrite(word pin, boolean value);
    //We don't do reads
    boolean digitalRead(word pin) { return false; };
    //We are not bidirectional
    void pinMode(word pin, boolean direction) { return; };

  private:
    byte *_contents;
    byte _width, _pinSER, _pinSRCK, _pinSRCLR, _pinRCK, _pinG;
    boolean _useSPI;
    
    void updateContents();
    void pushAndLatch();
};

/* 74*165 family */
class TEC_SR_74165: public TExpanderChip {
  public:
    TEC_SR_74165(byte pinSHLD = SS, byte pinQH = MISO, byte pinCLK = SCK,
                 byte width = 1, boolean useSPI = true) {
      _useSPI = useSPI;
      _pinSHLD = pinSHLD;
      _pinQH = pinQH;
      _pinCLK = pinCLK;
      _width = width;
      _contents = (byte *)calloc(_width, sizeof(byte));
    };

    TEC_SR_74165(byte pinSHLD = SS, byte width = 1) {
      TEC_SR_74165(pinSHLD, MISO, SCK, width, true);
    };

    void begin();
    void end();
    //We don't do writes
    void digitalWrite(word pin, boolean value) { return; };
    boolean digitalRead(word pin);
    //We are not bidirectional
    void pinMode(word pin, boolean direction) { return; };
    
  private:
    byte *_contents;
    byte _width, _pinCLK, _pinSHLD, _pinQH;
    boolean _useSPI;
    
    void sampleAndLatch();
    void fetchContents();
};

/* PCF8575C, PCF8575, PCF8574 */
#define PEC_PCF8575C_PIN_INT_HW 0xFF

class TEC_PE_PCF8575C: public TExpanderChip {
  public:
    /* NOTE: targetAddress is only the A[0..2] bits */
    TEC_PE_PCF8575C(byte pinINT, byte targetAddress, boolean wide = true) {
      _pinINT = pinINT;
      _i2caddr = ((B01000000 | targetAddress << 1) >> 1);
      _wide = wide;
      _dialAddress = false;
      _contents = (byte *)malloc((_wide ? 2 : 1) * sizeof(byte));
    };

    TEC_PE_PCF8575C(byte pinINT, byte pinA0, byte pinA1, byte pinA2,
                    byte targetAddress, boolean wide = true) {
      TEC_PE_PCF8575C(pinINT, targetAddress, wide);
      _pinA0 = pinA0;
      _pinA1 = pinA1;
      _pinA2 = pinA2;
      _dialAddress = true;
    };

    void begin();
    void end() { return; };
    void digitalWrite(word pin, boolean value);
    boolean digitalRead(word pin);
    void pinMode(word pin, boolean direction);

  private:
    byte *_contents;
    byte _i2caddr, _pinINT, _pinA0, _pinA1, _pinA2;
    boolean _wide, _dialAddress;

    void dialAddress();
    void fetchContents();
};

#endif
