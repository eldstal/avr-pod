#include "podcontrols.h"
#include <avr/io.h>

// Global data passing buffers.
// Reports are set in this module and read elsewhere
// LED data is set elsewhere and acted upon in this module
report_t reportBuffer;
led_t    LEDBuffer;

// Private functions
void prepareADC(int stage);
unsigned int isADCReady();
unsigned char getADC();


void initPodControls() {

  /*
   * ADC setup
   */
  // Disable ADC power saving
  PRR = PRR & ~(1 << PRADC);

  // Enable digital input on PC0:PC2
  // Disable digital input on the ADC pin PC4
  DIDR0 |= (1<<ADC4D);
  DIDR0 &= ~((1<<ADC0D) | (1<<ADC1D) | (1<<ADC2D));

  // Select the appropriate ADC channel (lower nibble of ADMUX)
  ADMUX = (ADMUX & 0xF0) | 0x4;

  // 16MHz / 32 = 500 kHz
  ADCSRA = (ADCSRA & ~0x07) | 0x05;

  // Use the external AVCC voltage as a reference
  ADMUX |= (1 << REFS0);

  // ADC enabled
  ADCSRA |= 1<<ADEN;

  // No auto-trigger of the ADC
  ADCSRA &= ~(1<<ADATE);

  // Left-adjusted 8-bit precision ADC
  ADMUX |= 1<<ADLAR;

  prepareADC(0);


  /*
   * GPIO Setup
   */
  // PortB pins 0, 1 and 2 are used as anodes to LED banks and the switch bank
  DDRB |= (1<<DDB0) | (1<<DDB1) | (1<<DDB2);

  // PortB pins 3, 4 and 5 are used as cathodes for the LEDs
  DDRB |= (1<<DDB3) | (1<<DDB4) | (1<<DDB5);

  // PORTC pins 1, 2, 3 are used to control the potentiometer mux
  DDRC |= (1<<DDC0) | (1<<DDC1) | (1<<DDC2);

  // Port D pins 5, 6, 7 are used as inputs for the switch bank
  DDRD &= ~((1<<DDD5) | (1<<DDD6) | (1<<DDD7));

}

inline void updateSensorData() {
  // We need to fetch data from ADCs etc. in stages.
  // Stages 0-2 are for pot bank 1
  // Stages 3-6 are for pot bank 2
  // Stage  7 is for switches
  static int stage = 0;

  // Button 0 shows we're alive still
  switch (stage) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      if (isADCReady()) {
        reportBuffer.ax[stage] = getADC();

        // Next time, move on to another stage!
        ++stage;
      }
      break;

    case 7:
    default:

      // The switches are connected to the same voltage feed as the
      // bank 0 LEDs, so we cannot fiddle with those registers here.
      // switches are read in updateLEDState instead.
      stage = 0;
      break;
  }

  // While some other stuff is running, we can have the ADC perform its conversion
  prepareADC(stage);
}

void showInputOnLEDs() {
  // Test the pot/switch states without USB
  LEDBuffer.indicators = 0x00;
  for (int c = 0; c < 7; ++c) {
    int ledstate = reportBuffer.ax[c] > 128;
    LEDBuffer.indicators |= ledstate << c;
  }

  LEDBuffer.switches = 0x00;
  for (int c = 0; c<3; ++c) {
    int switchstate = (reportBuffer.switches & (0x1 << c)) != 0;
    LEDBuffer.switches |= switchstate << c;
  }
}

void updateLEDState() {

  showInputOnLEDs();
  static int stage = 0;

  // Turn off all three LEDs in the currently active bank
  PORTB |= 0x7 << 3;

  // There are 3 stages (LED banks), controlled
  // by the anode pins PB0:PB2. Enable the right anode!
  PORTB &= ~0x07;
  PORTB |= 0x1 << stage;

  unsigned char values = 0x0;
  switch (stage) {
    case 0:
      values = LEDBuffer.switches & 0x7;
      // Enabling this bank of LEDs also allows us to
      // read the toggle switches. Let's do that.
      reportBuffer.switches = (PIND >> 5) & 0x7;
      break;
    case 1:
      values = LEDBuffer.indicators & 0x7;
      break;
    case 2:
      values = (LEDBuffer.indicators >> 3) & 0x7;
      break;
  }

  // In each bank, three LEDs can be controlled by
  // pulling the corresponding cathode (PB3:PB5) low
  PORTB &= ~(values << 3);

  // Move forward to the next stage
  if (stage == 2) stage = 0;
  else            ++stage;

}

void prepareADC(int stage) {
  // Stages 0-6 are ADC stages, anything else is ignored
  if (stage > 6) return;

  // Set the external ADC mux to the appropriate channel
  PORTC = (PORTC & ~0x7) | (stage & 0x7);
  //PORTC &= ~0x7;

  // Wait for the mux to settle
  unsigned char i=255;
  while (--i);

  // Start a conversion
  ADCSRA |= 1<<ADSC;
}

inline unsigned int isADCReady() {
  return ADCSRA & (1 << ADIF);
}

inline unsigned char getADC() {

  // Clear the conversion flag
  ADCSRA |= (1 << ADIF);

  // 10 bits of data is left-adjusted over two 8-bit registers
  // We discard the two least significant bits.
  return ADCH;
}
