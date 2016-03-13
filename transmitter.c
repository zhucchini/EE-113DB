#include "L138_LCDK_aic3106_init.h"
#include "evmomapl138_gpio.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define SAMP_FREQ 	48000
#define PI 			3.141592654
#define N			5000
#define S			4
#define HIGH		12000
#define LOW			0
#define K			4001 //1+(HIGH-LOW)/3
#define CBUFF_LEN	30
#define TRUE		1
#define FALSE 		0


float symbols[] = {220.0, 261.63, 329.63, 392.00};

float left_sample = 0;

float phi_t = 0.0;
float amplitude = 20000;
int curr_symbol = 0;
float curr_freq = LOW;
int counter = 0;

double response[K] = {0};
int response_index = 0;

float buffer[N] = {0};
int buffer_index = 0;
int ran = FALSE;

int cycle_count = 0;
int symbol_index = 0;


// PLL stuff ------------------------------------------------------

float alpha = 0.01; //loop filter parameter
float beta = 0.002; //loop filter parameter
float Fmsg = 6000; //vco rest frequency (carrier frequency)
float x[CBUFF_LEN] = {0}; //input signal
int   x_index = 0;
float B[CBUFF_LEN+1] = {-0.0042, 0.0000, -0.0093, 0.0000, -0.0188, 0.0000, -0.0344, 0.0000,
					   -0.0596, 0.0000, -0.1030, 0.0000, -0.1968, 0.0000, -0.6314, 0.0000,
					    0.6314, 0.0000,  0.1968, 0.0000,  0.1030, 0.0000,  0.0596, 0.0000,
					    0.0344, 0.0000,  0.0188, 0.0000,  0.0093, 0.0000,  0.0042};
                    //remez(30, [0.1,0.9],[1,1], 'Hilbert') in matlab

float sReal; //real part of analytical signal (after hilbert)
float sImag; //imag part of analytical signal
float q = 0; //input to loop filter
float sigma = 0; // part of loop filter output
float loopFilterOutput = 0;
float phi = 0; //phase accumulator value
float pi = 3.14159265358979;
float phaseDetectorOutputReal = 0;
float phaseDetectorOutputImag = 0;
float vcoOutputReal = 1;
float vcoOutputImag = 0;
float scaleFactor = 3.0517578125e-5;
float output_freq = 200;
// ---------------------------------------------------------------------


inline void increase_freq(void) {
	curr_freq += 3.0;
	response_index++;
}

interrupt void interrupt4(void){
	//float left_sample = input_left_sample();

	//Frequency Response stuff

//	if(response_index < K) {
//		response[response_index] += fabsf(left_sample)/N;
//	}

//	if(buffer_index < N) {
//		buffer[buffer_index] = left_sample;
//		buffer_index++;
//	}

	phi_t += ((6000.0+symbols[curr_symbol])/(float)SAMP_FREQ)*2.0*PI;

//	if(counter == N-1) {
//		curr_symbol = (curr_symbol+1)%S;
//	}

	// PLL Implementation ------------------------------------------------
	//PLL Routine
	x[x_index] = left_sample;
	sImag = 0;

	int i;
	for(i = 0; i <= CBUFF_LEN; i+= 2) { //indexing by 2, B[odd] = 0
		sImag += x[(x_index+i)%CBUFF_LEN]*B[i]; //dot product
	}

	sReal = x[(x_index+15)%CBUFF_LEN];//*scaleFactor; //group delay of filter is 15 samples

	//scaleFactor = 1/sqrt(sReal*sReal + sImag*sImag);
	sReal *= scaleFactor; //scaling prior to loop filter
	sImag *= scaleFactor; //scaling prior to loop filter

	x_index = (x_index+1)%(CBUFF_LEN);

	//execute D-PLL (loop)
	vcoOutputReal = cosf(phi);
	vcoOutputImag = sinf(phi);

	if(buffer_index < N) buffer[buffer_index] = vcoOutputReal;
	buffer_index++;

	phaseDetectorOutputReal = sReal*vcoOutputReal+sImag*vcoOutputImag;
	phaseDetectorOutputImag = sImag*vcoOutputReal-sReal*vcoOutputImag;
	q = phaseDetectorOutputReal*phaseDetectorOutputImag;
	sigma += beta*q;
	loopFilterOutput = sigma+alpha*q;
	phi+=2.0*pi*Fmsg/(float)SAMP_FREQ + loopFilterOutput;

	output_freq = Fmsg + SAMP_FREQ*loopFilterOutput/(2.0*pi);

	if(phi > 2*pi) phi -= 2*pi; //modulo 2*pi
	//if(phi < 0) phi += 2*pi;



	counter++;
	counter = counter%N;

	left_sample = amplitude*sin(phi_t);
	output_left_sample(left_sample);
}

int main() {
	L138_initialise_intr(FS_48000_HZ,ADC_GAIN_24DB,DAC_ATTEN_0DB,LCDK_MIC_INPUT);
	while(1);
}
