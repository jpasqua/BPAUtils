/*
 * Indicators
 *    Indicators are a way of communicating to users through (potentially)
 *    multi-color LEDs. Particular Indicator implementations may use
 *    the LED built onto a microcontroller board, or perhaps external
 *    NeoPixels.
 *
 * NOTE: In the future we may wish to add functionality like pulsing, fading,
 *       or blinking so clients don't need to do that on their own.
 *
 */

#ifndef Indicators_h
#define Indicators_h

#include <ArduinoLog.h>

//--------------- Begin:  Includes ---------------------------------------------
//                                  Core Libraries
//                                  Third Party Libraries
#if defined(ESP32)
  #include <analogWrite.h>
  #define PWMRANGE 255
#endif
#include <Adafruit_NeoPixel.h>
//                                  Local Includes
//--------------- End:    Includes ---------------------------------------------


/*------------------------------------------------------------------------------
 *
 * The base Indicator class with an empty implementation. Use instances of this
 * class if you have no indicator, but still need to provide an Indicator to
 * some system.
 *
 *----------------------------------------------------------------------------*/

class Indicator {
public:
  virtual void setColor(uint32_t c) { (void)c;  }
  virtual void setColor(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b;  }
  virtual void on() {  }
  virtual void off() {  }
};

/*------------------------------------------------------------------------------
 *
 * An on/off LED indicator
 *
 *----------------------------------------------------------------------------*/

class OnOffIndicator : public Indicator {
public:
  void begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, OUTPUT);
  }

  virtual void setColor(uint32_t c) {
    digitalWrite(_pin, (c != 0));
  }
  
  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    digitalWrite(_pin, ((r != 0)&&(g != 0)&&(b !=0)));
  }

  void on() { digitalWrite(_pin, HIGH); }
  void off() { digitalWrite(_pin, LOW); }

private:
  uint8_t _pin;
};


/*------------------------------------------------------------------------------
 *
 * An LED indicator that can display different intensities
 *
 *----------------------------------------------------------------------------*/

class IntensityIndicator : public Indicator {
public:
  void begin(uint8_t pin) {
    _pin = pin;
  }

  virtual void setColor(uint32_t c) {
    setColor((c&0xff0000)>>16, (c&0x00ff00)>>8, (c&0x0000ff));
  }
  
  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t val = r > g ? r : b;
    val = val > b ? val : b;
    analogWrite(_pin, val);
  }

  void on() { analogWrite(_pin, 255); }
  void off() { analogWrite(_pin, 0); }

private:
  uint8_t _pin;
};


/*------------------------------------------------------------------------------
 *
 * If you have one or more NeoPixel Indicators, create a NeoPixelIndicators
 * instance that does all of the heavy lifting for them.
 *
 *----------------------------------------------------------------------------*/

class NeoPixelIndicators {
public:
  Adafruit_NeoPixel *leds;

  NeoPixelIndicators(uint8_t pin, uint8_t count, neoPixelType type=(NEO_GRB + NEO_KHZ800)) {
    _pin = pin;
    _count = count;
    _type = type;
  }

  void begin() {
    // leds = new Adafruit_NeoPixel(_count, _pin, NEO_RGB + NEO_KHZ800);
    leds = new Adafruit_NeoPixel(_count, _pin, _type);
    leds->begin();
    leds->clear();
    leds->show();
  }

  void setBrightness(uint8_t b) {
    leds->setBrightness(b);
    leds->show();
  }

private:
  uint8_t _pin = 0;
  uint8_t _count = 0;
  neoPixelType _type = NEO_GRB + NEO_KHZ800;
};


/*------------------------------------------------------------------------------
 *
 * A NeoPixel Indicator whose parent is an instance of NeoPixelIndicators 
 *
 *----------------------------------------------------------------------------*/

class NeoPixelIndicator : public Indicator {
public:
  void begin(NeoPixelIndicators* parent, uint8_t which) {
    _parent = parent;
    _which = which;
  }

  virtual void setColor(uint32_t c) {
    _parent->leds->setPixelColor(_which, c);
    _parent->leds->show();
  }
  
  void setColor(uint8_t r, uint8_t g, uint8_t b) {
    _parent->leds->setPixelColor(_which, r, g, b);
    _parent->leds->show();
  }

  void on() { setColor(0xFFFFFF); }
  void off() { setColor(0x000000); }

private:
  NeoPixelIndicators* _parent;
  uint8_t _which;
};


#endif  // Indicators_h