/*
audiogetfft 0x04

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h> //include libm

#include "audioget.h"

#include "adc.h"


extern volatile int16_t audioget_fr[AUDIOGET_SAMPLES];
extern volatile int16_t audioget_fi[AUDIOGET_SAMPLES];


volatile uint8_t audioget_takesample = 0;
static uint16_t audioget_fsc = 0;
/*
 * timer1 interrupt
 */
ISR(TIMER1_OVF_vect) {
	audioget_fsc++;
	audioget_takesample = 1;
}


/*
 * init ports
 */
void audioget_init() {
	//init adc
	//ADC speed (F_CPU/prescaler)/13 (adc takes 13 clock to perform set max for not error = 1Mhz)
	//so a maximum ADC clock of 1MHz means approximately 77k samples per second
	adc_setchannel(AUDIOGET_INPUTCHANNEL);
	adc_init();

	//init timer
	ICR1 = TIMER1_ICR1; //ICR1
	TCCR1A = (1 << WGM11); //fast pwm mode
	TCCR1B = (1 << WGM13) | (1 << WGM12);
	TCCR1B |= TIMER1_PRESCALER; //prescaler
#ifdef TIMSK
	TIMSK |= (1<<TOIE1); //enable timer1
#else
  TIMSK1 |= (1<<TOIE1); //enable timer1
#endif  
	//init fft variables
	uint16_t i = 0;
	for(i=0; i<AUDIOGET_SAMPLES; i++) {
		audioget_fr[i] = 0;
		audioget_fi[i] = 0;
	}

	//enable interrupt
	sei();
}


/*
 * load samples from adc pin
 * signal must be biased to ADVREF voltage / 2
 */
void audioget_getsamples() {
	int16_t biasadc = 0;
	int16_t biasadcmin = 1024;
	int16_t biasadcmax = 0;

	int16_t adc_raw = 0;

	audioget_fsc = 0;

	//set channel
	adc_setchannel(AUDIOGET_INPUTCHANNEL);

	uint8_t i = 0;
	audioget_takesample = 0;
	for(i=0; i<AUDIOGET_SAMPLES; i++) {
		//wait for timer pulse
		while(!audioget_takesample) {};
		audioget_takesample = 0;

		//get sample
		adc_raw = adc_readsel();

		//dynamic bias computation
		#if AUDIOGET_DYNAMICBIAS == 1
		if(adc_raw < biasadcmin)
			biasadcmin = adc_raw;
		if(adc_raw > biasadcmax)
			biasadcmax = adc_raw;
		biasadc = (biasadcmin + (biasadcmax-biasadcmin)/2);
		adc_raw = adc_raw - biasadc;
		#endif

		//audioget_fr holds the real part of the signal
		audioget_fr[i] = adc_raw;
		//audioget_fi holds the complex part of the signal
		audioget_fi[i] = 0; //always 0
	}
}


/*
 * compute fft
 */
void audioget_computefft() {
	rev_bin((int *)audioget_fr, AUDIOGET_SAMPLES);//reverse f_r
	//f_i could not being reversed, cause it's composed by zero only
	fft_radix4_I((int *)audioget_fr, (int *)audioget_fi, AUDIOGET_SAMPLESLOG2); //do fft computation using radix4
}


/*
 * compute weighting
 */
void audioget_doweighting() {
	uint8_t i = 0;

	double weightingtabr[] = AUDIOGET_WEIGHTINGTR;
	double weightingtabi[] = AUDIOGET_WEIGHTINGTI;
	
	//apply filter function
	for (i=0; i<AUDIOGET_SAMPLES/2; i++) {
		double weighingfactor = weightingtabr[i];
		audioget_fr[i] *= weighingfactor;
		audioget_fr[AUDIOGET_SAMPLES - (i + 1)] *= weighingfactor;
		weighingfactor = weightingtabi[i];
		audioget_fi[i] *= weighingfactor;
		audioget_fi[AUDIOGET_SAMPLES - (i + 1)] *= weighingfactor;
	}
}


/*
 * get the signal magnitude
 */
void audioget_getmagnitude(double spectrum[]) {
	uint8_t i = 0;

	//compute magnitude from 0 to AUDIOGET_SAMPLES/2
	//since we want it normalized, we divided it by number of samples
	//since we dropped half the FFT, we multiply it 2 to keep the same energy
	for (i=0; i<=AUDIOGET_SAMPLES/2; i++) {
		spectrum[i] = 2*sqrt(((long)audioget_fr[i]*(long)audioget_fr[i] + (long)audioget_fi[i]*(long)audioget_fi[i]) >> (AUDIOGET_SAMPLESLOG2*2));
	}
}


/*
 * get the running frequency to compute time window strategies
 */
uint16_t audioget_getfsc() {
	return audioget_fsc;
}


/*
 * get rms value from audio signal
 * signal must be biased to ADVREF voltage / 2
 */
int16_t audioget_getrmsval() {
	uint8_t i = 0;
	int16_t getval = 0;
	static double retval = 0;

	//calculate rms for DFT with real input
	//use parseval theorem to transform rms on frequency domain to rms on time domain, on real DTFT interval
	uint16_t magnitud = 0;
	for (i=0; i<=AUDIOGET_SAMPLES/2; i++) {
		magnitud += ((long)audioget_fr[i]*(long)audioget_fr[i] + (long)audioget_fi[i]*(long)audioget_fi[i]) >> (AUDIOGET_SAMPLESLOG2*2);
	}
	magnitud = magnitud*2;
	getval = sqrt(magnitud);

	//appy rms offset
	getval += AUGIOGET_RMSOFFSET;

	//time-weight filter
	#if AUGIOGET_TW == AUGIOGET_TWF
		retval = AUGIOGET_TWFALPHA*retval+(1-AUGIOGET_TWFALPHA)*getval;
	#elif AUGIOGET_TW == AUGIOGET_TWS
		retval = AUGIOGET_TWSALPHA*retval+(1-AUGIOGET_TWSALPHA)*getval;
	#endif

	if(retval<0)
		retval = 0;

	return (int16_t)retval;
}

/*
 * get spl value,
 * depending on actual voltage read, voltage at a db reference, the db reference
 */
int16_t audioget_getspl(float voltnow, float voltref, float dbref) {
	int16_t ret = (20 * log10(voltnow/voltref)) +  dbref;
	return ret;
}

