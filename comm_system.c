#include "L138_LCDK_aic3106_init.h"
#include "evmomapl138_gpio.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

// DEFINITIONS ---------------------------------------------------//
#define N		   		480
#define S				4
#define SCALE_SIZE		8
#define TRUE	   		1
#define FALSE	   		0
#define PI			    3.14159265358979
#define A3				220.0 // in Hz
#define SAMPLING_FREQ	48000
#define PRECISION		36
#define PROCESS_TIME	6
#define NUM_SECTIONS	2
#define SYMBOL_LENGTH	3
#define NUM_SYMBOLS		8 //2^SYMBOL_LENGTH
#define DATA_SIZE		32
#define DATA_POINTS		1000
//#define RS_TOTAL		??
//#define RS_SENT		??
#define SYM_PER_DATA	16
//#define ARQ_SYMS		??
//#define REPEAT_SYMS	??
#define MAX_SPEED		120.0
#define INT_MAX			0x7FFFFFFF


// NOT CONSTANTS -----------------------------------------------------//
float scale[PRECISION];
int   periods[PRECISION];
int   f_gray_code[] = {0,1,3,2,6,7,5,4};
int   r_gray_code[] = {0,1,3,2,7,6,4,5};
float dial;

// RECEIVER VARIABLES -----------------------------------------------------//
int flag = FALSE;
int peak_index;
int index = 0;
int n = 0;
int iter = 0;

float target[NUM_SYMBOLS]; //symbol frequencies
int   targetPeriod[NUM_SYMBOLS];
float phase;
float correlation[PRECISION];
float window0[N];
float left_sample;
float output[N];
int   max = 0;
int   max_a = 0, max_b = 9, max_c = 18, max_d = 27;
int   id = 0;
int   samplesLeft = 0;
float ratio = 1.0;

float result[N];
int   res_index = 0;

int  curr_rec_symbol;
char curr_rec_bitvec;

char buffer[3];
int  rec_message[60] = {0};
int  rec_message_index = 0;
int  rec_transmission_index = 0;

short rec_arq_in_prog = FALSE;

// TRANSMITTER VARIABLES -----------------------------------------------------//
float phi = 0.0;
float amplitude = 50000;
int   curr_symbol = 0;
int   curr_bitvec = 0;
int   counter = 0;
unsigned int   message = 0;
float symbols[25];
int   message_len = 25;
int   message_index = 0;

int   debug1;
unsigned short   debug2;

int   transmission_index = 0;
short rec_ready = TRUE;
short arq_in_prog = FALSE;

bv_t  p_block;
bv_t  e_block;
bv_t  rs_block;
bv_t  c_block;

unsigned char* bit_ptr;


// FILTER VARIABLES
float a[NUM_SECTIONS][3];
float b[NUM_SECTIONS][3];
float w[NUM_SECTIONS][2];

// INLINE FUCTION DECLARATIONS -----------------------------------------//
inline void updateCorrelation(void);

inline void findMaxCorrelation(void);

inline void iirso_filter(void);

inline void bitvec_to_symbol(void);

inline void symbol_to_bitvec(void);

// FUNCTIONS -----------------------------------------------------//

interrupt void interrupt4(void) { // interrupt service routine
	left_sample = input_left_sample();




	// ------------------------------------------------BEGIN RECEIVER ------------------------------------------------------------------------------------
	window0[index] = left_sample;

	//iirso_filter();

	//*** TODO Change detection to PLL

	if(index > N/2 && index < N-PROCESS_TIME) {
		updateCorrelation();
	} else if(index >= N-PROCESS_TIME) {
		findMaxCorrelation();
		// ratio = target[id]/scale[max]; //auto-tune stuff

		if(res_index < N && index == N-1) {		//debugging purposes
			result[res_index] = target[id];
			res_index++;
		}

		//add to message (ED and EC go here)
		if(rec_message_index < message_len && index == N-1) {		//add to message
			rec_message[rec_message_index] = r_gray_code[id];
			rec_message_index++;
		}


	} //else { //first half of window for processing
//
//	}

	//*** TODO Implement ARQ and Viterbi Decode here ***


	//*** TODO Clock Detection (using ZCR?)

	index++;
	index = index % N;

	// -------------------------------------------------END RECEIVER----------------------------------------------------------------------------------------






	// -----------------------------------------------BEGIN TRANSMITTER--------------------------------------------------------------------------------------
	phi += (target[7-curr_symbol]/(float)SAMPLING_FREQ)*2.0*PI;
	if(phi > 2.0*PI) phi -= 2.0*PI;

	//*** TODO add preamble and key exchange (use ElGamal)

	//*** TODO implement frequency hopping

	//*** TODO encoding here
	if(!arq_in_prog && rec_ready) {
		if(counter == 0) {
			message = (int) INT_MAX * (dial/MAX_SPEED);
			load(p_block,message);
			//message_index++;
			message_index = message_index%message_len;
			//e_block = encrypt(p_block) //encryption
			debug1 = unload(p_block);
		}
		else if(counter == 1) {
			//RS_encode(c_block,rs_block);
		}
		else if(counter >= 2 && counter < 34){
			//conv_encode(c_block, rs_block);
			//conv_encode(c_block, p_block);
			encode(c_block, pop(p_block));
		}
		else if(counter == 34) {
			rec_ready = FALSE;
		}

	}
	if(!arq_in_prog) {
		if(counter == N-1) {
			debug2 = get(c_block,transmission_index*3,transmission_index*3+2); //for debugging
			curr_symbol = f_gray_code[get(c_block,transmission_index*3,transmission_index*3+2)];
//			curr_symbol = message[message_index];
//			message_index++;
//			message_index = message_index%message_len;
			transmission_index++;
			if(transmission_index == 9) {
				rec_ready = TRUE;
				transmission_index = 0;
			}
			symbols[message_index] = target[7-curr_symbol];
		}
	}

	counter++;
	counter = counter%N;
	// ------------------------------------------------END TRANSMITTER---------------------------------------------------------------------------------------



	output_left_sample(amplitude*sin(phi));
	return;
}

int main(void) {
	int i;
	for(i = 0; i < PRECISION; i++) {
		scale[i] = A3*pow(2,i/36.0);
		periods[i] = SAMPLING_FREQ/scale[i];
		correlation[i] = 0;
	}

	for(i = 0; i < NUM_SYMBOLS; i++) {
		target[i] = A3*pow(2,i/((double)NUM_SYMBOLS+1.0));
		targetPeriod[i] = SAMPLING_FREQ/scale[i];
	}


	for(i = 0; i < 7; i++)
	{
		targetPeriod[i]=SAMPLING_FREQ/target[i];
	}

	p_block = malloc (sizeof(struct bitvec));
	e_block = malloc (sizeof(struct bitvec));
	rs_block = malloc (sizeof(struct bitvec));
	c_block = malloc (sizeof(struct bitvec));
	bv_new(p_block, 32);
	bv_new(e_block, 1);
	bv_new(rs_block, 1);
	bv_new(c_block, 1);

	bit_ptr = p_block->bits;

	int test1 = (0xE9 << 8) + 0xC7;
	load(p_block,test1);
	printf("%d : %d \n", test1, unload(p_block));
	print_vec(p_block);
	int test2 = get(p_block,16,18);
	printf("%d \n", test2);
	test2 = get(p_block,25,28);
	printf("%d \n", test2);
	test2 = get(p_block,22,26);
	printf("%d \n", test2);

	L138_initialise_intr(FS_48000_HZ,ADC_GAIN_24DB,DAC_ATTEN_0DB,LCDK_MIC_INPUT);

    while(1) {

    }

    print_vec(c_block);

    bv_free(p_block);
    bv_free(e_block);
    bv_free(rs_block);
    bv_free(c_block);
    free(p_block);
    free(e_block);
    free(rs_block);
    free(c_block);

}

// INLINE FUNCTIONS -----------------------------------------------------//
inline void bitvec_to_symbol(void) {
	curr_symbol = curr_bitvec ^ (curr_bitvec >> 1);
}

inline void symbol_to_bitvec(void) {

	curr_bitvec = curr_symbol;
	curr_bitvec ^= (curr_bitvec >> 16);
	curr_bitvec ^= (curr_bitvec >> 8);
	curr_bitvec ^= (curr_bitvec >> 4);
	curr_bitvec ^= (curr_bitvec >> 2);
	curr_bitvec ^= (curr_bitvec >> 1);
}


inline void iirso_filter(void) {
	//IIR Chebyshev, Butterworth, or Elliptic Filter here (see OMAP L138 by Reay p.174) ***
	float wn;
	float yn = left_sample;
	int section = 0;

	wn = yn - a[section][1]*w[section][0] - a[section][2]*w[section][1];
	yn = yn - b[section][0]*wn + b[section][1]*w[section][0] + b[section][2]*w[section][1];
	w[section][1] = w[section][0];
	w[section][0] = wn;

	section = 1;

	wn = yn - a[section][1]*w[section][0] - a[section][2]*w[section][1];
	yn = yn - b[section][0]*wn + b[section][1]*w[section][0] + b[section][2]*w[section][1];
	w[section][1] = w[section][0];
	w[section][0] = wn;


}

inline void updateCorrelation(void) {
	float temp;
	temp = (left_sample - window0[index-periods[0]]);
	correlation[0] += temp*temp;
	temp = (left_sample - window0[index-periods[1]]);
	correlation[1] += temp*temp;
	temp = (left_sample - window0[index-periods[2]]);
	correlation[2] += temp*temp;
	temp = (left_sample - window0[index-periods[3]]);
	correlation[3] += temp*temp;
	temp = (left_sample - window0[index-periods[4]]);
	correlation[4] += temp*temp;
	temp = (left_sample - window0[index-periods[5]]);
	correlation[5] += temp*temp;
	temp = (left_sample - window0[index-periods[6]]);
	correlation[6] += temp*temp;
	temp = (left_sample - window0[index-periods[7]]);
	correlation[7] += temp*temp;
	temp = (left_sample - window0[index-periods[8]]);
	correlation[8] += temp*temp;
	temp = (left_sample - window0[index-periods[9]]);
	correlation[9] += temp*temp;
	temp = (left_sample - window0[index-periods[10]]);
	correlation[10] += temp*temp;
	temp = (left_sample - window0[index-periods[11]]);
	correlation[11] += temp*temp;
	temp = (left_sample - window0[index-periods[12]]);
	correlation[12] += temp*temp;
	temp = (left_sample - window0[index-periods[13]]);
	correlation[13] += temp*temp;
	temp = (left_sample - window0[index-periods[14]]);
	correlation[14] += temp*temp;
	temp = (left_sample - window0[index-periods[15]]);
	correlation[15] += temp*temp;
	temp = (left_sample - window0[index-periods[16]]);
	correlation[16] += temp*temp;
	temp = (left_sample - window0[index-periods[17]]);
	correlation[17] += temp*temp;
	temp = (left_sample - window0[index-periods[18]]);
	correlation[18] += temp*temp;
	temp = (left_sample - window0[index-periods[19]]);
	correlation[19] += temp*temp;
	temp = (left_sample - window0[index-periods[20]]);
	correlation[20] += temp*temp;
	temp = (left_sample - window0[index-periods[21]]);
	correlation[21] += temp*temp;
	temp = (left_sample - window0[index-periods[22]]);
	correlation[22] += temp*temp;
	temp = (left_sample - window0[index-periods[23]]);
	correlation[23] += temp*temp;
	temp = (left_sample - window0[index-periods[24]]);
	correlation[24] += temp*temp;
	temp = (left_sample - window0[index-periods[25]]);
	correlation[25] += temp*temp;
	temp = (left_sample - window0[index-periods[26]]);
	correlation[26] += temp*temp;
	temp = (left_sample - window0[index-periods[27]]);
	correlation[27] += temp*temp;
	temp = (left_sample - window0[index-periods[28]]);
	correlation[28] += temp*temp;
	temp = (left_sample - window0[index-periods[29]]);
	correlation[29] += temp*temp;
	temp = (left_sample - window0[index-periods[30]]);
	correlation[30] += temp*temp;
	temp = (left_sample - window0[index-periods[31]]);
	correlation[31] += temp*temp;
	temp = (left_sample - window0[index-periods[32]]);
	correlation[32] += temp*temp;
	temp = (left_sample - window0[index-periods[33]]);
	correlation[33] += temp*temp;
	temp = (left_sample - window0[index-periods[34]]);
	correlation[34] += temp*temp;
	temp = (left_sample - window0[index-periods[35]]);
	correlation[35] += temp*temp;
}

inline void findMaxCorrelation(void) {
	if(index == N-PROCESS_TIME) {
		if (correlation[1] < correlation[max_a]) max_a =1;
		if (correlation[2] < correlation[max_a]) max_a =2;
		if (correlation[3] < correlation[max_a]) max_a =3;
		if (correlation[4] < correlation[max_a]) max_a =4;
		if (correlation[5] < correlation[max_a]) max_a =5;
		if (correlation[6] < correlation[max_a]) max_a =6;
		if (correlation[7] < correlation[max_a]) max_a =7;
		if (correlation[8] < correlation[max_a]) max_a =8;
	} else if(index == N-(PROCESS_TIME-1)) {
		if (correlation[10] < correlation[max_b]) max_b =10;
		if (correlation[11] < correlation[max_b]) max_b =11;
		if (correlation[12] < correlation[max_b]) max_b =12;
		if (correlation[13] < correlation[max_b]) max_b =13;
		if (correlation[14] < correlation[max_b]) max_b =14;
		if (correlation[15] < correlation[max_b]) max_b =15;
		if (correlation[16] < correlation[max_b]) max_b =16;
		if (correlation[17] < correlation[max_b]) max_b =17;
	} else if(index == N-(PROCESS_TIME-2)) {
		if (correlation[19] < correlation[max_c]) max_c =19;
		if (correlation[20] < correlation[max_c]) max_c =20;
		if (correlation[21] < correlation[max_c]) max_c =21;
		if (correlation[22] < correlation[max_c]) max_c =22;
		if (correlation[23] < correlation[max_c]) max_c =23;
		if (correlation[24] < correlation[max_c]) max_c =24;
		if (correlation[25] < correlation[max_c]) max_c =25;
		if (correlation[26] < correlation[max_c]) max_c =26;
	} else if(index == N-(PROCESS_TIME-3)) {
		if (correlation[28] < correlation[max_d]) max_d =28;
		if (correlation[29] < correlation[max_d]) max_d =29;
		if (correlation[30] < correlation[max_d]) max_d =30;
		if (correlation[31] < correlation[max_d]) max_d =31;
		if (correlation[32] < correlation[max_d]) max_d =32;
		if (correlation[33] < correlation[max_d]) max_d =33;
		if (correlation[34] < correlation[max_d]) max_d =34;
		if (correlation[35] < correlation[max_d]) max_d =35;
	} else if(index == N-(PROCESS_TIME-4)) { //index == N-2
		max = max_a;
		if(correlation[max_b] < correlation[max]) max = max_b;
		if(correlation[max_c] < correlation[max]) max = max_c;
		if(correlation[max_d] < correlation[max]) max = max_d;
		max_a = 0;
		max_b = 9;
		max_c = 18;
		max_d = 27;

		id = 7;
		float diff = abs(target[0] - scale[max]);
		float new_diff = abs(target[1] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 6;
		}
		new_diff = abs(target[2] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 5;
		}
		new_diff = abs(target[3] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 4;
		}
		new_diff = abs(target[4] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 3;
		}
		new_diff = abs(target[5] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 2;
		}
		new_diff = abs(target[6] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 1;
		}
		new_diff = abs(target[7] - scale[max]);
		if(new_diff < diff) {
			diff = new_diff;
			id = 0;
		}
	} else {
		correlation[0] = 0;
		correlation[1] = 0;
		correlation[2] = 0;
		correlation[3] = 0;
		correlation[4] = 0;
		correlation[5] = 0;
		correlation[6] = 0;
		correlation[7] = 0;
		correlation[8] = 0;
		correlation[9] = 0;
		correlation[10] = 0;
		correlation[11] = 0;
		correlation[12] = 0;
		correlation[13] = 0;
		correlation[14] = 0;
		correlation[15] = 0;
		correlation[16] = 0;
		correlation[17] = 0;
		correlation[18] = 0;
		correlation[19] = 0;
		correlation[20] = 0;
		correlation[21] = 0;
		correlation[22] = 0;
		correlation[23] = 0;
		correlation[24] = 0;
		correlation[25] = 0;
		correlation[26] = 0;
		correlation[27] = 0;
		correlation[28] = 0;
		correlation[29] = 0;
		correlation[30] = 0;
		correlation[31] = 0;
		correlation[32] = 0;
		correlation[33] = 0;
		correlation[34] = 0;
		correlation[35] = 0;

	}
}


