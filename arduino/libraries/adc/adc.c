/*ADC_REFRES
ADC Library 0x06

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#if defined (__AVR_ATtiny13A__)
#elif defined (__AVR_ATmega8__)
#elif defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__)
#elif defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
#elif defined (__AVR_ATmega2560__)
#else
	#error "no definitions available for this AVR"
#endif

#include "adc.h"

//complete this if trigger is on
//call adc_setchannel()
//call sei()
//ISR(ADC_vect) 
//{
//}

/*
 * set an adc channel
 */
void adc_setchannel(uint8_t channel)
{
	ADCSRA &= ~(1 << ADEN);
	#if defined (__AVR_ATtiny13A__)
	ADMUX &= 0b11111100; //clean channel
	channel = channel & 0b00000011;
	#elif defined (__AVR_ATmega8__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__)
	ADMUX &= 0b11111000; //clean channel
	channel = channel & 0b00000111;
	#elif defined (__AVR_ATmega2560__)
	if(channel < 8) {
		ADMUX &= 0b11111000; //clean channel
		channel = channel & 0b00000111;
		ADCSRB &= ~(1 << MUX5); //clean MUX5
	} else {
		ADMUX &= 0b11111000; //clean channel
		channel = (channel-8) & 0b00000111;
		ADCSRB |= (1 << MUX5); //set MUX5
	}
	#endif
	ADMUX |= channel; //set channel
	ADCSRA |= (1 << ADEN);
}

/*
 * read from selected adc channel
 */
uint16_t adc_readsel()
{
	ADCSRA |= (1 << ADSC); // Start conversion
	while( !(ADCSRA & (1<<ADIF)) ); // Wait for conversion to complete
	uint16_t adc = ADC;
	ADCSRA |= (1 << ADIF); // Clear ADIF by writing one to it
	return(adc);
}

/*
 * read from adc channel
 */
uint16_t adc_read(uint8_t channel)
{
	adc_setchannel(channel);
	return adc_readsel();
}

/*
 * init adc
 */
void adc_init() 
{
	// Init registers
	ADMUX = 0;
	ADCSRA = 0;
	#if defined (__AVR_ATtiny13A__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega2560__)
	ADCSRB = 0;
	#endif
/* ATMEGA 328p datasheet
0 0 AREF, Internal V ref turned off
0 1 AV CC with external capacitor at AREF pin
1 0 Reserved
1 1 Internal 1.1V Voltage Reference with external capacitor at AREF pin
*/
	// Set ADC reference
	#if defined (__AVR_ATtiny13A__)
		#if ADC_REF == 0
		ADMUX |= (0 << REFS0); // VCC used as analog reference
		#elif ADC_REF == 1
		ADMUX |= (1 << REFS0); // Internal Voltage Reference
		#endif

	#elif defined (__AVR_ATmega8__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega2560__)
		#if ADC_REF == 0
		  ADMUX |= (0 << REFS1) | (0 << REFS0); // AREF, Internal Vref turned off
		
		#elif ADC_REF == 1
		  ADMUX |= (0 << REFS1) | (1 << REFS0); // AVCC with external capacitor at AREF pin
		
		#elif ADC_REF == 2
	
		 #if defined (__AVR_ATmega2560__)
		  ADMUX |= (1 << REFS1) | (0 << REFS0); // Internal 1.1V Voltage Reference with external capacitor at AREF pin
      // ??? Atmega328p 1 1 = internal 1.1V Ref with Cap at AREF
		 #endif
		
		#elif ADC_REF == 3
		 ADMUX |= (1 << REFS1) | (1 << REFS0); // Internal 2.56V (or 1.1V atmeag328p) Voltage Reference with external cap at AREF
		#endif
	#endif
	
	// Set ADC prescaler
	#if ADC_PRESCALER == 2
	ADCSRA |= (0 << ADPS2) | (0 << ADPS1) | (1 << ADPS0); // Prescaler 2
	#elif ADC_PRESCALER == 4
	ADCSRA |= (0 << ADPS2) | (1 << ADPS1) | (0 << ADPS0); // Prescaler 4
	#elif ADC_PRESCALER == 8
	ADCSRA |= (0 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 8
	#elif ADC_PRESCALER == 16
	ADCSRA |= (1 << ADPS2) | (0 << ADPS1) | (0 << ADPS0); // Prescaler 16
	#elif ADC_PRESCALER == 32
	ADCSRA |= (1 << ADPS2) | (0 << ADPS1) | (1 << ADPS0); // Prescaler 32
	#elif ADC_PRESCALER == 64
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0); // Prescaler 64
	#elif ADC_PRESCALER == 128
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // Prescaler 128
	#endif
	
	// Set ADC justify
	#if ADC_JUSTIFY == 'L'
	ADMUX |= (1 << ADLAR); // Left adjust ADC result to allow easy 8 bit reading
	#elif ADC_JUSTIFY == 'R'
	ADMUX &= ~(1 << ADLAR); // Right adjust
	#endif
	
	// Set ADC trigger and mode
	#if ADC_TRIGGERON == 1
		#if defined (__AVR_ATtiny13A__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega2560__)
		ADCSRB |= (0 << ADTS2) | (0 << ADPS1) | (0 << ADPS0); // Free Running mode
		ADCSRA |= (1 << ADATE); // Enable ADC Interrupt
		#elif defined (__AVR_ATmega8__)
		ADCSRA |= (1 << ADFR); // Set ADC to Free-Running Mode
		#endif
		ADCSRA |= (1 << ADIE); // Enable ADC Interrupt 
	#endif
	
	// Enable ADC
	ADCSRA |= (1 << ADEN);

	#if	ADC_TRIGGERON == 1
	ADCSRA |= (1 << ADSC); // Start conversions
	#endif
}

/*
 * get reference voltage using bandgap voltage
 */
double adc_getrealvref()
{
	double intvoltage = 0;
	#if defined (__AVR_ATmega8__) || defined (__AVR_ATmega168__) || defined (__AVR_ATmega168P__) || defined (__AVR_ATmega328__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega2560__)
	uint8_t admuxbak = ADMUX; //store current admux
	//set bandgap voltage channel, and read value
	adc_setchannel(6);
	ADCSRA &= ~(1 << ADEN);
	ADMUX |= (1 << MUX3); //set MUX3
	#if defined (__AVR_ATmega2560__)
	ADMUX |= (1 << MUX4); //set MUX4
	#endif
	ADCSRA |= (1 << ADEN);
	_delay_us(250);
	uint16_t adc = adc_readsel();
	ADCSRA &= ~(1 << ADEN);
	ADMUX = admuxbak; //restore admux
	ADCSRA |= (1 << ADEN);
	//calculate internal voltage
	intvoltage = ((ADC_BANDGAPVOLTAGE * ADC_REFRES) / adc) / 1000.0;
	#endif
	return intvoltage;
}

/*
 * convert an adc value to a resistence value
 */
long adc_getresistence(uint16_t adcread, uint16_t adcbalanceresistor)
{
	if(adcread == 0)
		return 0;
	else
		return (long)((long)(ADC_REFRES*(long)adcbalanceresistor)/adcread-(long)adcbalanceresistor);
}

/*
 * convert an adc value to a voltage value
 */
double adc_getvoltage(uint16_t adcread, double adcvref) {
	if(adcread == 0)
		return 0;
	else
		return (double)(adcread*adcvref/(double)ADC_REFRES);
}

/*
 * exponential moving avarage filter
 *
 * "newvalue" new adc read value
 * "value" old adc filtered value
 * return a new filtered value
 *
 * References:
 *   Guillem Planissi: Measurement and filtering of temperatures with NTC
 */
#define ADC_EMAFILTERALPHA 30
unsigned int adc_emafilter(unsigned int newvalue, unsigned int value)
{
	//use exponential moving avarate Y=(1-alpha)*Y + alpha*Ynew, alpha between 1 and 0
	//in uM we use int math, so Y=(63-63alpha)*Y + 63alpha*Ynew  and  Y=Y/63 (Y=Y>>6)
	value = (64-ADC_EMAFILTERALPHA)*value+ADC_EMAFILTERALPHA*newvalue;
	value = (value>>6);
	return value;
}

