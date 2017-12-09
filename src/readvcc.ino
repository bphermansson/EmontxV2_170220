#include <Arduino.h>

int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

/* Calibration
DWM=3.30V
Readvcc=3.242
internal1.1Ref = 1.1 * Vcc1 (per voltmeter) / Vcc2 (per readVcc() function)
1.1*3.300*3.242 = 11.76846
scale_constant = internal1.1Ref * 1023 * 1000
11.76846 * 1023 * 1000 = 12039134 ?
Use calibration value determined by trial and error instead.

With solar cell:
Vcc read 3286
DVM 3.27
*/
  int result = (high<<8) | low;
  result = 1147000L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  //Serial.print("batt: ");
  //Serial.println(result);
  return result; // Vcc in millivolts
}
