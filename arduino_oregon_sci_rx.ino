// Taken from https://jeelabs.net/projects/cafe/wiki/Decoding_the_Oregon_Scientific_V2_protocol
// Oregon V2 decoder added - Dominique Pierre
// Oregon V3 decoder revisited - Dominique Pierre
// New code to decode OOK signals from weather sensors, etc.
// 2010-04-11 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id: ookDecoder.pde 5331 2010-04-17 10:45:17Z jcw $
//
// BK modified for Arduino Pro Mini 2/12/2019
// - Our 433 MHz detector produces a digital output.  Use Pin D2 (INT0) as interrupt input.
// - Blink Green LED on Port D13 on each decoded packet
//
// The PCR800 rain gauge produces these 2 lines every ~45 seconds:
// OSV3 2A190453305901192060
// HEZ 01800120850700

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED             13      // Port 13 - standard
#define LED_PULSE_LEN   20000   // in units of loop() iterations

class DecodeOOK {
  protected:
    byte total_bits, bits, flip, state, pos, data[25];

    virtual char decode (word width) = 0;

  public:

    enum { UNKNOWN, T0, T1, T2, T3, OK, DONE };

    DecodeOOK () {
      resetDecoder();
    }

    bool nextPulse (word width) {
      if (state != DONE)

        switch (decode(width)) {
          case -1: resetDecoder(); break;
          case 1:  done(); break;
        }
      return isDone();
    }

    bool isDone () const {
      return state == DONE;
    }

    const byte* getData (byte& count) const {
      count = pos;
      return data;
    }

    void resetDecoder () {
      total_bits = bits = pos = flip = 0;
      state = UNKNOWN;
    }

    // add one bit to the packet data buffer

    virtual void gotBit (char value) {
      total_bits++;
      byte *ptr = data + pos;
      *ptr = (*ptr >> 1) | (value << 7);

      if (++bits >= 8) {
        bits = 0;
        if (++pos >= sizeof data) {
          resetDecoder();
          return;
        }
      }
      state = OK;
    }

    // store a bit using Manchester encoding
    void manchester (char value) {
      flip ^= value; // manchester code, long pulse flips the bit
      gotBit(flip);
    }

    // move bits to the front so that all the bits are aligned to the end
    void alignTail (byte max = 0) {
      // align bits
      if (bits != 0) {
        data[pos] >>= 8 - bits;
        for (byte i = 0; i < pos; ++i)
          data[i] = (data[i] >> bits) | (data[i + 1] << (8 - bits));
        bits = 0;
      }
      // optionally shift bytes down if there are too many of 'em
      if (max > 0 && pos > max) {
        byte n = pos - max;
        pos = max;
        for (byte i = 0; i < pos; ++i)
          data[i] = data[i + n];
      }
    }

    void reverseBits () {
      for (byte i = 0; i < pos; ++i) {
        byte b = data[i];
        for (byte j = 0; j < 8; ++j) {
          data[i] = (data[i] << 1) | (b & 1);
          b >>= 1;
        }
      }
    }

    void reverseNibbles () {
      for (byte i = 0; i < pos; ++i)
        data[i] = (data[i] << 4) | (data[i] >> 4);
    }

    void done () {
      while (bits)
        gotBit(0); // padding
      state = DONE;
    }
};

// 433 MHz decoders


class OregonDecoderV2 : public DecodeOOK {
  public:
    OregonDecoderV2() {}

    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
      if (!(total_bits & 0x01))
      {
        data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
      }
      total_bits++;
      pos = total_bits >> 4;
      if (pos >= sizeof data) {
        resetDecoder();
        return;
      }
      state = OK;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
            if (w != 0) {
              // Long pulse
              ++flip;
            } else if (32 <= flip) {
              // Short pulse, start bit
              flip = 0;
              state = T0;
            } else {
              // Reset decoder
              return -1;
            }
            break;
          case OK:
            if (w == 0) {
              // Short pulse
              state = T0;
            } else {
              // Long pulse
              manchester(1);
            }
            break;
          case T0:
            if (w == 0) {
              // Second short pulse
              manchester(0);
            } else {
              // Reset decoder
              return -1;
            }
            break;
        }
      } else {
        return -1;
      }
      return total_bits == 160 ? 1 : 0;
    }
};

class OregonDecoderV3 : public DecodeOOK {
  public:
    OregonDecoderV3() {}

    // add one bit to the packet data buffer
    virtual void gotBit (char value) {
      data[pos] = (data[pos] >> 1) | (value ? 0x80 : 00);
      total_bits++;
      pos = total_bits >> 3;
      if (pos >= sizeof data) {
        resetDecoder();
        return;
      }
      state = OK;
    }

    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
            if (w == 0)
              ++flip;
            else if (32 <= flip) {
              flip = 1;
              manchester(1);
            } else
              return -1;
            break;
          case OK:
            if (w == 0)
              state = T0;
            else
              manchester(1);
            break;
          case T0:
            if (w == 0)
              manchester(0);
            else
              return -1;
            break;
        }
      } else {
        return -1;
      }
      return  total_bits == 80 ? 1 : 0;
    }
};

class CrestaDecoder : public DecodeOOK {
  public:
    CrestaDecoder () {}

    virtual char decode (word width) {
      if (200 <= width && width < 1300) {
        byte w = width >= 750;
        switch (state) {
          case UNKNOWN:
            if (w == 1)
              ++flip;
            else if (2 <= flip && flip <= 10)
              state = T0;
            else
              return -1;
            break;
          case OK:
            if (w == 0)
              state = T0;
            else
              gotBit(1);
            break;
          case T0:
            if (w == 0)
              gotBit(0);
            else
              return -1;
            break;
        }
      } else if (width >= 2500 && pos >= 7)
        return 1;
      else
        return -1;
      return 0;
    }
};

class KakuDecoder : public DecodeOOK {
  public:
    KakuDecoder () {}

    virtual char decode (word width) {
      if (180 <= width && width < 450 || 950 <= width && width < 1250) {
        byte w = width >= 700;
        switch (state) {
          case UNKNOWN:
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            if (w)
              state = T1;
            else
              return -1;
            break;
          case T1:
            state += w + 1;
            break;
          case T2:
            if (w)
              gotBit(0);
            else
              return -1;
            break;
          case T3:
            if (w == 0)
              gotBit(1);
            else
              return -1;
            break;
        }
      } else if (width >= 2500 && 8 * pos + bits == 12) {
        for (byte i = 0; i < 4; ++i)
          gotBit(0);
        alignTail(2);
        return 1;
      } else
        return -1;
      return 0;
    }
};

class XrfDecoder : public DecodeOOK {
  public:
    XrfDecoder () {}

    // see also http://davehouston.net/rf.htm
    virtual char decode (word width) {
      if (width > 2000 && pos >= 4)
        return 1;
      if (width > 5000)
        return -1;
      if (width > 4000 && state == UNKNOWN)
        state = OK;
      else if (350 <= width && width < 1800) {
        byte w = width >= 720;
        switch (state) {
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            gotBit(w);
            break;
        }
      } else
        return -1;
      return 0;
    }
};

class HezDecoder : public DecodeOOK {
  public:
    HezDecoder () {}

    // see also http://homeeasyhacking.wikia.com/wiki/Home_Easy_Hacking_Wiki
    virtual char decode (word width) {
      if (200 <= width && width < 1200) {
        byte w = width >= 600;
        gotBit(w);
      } else if (width >= 5000 && pos >= 5 /*&& 8 * pos + bits == 50*/) {
        for (byte i = 0; i < 6; ++i)
          gotBit(0);
        alignTail(7); // keep last 56 bits
        return 1;
      } else
        return -1;
      return 0;
    }
};

// 868 MHz decoders

class VisonicDecoder : public DecodeOOK {
  public:
    VisonicDecoder () {}

    virtual char decode (word width) {
      if (200 <= width && width < 1000) {
        byte w = width >= 600;
        switch (state) {
          case UNKNOWN:
          case OK:
            state = w == 0 ? T0 : T1;
            break;
          case T0:
            gotBit(!w);
            if (w)
              return 0;
            break;
          case T1:
            gotBit(!w);
            if (!w)
              return 0;
            break;
        }
        // sync error, flip all the preceding bits to resync
        for (byte i = 0; i <= pos; ++i)
          data[i] ^= 0xFF;
      } else if (width >= 2500 && 8 * pos + bits >= 36 && state == OK) {
        for (byte i = 0; i < 4; ++i)
          gotBit(0);
        alignTail(5); // keep last 40 bits
        // only report valid packets
        byte b = data[0] ^ data[1] ^ data[2] ^ data[3] ^ data[4];
        if ((b & 0xF) == (b >> 4))
          return 1;
      } else
        return -1;
      return 0;
    }
};

class EMxDecoder : public DecodeOOK {
  public:
    EMxDecoder () {}

    // see also http://fhz4linux.info/tiki-index.php?page=EM+Protocol
    virtual char decode (word width) {
      if (200 <= width && width < 1000) {
        byte w = width >= 600;
        switch (state) {
          case UNKNOWN:
            if (w == 0)
              ++flip;
            else if (flip > 20)
              state = OK;
            else
              return -1;
            break;
          case OK:
            if (w == 0)
              state = T0;
            else
              return -1;
            break;
          case T0:
            gotBit(w);
            break;
        }
      } else if (width >= 1500 && pos >= 9)
        return 1;
      else
        return -1;
      return 0;
    }
};

class KSxDecoder : public DecodeOOK {
  public:
    KSxDecoder () {}

    // see also http://www.dc3yc.homepage.t-online.de/protocol.htm
    virtual char decode (word width) {
      if (200 <= width && width < 1000) {
        byte w = width >= 600;
        switch (state) {
          case UNKNOWN:
            gotBit(w);
            bits = pos = 0;
            if (data[0] != 0x95)
              state = UNKNOWN;
            break;
          case OK:
            state = w == 0 ? T0 : T1;
            break;
          case T0:
            gotBit(1);
            if (!w)
              return -1;
            break;
          case T1:
            gotBit(0);
            if (w)
              return -1;
            break;
        }
      } else if (width >= 1500 && pos >= 6)
        return 1;
      else
        return -1;
      return 0;
    }
};

class FSxDecoder : public DecodeOOK {
  public:
    FSxDecoder () {}

    // see also http://fhz4linux.info/tiki-index.php?page=FS20%20Protocol
    virtual char decode (word width) {
      if (300 <= width && width < 775) {
        byte w = width >= 500;
        switch (state) {
          case UNKNOWN:
            if (w == 0)
              ++flip;
            else if (flip > 20)
              state = T1;
            else
              return -1;
            break;
          case OK:
            state = w == 0 ? T0 : T1;
            break;
          case T0:
            gotBit(0);
            if (w)
              return -1;
            break;
          case T1:
            gotBit(1);
            if (!w)
              return -1;
            break;
        }
      } else if (width >= 1500 && pos >= 5)
        return 1;
      else
        return -1;
      return 0;
    }
};

OregonDecoderV2 orscV2;
OregonDecoderV3 orscV3;
CrestaDecoder cres;
KakuDecoder kaku;
XrfDecoder xrf;
HezDecoder hez;
VisonicDecoder viso;
EMxDecoder emx;
KSxDecoder ksx;
FSxDecoder fsx;

volatile word pulse;
volatile uint16_t led_timer;

ISR(INT0_vect) {
  static word last;
  // determine the pulse length in microseconds, for either polarity
  pulse = micros() - last;
  last += pulse;
}

void reportSerial (const char* s, class DecodeOOK& decoder) {
  byte pos;
  const byte* data = decoder.getData(pos);
  Serial.print(s);
  Serial.print(' ');
  for (byte i = 0; i < pos; ++i) {
    Serial.print(data[i] >> 4, HEX);
    Serial.print(data[i] & 0x0F, HEX);
  }

  // Serial.print(' ');
  // Serial.print(millis() / 1000);
  Serial.println();

  decoder.resetDecoder();
  led_timer = LED_PULSE_LEN;
}

////////////////////////////////
// From https://forum.arduino.cc/index.php?action=dlattach;topic=203425.0;attach=63522
// Get a nybble from manchester bytes, short name so equations elsewhere are neater :-)
double nyb(int nybble, const byte *data){
  int bite = nybble / 2;  //DIV 2, find the byte
  //int nybb  = nybble % 2;  //MOD 2  0=MSB 1=LSB
  int nybb  = (nybble+1) % 2;  //MOD 2  1=MSB 0=LSB  - bk swap nibbles
  byte b = data[bite];
  if (nybb == 0){
    b = (byte)((byte)(b) >> 4);
  }
  else{
    b = (byte)((byte)(b) & (byte)(0xf));
  }      
  return (double)b;
}

// Decode the packet from a PCR800 rain guage
void rain(class DecodeOOK& decoder){
  byte pos;
  const byte* data = decoder.getData(pos);
  double rainTotal = ((nyb(18, data)*100000)+(nyb(17, data)*10000)+(nyb(16, data)*1000)+(nyb(15, data)*100)+(nyb(14, data)*10)+nyb(13, data))/1000;
  Serial.print("RAIN ");
  Serial.print(rainTotal);    // inches
  Serial.print(" ");
  float rainRate = ((nyb(12, data)*100000)+(nyb(11, data)*10000)+(nyb(10, data)*1000)+(nyb(9, data)*100)+(nyb(8, data)*10)+nyb(7, data))/10000;
  Serial.println(rainRate);   // inches / hour
}
////////////////////////////////

// LED is active high
void blink_led()
{
  int i;
  for(i=0; i<5; i++) {
    pinMode(LED, OUTPUT);  
    digitalWrite(LED, HIGH);  
    delay(100);
    pinMode(LED, OUTPUT);  
    digitalWrite(LED, LOW);  
    delay(100);
  }
}

void setup () {
  Serial.begin(115200);
  Serial.println("\n[ookDecoder]");
  Serial.println("[Ver 1.0.0-BK]");

  // LED blink at start-up
  blink_led();

  // Set up the data pin as an interrupt source
  DDRD &= ~(1<<PD2);        // Set PD2 as input (Using for interupt INT0)
  //PORTD = 1<<PD2;           // Enable PD2 pull-up resistor  - don't do this
  PORTD &= ~(1<<PD2);       // Disable PD2 pull-up resistor
  EICRA |= (1 << ISC00);    // set INT0 to trigger on ANY logic change
  EIMSK |= (1 << INT0);     // Turns on INT0
  //sei();                    // turn on interrupts
}

void loop () {
  static int i = 0;
  cli();
  word p = pulse;

  pulse = 0;
  sei();

  // Turn off LED after some period of time
  if(led_timer) {
    digitalWrite(LED, HIGH); 
    led_timer--;
  } 
  else {
    digitalWrite(LED, LOW);  
  }

  //if (p != 0){ Serial.print(++i); Serial.print('\n');}

  if (p != 0) {
    if (orscV2.nextPulse(p))
      reportSerial("OSV2", orscV2);
    if (orscV3.nextPulse(p)) {
      reportSerial("OSV3", orscV3);
      rain(orscV3);                 // hack - assume it's a rain guage
    }
    if (cres.nextPulse(p))
      reportSerial("CRES", cres);
    if (kaku.nextPulse(p))
      reportSerial("KAKU", kaku);
    if (xrf.nextPulse(p))
      reportSerial("XRF", xrf);
    if (hez.nextPulse(p))
      reportSerial("HEZ", hez);
  }

  if (p != 0) {
    if (viso.nextPulse(p))
      reportSerial("VISO", viso);
    if (emx.nextPulse(p))
      reportSerial("EMX", emx);
    if (ksx.nextPulse(p))
      reportSerial("KSX", ksx);
    if (fsx.nextPulse(p))
      reportSerial("FSX", fsx);
  }
}

