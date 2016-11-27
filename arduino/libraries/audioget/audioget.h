/*
audiogetfft 0x04

copyright (c) Davide Gironi, 2013

Released under GPLv3.
Please refer to LICENSE file for licensing information.
*/


#ifndef AUDIOGET_H
#define AUDIOGET_H


#include "fftradix4.h"


// FFT setup ----------------
//define number of samples to take
#define AUDIOGET_SAMPLES 64
#define AUDIOGET_SAMPLESLOG2 6 //log2 AUDIOGET_SAMPLES
//real and imaginary part array for fft alghoritm
volatile int16_t audioget_fr[AUDIOGET_SAMPLES];
volatile int16_t audioget_fi[AUDIOGET_SAMPLES];


// TIME WEIGHT setup ----------------
//define time weight type
#define AUGIOGET_TWF 1 //time weight Fast
#define AUGIOGET_TWS 2 //time weight Slow

#define AUGIOGET_TW AUGIOGET_TWS  //CONFIG: use fast or slow time weight

//time-weight is a RC filter, exponential averaging is equivalent to a simple RC filter
//Y=(1-alpha)*Y + alpha*Ynew, alpha between 1 and 0
//given a sample rate Fs (Hz), number samples taken every seconds
//N as the number of samples grabbed in the input loop
//and a window size W (seconds), W/1000=Wms (milliseconds)
//the number of samples computed on every function call per window should be
//every function call, the window samples has a weight of 1/S
//S = W*Fs/N
//alpha can be computed as
//alpha = Wms/((Fs/fsc)+Wms)
//given fsc the number of loop taken to collect the input, we could use N here, but in real
//world, record N samples takes more than N cycles
#define AUGIOGET_TWFALPHA 0.652
#define AUGIOGET_TWSALPHA 0.956

// offset setup ----------------
//define and audio offset, could be set to completly suppress DC noise
#define AUGIOGET_RMSOFFSET -2

//define weighting gain table, must cointain AUDIOGET_SAMPLES/2 elements
//define A-Weighting (Real and Imaginary part)
#define AUDIOGET_WEIGHTINGTAR {0, 0.0363026, 0.513928541, 0.843129901, 1.023094514, 1.113238974, 1.150785316, 1.155376165, 1.13740991, 1.102630535, 1.05439208, 0.994794792, 0.925287976, 0.847016355, 0.76104051, 0.668494706, 0.570712944, 0.469336568, 0.366405293, 0.264424011, 0.16638812, 0.075740395, -0.003775804, -0.068404531, -0.114833072, -0.140808562, -0.145845064, -0.131862152, -0.103509898, -0.067907722, -0.033622556, -0.008963035}
#define AUDIOGET_WEIGHTINGTAI {0, 0.505227283, 0.673226268, 0.563062114, 0.390563392, 0.220544591, 0.066543122, -0.070793602, -0.193315087, -0.302824007, -0.40056018, -0.487184777, -0.562867671, -0.627380288, -0.680177014, -0.720469871, -0.747306718, -0.759664611, -0.756570026, -0.737256522, -0.701366932, -0.649199279, -0.581981015, -0.502132952, -0.413453151, -0.321118596, -0.231384828, -0.150886776, -0.085535271, -0.039169914, -0.012326333, -0.00159859}
#define AUDIOGET_WEIGHTINGTR AUDIOGET_WEIGHTINGTAR
#define AUDIOGET_WEIGHTINGTI AUDIOGET_WEIGHTINGTAI

// BIAS setup ----------------
//enable or disable dynamic bias
//with dynamic bias the zero raw bias is estimated using max and min raw value read
#define AUDIOGET_DYNAMICBIAS 1
//zero value, raw value read when there is no sound
#define AUDIOGET_BIASZERORAW 512


// INPUT setup ----------------
//adc input channel
#define AUDIOGET_INPUTCHANNEL 0
//sampler timer
//freq = FCPU/1+(prescaler*ICR1)
//ICR1 = (FCPU/freq - 1)/prescaler
//note: ICR1 can not be small, because we have some commands to execute during the interrupt routine
//es. 22050kHz @ 16Mhz, ICR1 91, prescaler 1
#define TIMER1_ICR1 91
#define TIMER1_PRESCALER (1<<CS11)
#define AUDIOGET_ADCSAMPLERATE 22050
#define AUDIOGET_ADCSAMPLES AUDIOGET_SAMPLES


// INPUT hardware setup ----------------
//define voltage reference and spl db reference
#define AUDIOGET_VOLTREF 0.03008496 //0.000126 //0.000315
#define AUDIOGET_DBREF 45 //38 //30
/* transfer factor, see http://www.sengpielaudio.com/calculator-transferfactor.htm
Referenz 94db/1VPa
Sensitivity = 20 × log (transfer factor in mV/Pa)
transfer factor in mV/Pa = 10^(sensivity/20), FORMEL: 
             value=Math.round(1000*Math.pow(10,(inp.value/20))*1*1000000)/1000000);
			 LINUX: echo "scale=4;1000*(e((-32/20)*l(10))*1*1000000)/1000000;" |bc -l
     NEW                        OLD
   94dB 1Pascal               74db 0.1 Pascal
-30dB -> 31.6228 mV/Pa      316.2278 mV/Pa
-32db -> 25.1189 mV/Pa      251.1886 mV/Pa
-38db -> 12.5893 mV/Pa		125.8925 mV/Pa
-40db -> 10.0000 mV/Pa      100.0000 mV/Pa

315 mV -> -10db  			 315mV -> -30db 
*/

#ifdef __cplusplus
 extern "C" {
#endif
//functions
  extern void audioget_init();
  extern void audioget_getsamples();
  extern void audioget_doweighting();
  extern void audioget_computefft();
  extern uint16_t audioget_getfsc();
  extern void audioget_getmagnitude(double spectrum[]);
  extern int16_t audioget_getrmsval();
  extern int16_t audioget_getspl(float voltnow, float voltref, float dbref);
#ifdef __cplusplus
}
#endif

#endif
