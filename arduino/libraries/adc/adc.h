/*
ADC Library 0x06

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef ADC_H
#define ADC_H

//setup values
#define ADC_REF 1 //3 no output //0 no output //2 no output //1 isOK //0 no output //adc reference voltage (see adc_init function for reference)
/*
 * 0 = AREF, Internal Vref turned off
 * 1 = ref is AVCC with external capacitor at AREF pin
 * 2 = reserved?, Internal 1.1V Voltage Reference with external capacitor at AREF pin
 * 3 = ref is Internal 2.56V Voltage Reference with external cap at AREF, or 
 *     internal 1.1V Ref with cap at AREF (Atmega 328)
 */
#define ADC_PRESCALER 16 //adc prescaler (2, 4, 8, 16, 32, 64, 128)
#define ADC_JUSTIFY 'R' //adc justify ('L' or 'R')
#define ADC_TRIGGERON 0 //adc trigger (1 on - 0 off)
#define ADC_REFRES 1024 //reference resolution used for conversions

//bandgap reference voltage * 1000, refers to datasheet
#if defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega2560__)
#define ADC_BANDGAPVOLTAGE 1100L
#elif defined (__AVR_ATmega8__)
#define ADC_BANDGAPVOLTAGE 1300L
#endif

#ifdef __cplusplus
 extern "C" {
#endif
//functions
  extern void adc_setchannel(uint8_t channel);
  extern uint16_t adc_read(uint8_t channel);
  extern uint16_t adc_readsel();
  extern void adc_init();
  extern double adc_getrealvref();
  extern long adc_getresistence(uint16_t adcread, uint16_t adcbalanceresistor);
  extern double adc_getvoltage(uint16_t adcread, double adcvref);
  extern unsigned int adc_emafilter(unsigned int newvalue, unsigned int value);
#ifdef __cplusplus
}
#endif

#endif
