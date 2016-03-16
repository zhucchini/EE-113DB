#include "L138_LCDK_aic3106_init.h"
#include "evmomapl138_gpio.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"
#include "conv_encoder.h"
#include "viterbi.h"
#include "DES_encrypt.h"

// DEFINITIONS ---------------------------------------------------//
#define N		   		240
#define S				4
#define SCALE_SIZE		8
#define TRUE	   		1
#define FALSE	   		0
#define PI			    3.14159265358979
#define ROOT_NOTE		440.0 // in Hz
#define SAMPLING_FREQ	24000
#define PRECISION		36
#define PROCESS_TIME	21 // for receiver to do demodulation, viterbi, CRC ED
#define NUM_SECTIONS	2
#define SYMBOL_LENGTH	3
#define NUM_SYMBOLS		8 //2^SYMBOL_LENGTH
#define DATA_SIZE		32
#define DATA_POINTS		1000
//#define RS_TOTAL		??
//#define RS_SENT		??
#define SYM_PER_DATA	20 // (1/code_rate)*(32 + CRC_size)/SYMBOL_LENGTH
//#define ARQ_SYMS		??
//#define REPEAT_SYMS	??
#define MAX_SPEED		120.0
#define INT_MAX			0x7FFFFFFF
#define DEBUG			0
#define TEST			1
#define MAX_TRANS		1000


// NOT CONSTANTS -----------------------------------------------------//
float scale[PRECISION];
int   periods[PRECISION];
int   f_gray_code[] = {0,1,3,2,6,7,5,4};
int   r_gray_code[] = {0,1,3,2,7,6,4,5};
float dial;
float display;
int pad_a;
int pad_b;
int rep_pad;
short
int bit_errors = 0;
int num_transmissions = 0;
int num_arqs = 0;
bv_t test1;
bv_t test2;

// RECEIVER VARIABLES -----------------------------------------------------//
int flag = FALSE;
int peak_index;
int index = 0;
int n = 0;
int iter = 0;

short rec_init = TRUE;

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
bv_t rec_message;
int  rec_message_index = 0;
int  rec_transmission_index = 0;

short rec_arq_in_prog = FALSE;

// TRANSMITTER VARIABLES -----------------------------------------------------//
float phi = 0.0;
float amplitude = 50000;
int   curr_symbol = 0;
int   curr_bitvec = 0;
int   counter = 0;
int   message1 = 0;
int   message2 = 0;
int   rep_message1;
int   rep_message2;
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

bv_t  left;
bv_t  right;
int next_right;
int temp_left;

unsigned char* bit_ptr;


// FILTER VARIABLES
float a[NUM_SECTIONS][3];
float b[NUM_SECTIONS][3];
float w[NUM_SECTIONS][2];

// INLINE FUCTION DECLARATIONS -----------------------------------------//
inline void updateCorrelation(void);

inline void findMaxCorrelation(void);

inline void iirso_filter(void);

// FUNCTIONS -----------------------------------------------------//

interrupt void interrupt4(void) { // interrupt service routine
	left_sample = input_left_sample();


	// ------------------------------------------------BEGIN RECEIVER ------------------------------------------------------------------------------------
		window0[index] = left_sample;

		//iirso_filter();

		//*** TODO Change detection to PLL

		if(index > N/2 && index < N-PROCESS_TIME) {
			updateCorrelation();
		} else if(index >= N-PROCESS_TIME && !rec_init) {
			if(index < N-(PROCESS_TIME-6)) {
				findMaxCorrelation();
			} else if (index == N-(PROCESS_TIME-6)){
				vit_dec_bmh(r_gray_code[id] >> 1);
			} else if (index == N-(PROCESS_TIME-7)){
				vit_dec_ACS(0);
			} else if (index == N-(PROCESS_TIME-8)){
				vit_dec_ACS(1);
			} else if (index == N-(PROCESS_TIME-9)){
				vit_dec_ACS(2);
			} else if (index == N-(PROCESS_TIME-10)){
				vit_dec_ACS(3);
			} else if (index == N-(PROCESS_TIME-11)){
				update_paths();
			} else if (index == N-(PROCESS_TIME-12)){
				vit_dec_bmh((r_gray_code[id]) % 2);
			} else if (index == N-(PROCESS_TIME-13)){
				vit_dec_ACS(0);
			} else if (index == N-(PROCESS_TIME-14)){
				vit_dec_ACS(1);
			} else if (index == N-(PROCESS_TIME-15)){
				vit_dec_ACS(2);
			} else if (index == N-(PROCESS_TIME-16)){
				vit_dec_ACS(3);
			} else if (index == N-(PROCESS_TIME-17)){
				update_paths();
				rec_message_index++;
			} else if (rec_message_index == SYM_PER_DATA) {
				if (index == N-(PROCESS_TIME-18)) {
					vit_dec_get(rec_message);
					if (DEBUG) {
						printf("rec_message: ");
						print_vec(rec_message);
					}
				} else if (index == N-(PROCESS_TIME-19)) {
					if (!arq_in_prog && !check_CRC(rec_message,32)) {
						int temp300 = vit_unload(rec_message);
						display = MAX_SPEED*((float) (vit_unload(rec_message)^pad_a))/INT_MAX;
						if (DEBUG) {
							printf("vit_unload: %d, display: %f \n \n \n", temp300, display);
						}
						num_transmissions++;
					} else {
						if(!arq_in_prog) {
							arq_in_prog = TRUE;
							num_arqs++;
						} else {
							int temp300 = vit_unload(rec_message);
							display = MAX_SPEED*((float) (vit_unload(rec_message)^pad_a))/INT_MAX;
							arq_in_prog = FALSE;
							if(TEST) {
								load(test2,temp300^pad_a);
								bit_errors += hammingDistance(test1,test2);
							}
							num_transmissions++;
						}
					}
				} else if (index == N-(PROCESS_TIME-20)) {
					vit_dec_reset();
					rec_message_index = 0;
				}
			}



	//		if(res_index < N && index == N-1) {		//debugging purposes
	//			result[res_index] = target[id];
	//			res_index++;
	//		}

			//add to message (ED and EC go here)
	//		if(rec_message_index < message_len && index == N-1) {		//add to message
	//			rec_message[rec_message_index] = r_gray_code[id];
	//			rec_message_index++;
	//		}


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

		//*** TODO add preamble and key exchange (use ElGamal or RSA-256)

		//*** TODO implement frequency hopping

		//*** TODO Reed-Solomon encoding for HARQ II

		if(rec_ready) {
			if(counter == 0) {
				// get message from sensor (in this case dial
				// represent float data as an int
				message1 = (int) INT_MAX * (dial/MAX_SPEED);
				//load(p_block,message1);
				load(test1,message1);
				clear_vec(c_block);
				clearState();
				if (DEBUG) {
					printf("message1: %d, p_block: ", message1);
					print_vec(p_block);
				}

				// begin encryption (DES OFB mode)
				next_right = unload(right);

				//debugging
				//debug1 = unload(p_block);
			}
			else if(counter >= 1 && counter < 33) { // generate DES 'one-time pad' (16 rounds)
				if(counter%2 == 1) {
					temp_left = unload(left);
					copy_vec(left, right);
					feistel_sub(right, (counter-1)/2);
				}
				else {
					feistel_perm(right);
					temp_left ^= unload(right);
					load(right, temp_left);
				}

			}
			else if(counter == 33) { // xor with pad to get ciphertext
				load(right, next_right);
				pad_a = pad_b;
				pad_b = unload(left);
				message1 ^= pad_b; // actual message encryption
				load(p_block, message1);
	//			unload(e_block, p_block);
	//			bitvec_xor(e_block, left);
			}
			else if(counter == 34) { //add CRC for ED
				add_CRC(p_block,32);
				//add_CRC(e_block);
				//RS_encode(e_block,rs_block);
			}
			else if(counter >= 35 && counter < 75){ //encode with conv encoder
				//conv_encode(c_block, rs_block);
				//conv_encode(c_block, p_block);
				encode(c_block, pop(p_block));

				if (DEBUG && counter == 74) {
					printf("c_block: ");
					print_vec(c_block);
				}
			}
			else if(counter == 75) {
				message1 = unload(c_block);
				message2 = unload2(c_block);

				// printf("message1: %d, message2: %d \n", message1, message2);
				rec_ready = FALSE;
			}

		} else {
			// printf("why? \n");
			// transmission_index = 0;
			// rec_ready = FALSE;
		}

		if(counter == N-1) {
	//		debug2 = get(c_block,transmission_index*3,transmission_index*3+2); //for debugging
	//		curr_symbol = f_gray_code[get(c_block,transmission_index*3,transmission_index*3+2)];
			if(transmission_index == 0 && arq_in_prog) {
				message1 = rep_message1;
				message2 = rep_message2;
				pad_b = rep_pad;
			}

			int start_index = transmission_index*3;
			if(start_index > 31) {
				curr_symbol = f_gray_code[(message2 >> (28-(start_index-33))) & 0x07];
			} else if(start_index > 29) {
				curr_symbol = f_gray_code[((message1 << 1) ^ ((message2 >> 31) % 2)) & 0x07];
			} else {
				curr_symbol = f_gray_code[(message1 >> (29-start_index)) & 0x07];
			}

			transmission_index++;
			if(transmission_index == SYM_PER_DATA) {
				rec_ready = TRUE;
				transmission_index = 0;
				rep_message1 = message1;
				rep_message2 = message2;
				rep_pad = pad_b;
			}
			symbols[message_index] = target[7-curr_symbol];
			rec_init = FALSE;
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
		scale[i] = ROOT_NOTE*pow(2,i/36.0);
		periods[i] = SAMPLING_FREQ/scale[i];
		correlation[i] = 0;
	}

	for(i = 0; i < NUM_SYMBOLS; i++) {
		target[i] = ROOT_NOTE*pow(2,i/((double)NUM_SYMBOLS+1.0));
		targetPeriod[i] = SAMPLING_FREQ/scale[i];
	}


	for(i = 0; i < 7; i++)
	{
		targetPeriod[i]=SAMPLING_FREQ/target[i];
	}

	setupDecoder();


	test1 = malloc (sizeof(struct bitvec));
	test2 = malloc (sizeof(struct bitvec));
	bv_new(test1,32);
	bv_new(test2,32);

	p_block = malloc (sizeof(struct bitvec));
	e_block = malloc (sizeof(struct bitvec));
	rs_block = malloc (sizeof(struct bitvec));
	c_block = malloc (sizeof(struct bitvec));
	bv_new(p_block, 32);
	bv_new(e_block, 1);
	bv_new(rs_block, 1);
	bv_new(c_block, 1);

	right = malloc (sizeof (struct bitvec));
	left = malloc (sizeof (struct bitvec));
	bv_new(right,32);
	bv_new(left,32);
	load(right, rand()*rand());
	load(left, rand()*rand());

	int key_right = rand()*rand();
	int key_left = rand()*rand();

	generateKeys(key_left, key_right);

	rec_message = malloc (sizeof (struct bitvec));
	bv_new(rec_message,32);

	bit_ptr = p_block->bits;

	//load(p_block,0xF35AC);
	//add_CRC(p_block,32);
//	clear_vec(p_block);
//	load(p_block, 0xF35AC);
//	add_CRC(p_block,32);
//	print_vec(p_block);
//	clear_vec(c_block);
//	for(i = 0; i < 40; i++) {
//		unsigned short abc = pop(p_block);
////		printf("bit: %u, ", abc);
//		encode(c_block, abc);
//	}
//	print_vec(c_block);
//	int m1 = unload(c_block);
//	int m2 = unload2(c_block);
//	printf("message: %d, \n", m1);//, m2);
//
//	for(i = 0; i < 20; i++) {
//		int start_index = i*3;
//		if(start_index > 31)
//			curr_symbol = (m2 >> (28-(start_index-33))) & 0x07;//curr_symbol = f_gray_code[(message2 >> (28-(start_index-33))) & 0x07];
//		else if(start_index > 29)
//			curr_symbol = ((m1 << 1) ^ ((m2 >> 31) % 2)) & 0x07;//f_gray_code[((message1 << 1) ^ ((message2 >> 31) % 2)) & 0x07];
//		else
//			curr_symbol = (m1 >> (29-start_index)) & 0x07;//f_gray_code[(message1 >> (29-start_index)) & 0x07];
//		printf("curr_symbol: %d, curr_symbol >> 1: %d \n", curr_symbol, curr_symbol >> 1);
//		vit_dec_bmh(curr_symbol >> 1);
//		vit_dec_ACS(0);
//		vit_dec_ACS(1);
//		vit_dec_ACS(2);
//		vit_dec_ACS(3);
//		update_paths();
//
//		printf("curr_symbol: %d, curr_symbol mod 2: %d \n", curr_symbol, curr_symbol % 2);
//		vit_dec_bmh(curr_symbol % 2);
//		vit_dec_ACS(0);
//		vit_dec_ACS(1);
//		vit_dec_ACS(2);
//		vit_dec_ACS(3);
//		update_paths();
//	}
//	printf("done \n \n \n");
//	vit_dec_get(rec_message);
//	print_vec(rec_message);
//	printf("CRC?: %d \n", check_CRC(rec_message,32));

//	int test1 = (0xE9 << 8) + 0xC7;
//	load(p_block,test1);
//	printf("%d : %d \n", test1, unload(p_block));
//	//print_vec(p_block);
//	int test2 = get(p_block,16,18);
//	printf("%d \n", test2);
//	test2 = get(p_block,25,28);
//	printf("%d \n", test2);
//	test2 = get(p_block,22,26);
//	printf("%d \n", test2);



//	print_vec(right);
//	print_vec(left);
//
//	load(p_block, 0xFC4A78E5);
//	print_vec(p_block);
//
//	int j;
//	bv_t next_right = malloc (sizeof (struct bitvec));
//	for(j = 0; j < 7; j ++) {
//		bv_new(next_right,32);
//		copy_vec(next_right, left);
//
//		for(i = 0; i < 16; i++) {
//			feistel_round(right,left,i);
//		}
//
//		bv_free(right);
//		right = next_right;
//
//		copy_vec(c_block, p_block);
//		bitvec_xor(c_block,left);
//		print_vec(c_block);
//		bitvec_xor(c_block,left);
//		print_vec(c_block);
//	}

//	add_CRC(p_block,32);
//	print_vec(p_block);
//	printf("check: %d \n", check_CRC(p_block,32));
//	print_vec(p_block);
//	conv_encode(c_block, p_block);
//	print_vec(c_block);
//
//
//
//
//
//	linear_append(e_block,0x03);
//	print_vec(e_block);
//	conv_encode(rs_block,e_block);
//	print_vec(rs_block);
//
//	vit_dec(e_block, rs_block);
//	print_vec(e_block);


	L138_initialise_intr(FS_24000_HZ,ADC_GAIN_24DB,DAC_ATTEN_0DB,LCDK_MIC_INPUT);

    while(num_transmissions < MAX_TRANS) {

    }

    printf("%d errors in %d transmissions \n", bit_errors, num_transmissions);

    bv_free(test1);
    bv_free(test2);
    free(test1);
    free(test2);

    bv_free(p_block);
    bv_free(e_block);
    bv_free(rs_block);
    bv_free(c_block);
    free(p_block);
    free(e_block);
    free(rs_block);
    free(c_block);

    bv_free(right);
    free(right);
    bv_free(left);
    free(left);
    bv_free(rec_message);
    free(rec_message);

}

// INLINE FUNCTIONS -----------------------------------------------------//
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


