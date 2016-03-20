#include "bit_vector.h"
#include "DES_encrypt.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Subsitition boxes used in Feistel function
char s_box1[64] =  { 14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7,
					 0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8,
					 4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0,
					 15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13 };

char s_box2[64] =  { 15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10,
					 3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5,
					 0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15,
					 13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9 };

char s_box3[64] =  { 10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8,
					 13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1,
					 13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7,
					 1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12 };

char s_box4[64] =  { 7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15,
					 13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9,
					 10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4,
					 3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14 };

char s_box5[64] =  { 2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9,
					 14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6,
					 4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14,
					 11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3 };

char s_box6[64] =  { 12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11,
					 10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8,
					 9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6,
					 4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13 };

char s_box7[64] =  { 4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1,
					 13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6,
					 1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2,
					 6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12 };

char s_box8[64] =  { 13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7,
					 1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2,
					 7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8,
					 2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11 };
					 
					 
// Permutation used in Feistel function
short permutation[32] = { 15,  6, 19, 20, 28, 11, 27, 16,
						   0, 14, 22, 25,  4, 17, 30,  9,
						   1,  7, 23, 13, 31, 26,  2,  8,
						  18, 12, 29,  5, 21, 10,  3, 24 };

// First permutation used in key generation
short key_permutation1[56] = { 56, 48, 40, 32, 24 , 16,  8,
        					    0, 57,  49, 41, 33, 25, 17,
        					   11,  1,  58, 50, 42, 34, 26,
        					   18, 10,   2, 59, 51, 43, 35,
        					   62, 54,  46, 38, 30, 22, 14,
        					    6, 61,  53, 45, 37, 29, 21,
        				  	   13,  5,  60, 52, 44, 36, 28,
        					   20, 12,   4, 27, 19, 11,  3 };
 
// Shifts used during key generation
short key_shifts[16] = {1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1};

// Second set of permutations used in key generation
short m_key_permutation[32] =  {   16, 10, 23,  0,
								   27, 14,  5, 20,
								   18, 11,  3, 25,
								    6, 26, 19, 12,
								   51, 30, 36, 46,
								   39, 50, 44, 32,
								   48, 38, 55, 33,
								   41, 49, 35, 28    };

short e_key_permutation[16] = {13,  4,  2,  9, 22,  7, 15,  1,
								 40, 54, 29, 47, 43, 52, 45, 31 };

// 16 Round keys (split by axis index to s-box)
int    m_round_key[16] = {0};
short  e_round_key[16] = {0};

/* void generateKeys(void)
 *  Takes in 64 bit key, produces 16 round keys
 *  -round keys depend on only 56 bits of key
 *  -key is passed i in rightand left halves
 *  -call this in main()
 */
void generateKeys(int lkey, int rkey) {
	int i;
	int c[17] = {0};
	int d[17] = {0};

	bv_t temp_vecl = malloc (sizeof (struct bitvec));
	bv_new(temp_vecl,32);
	bv_t temp_vecr = malloc (sizeof (struct bitvec));
	bv_new(temp_vecr,32);

	load(temp_vecl,lkey);
	load(temp_vecr,rkey);

	print_vec(temp_vecl);
	print_vec(temp_vecr);

	int index;
	for(i = 0; i < 28; i++) {
		index = key_permutation1[i];
		if(index < 32)
			c[0] ^= (int)get_bit(temp_vecl,index) << (31 - i);
		else
			c[0] ^= (int)get_bit(temp_vecr,index-32) << (31 - i);
	}
	for(i = 0; i < 28; i++) {
		index = key_permutation1[28+i];
		if(index < 32)
			d[0] ^= (int)get_bit(temp_vecl,index) << (31 - i);
		else
			d[0] ^= (int)get_bit(temp_vecr,index-32) << (31 - i);
	}

	//printf("%d, %d \n", c[0],d[0]);

	for(i = 1; i <= 16; i++) {
		if(key_shifts[i-1] == 1) {
			c[i] = (c[i-1] << 1) ^ (c[i-1] >> 27);
			d[i] = (d[i-1] << 1) ^ (d[i-1] >> 27);
		}
		else {
			c[i] = (c[i-1] << 2) ^ (c[i-1] >> 26);
			d[i] = (d[i-1] << 2) ^ (d[i-1] >> 26);
		}
		//printf("%d, %d \n", c[i],d[i]);
	}


	int j;
	for(i = 0; i < 16; i++) {

		load(temp_vecl,c[i+1]);
		load(temp_vecr,d[i+1]);
		print_vec(temp_vecl);
		print_vec(temp_vecr);

		for(j = 0; j < 32; j++) {
			index = m_key_permutation[j];
			if(index < 28)
				m_round_key[i] ^= (int)get_bit(temp_vecl,index) << (31 - j);
			else
				m_round_key[i] ^= (int)get_bit(temp_vecr,index-28) << (31 - j);
		}
		//printf("%d, ", m_round_key[i]);
		for(j = 0; j < 16; j++) {
			index = e_key_permutation[j];
			if(index < 28)
				e_round_key[i] ^= (short)get_bit(temp_vecl,index) << (15 - j);
			else
				e_round_key[i] ^= (short)get_bit(temp_vecr,index-28) << (15 - j);
		}
		//printf("%d \n", e_round_key[i]);
	}
}

/* void feistelSub(bv_t, short)
 *  Performs the feistel substition for a given round
 *  -uses the current round key to XOR, s-boxes for subst
 *  -loop unrolled for interrupt compatibility
 */
void feistel_sub(bv_t bv, short round) {
	short indices = 0;

	int i;
	for(i = 0; i < 8; i++) {
		indices += get_bit(bv,(4*i-1)%32) << (i*2);
		indices += get_bit(bv,(4*i+1)%32) << (i*2+1);
	}

	indices ^= e_round_key[round];
	int temp = unload(bv)^m_round_key[round];
	int temp_vec = 0;

	clear_vec(bv);
	short index1 = indices & 0x03;
	short index2 = temp & 0x0F;
	temp_vec ^= s_box1[8*index1+index2];
	//linear_append(bv,s_box1[8*index1+index2]);
	index1 = (indices >> 2) & 0x03;
	index2 = (indices >> 4) & 0x0F;
	temp_vec ^= s_box2[8*index1+index2] << 4;
	//linear_append(bv,s_box2[8*index1+index2]);
	index1 = (indices >> 4) & 0x03;
	index2 = (indices >> 8) & 0x0F;
	temp_vec ^= s_box3[8*index1+index2] << 8;
	//linear_append(bv,s_box3[8*index1+index2]);
	index1 = (indices >> 6) & 0x03;
	index2 = (indices >> 12) & 0x0F;
	temp_vec ^= s_box4[8*index1+index2] << 12;
	//linear_append(bv,s_box4[8*index1+index2]);
	index1 = (indices >> 8) & 0x03;
	index2 = (indices >> 16) & 0x0F;
	temp_vec ^= s_box5[8*index1+index2] << 16;
	//linear_append(bv,s_box5[8*index1+index2]);
	index1 = (indices >> 10) & 0x03;
	index2 = (indices >> 20) & 0x0F;
	temp_vec ^= s_box6[8*index1+index2] << 20;
	//linear_append(bv,s_box6[8*index1+index2]);
	index1 = (indices >> 12) & 0x03;
	index2 = (indices >> 24) & 0x0F;
	temp_vec ^= s_box7[8*index1+index2] << 24;
	//linear_append(bv,s_box7[8*index1+index2]);
	index1 = (indices >> 14) & 0x03;
	index2 = (indices >> 28) & 0x0F;
	temp_vec ^= s_box8[8*index1+index2] << 28;
	//linear_append(bv,s_box8[8*index1+index2]);
	load(bv, temp_vec);
}

/* void feistel_perm(bv_t)
 *  Uses fixed permutation on bitvector
 *  -should be called right after feistel_sub
 *  -loop unrolled for interrupt compatibility
 */
void feistel_perm(bv_t bv) {
	int temp = 0;
	temp ^= (int)get_bit(bv,permutation[0]);
	temp ^= (int)get_bit(bv,permutation[1]) << 1;
	temp ^= (int)get_bit(bv,permutation[2]) << 2;
	temp ^= (int)get_bit(bv,permutation[3]) << 3;
	temp ^= (int)get_bit(bv,permutation[4]) << 4;
	temp ^= (int)get_bit(bv,permutation[5]) << 5;
	temp ^= (int)get_bit(bv,permutation[6]) << 6;
	temp ^= (int)get_bit(bv,permutation[7]) << 7;
	temp ^= (int)get_bit(bv,permutation[8]) << 8;
	temp ^= (int)get_bit(bv,permutation[9]) << 9;
	temp ^= (int)get_bit(bv,permutation[10]) << 10;
	temp ^= (int)get_bit(bv,permutation[11]) << 11;
	temp ^= (int)get_bit(bv,permutation[12]) << 12;
	temp ^= (int)get_bit(bv,permutation[13]) << 13;
	temp ^= (int)get_bit(bv,permutation[14]) << 14;
	temp ^= (int)get_bit(bv,permutation[15]) << 15;
	temp ^= (int)get_bit(bv,permutation[16]) << 16;
	temp ^= (int)get_bit(bv,permutation[17]) << 17;
	temp ^= (int)get_bit(bv,permutation[18]) << 18;
	temp ^= (int)get_bit(bv,permutation[19]) << 19;
	temp ^= (int)get_bit(bv,permutation[20]) << 20;
	temp ^= (int)get_bit(bv,permutation[21]) << 21;
	temp ^= (int)get_bit(bv,permutation[22]) << 22;
	temp ^= (int)get_bit(bv,permutation[23]) << 23;
	temp ^= (int)get_bit(bv,permutation[24]) << 24;
	temp ^= (int)get_bit(bv,permutation[25]) << 25;
	temp ^= (int)get_bit(bv,permutation[26]) << 26;
	temp ^= (int)get_bit(bv,permutation[27]) << 27;
	temp ^= (int)get_bit(bv,permutation[28]) << 28;
	temp ^= (int)get_bit(bv,permutation[29]) << 29;
	temp ^= (int)get_bit(bv,permutation[30]) << 30;
	temp ^= (int)get_bit(bv,permutation[31]) << 31;

	load(bv,temp);
}

/* if interrupt will accept it, this function will perform
 * the entire round, otherwise use this code, copy paste and
 * split over two interrupts
 */
void feistel_round(bv_t left, bv_t right, short round) {
	int temp = unload(left);
	copy_vec(left, right);
	feistel_sub(right, round);
	feistel_perm(right);
	temp ^= unload(right);
	load(right, temp);
}
