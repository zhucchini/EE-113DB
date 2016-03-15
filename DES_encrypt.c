#include "bit_vector.h"
#include "DES_encrypt.h"
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

short permutation[32] = { 15,  6, 19, 20, 28, 11, 27, 16,
						   0, 14, 22, 25,  4, 17, 30,  9,
						   1,  7, 23, 13, 31, 26,  2,  8,
						  18, 12, 29,  5, 21, 10,  3, 24 };

int    m_round_key[16];
short  e_round_key[16];

void generateKeys(long h) {
	int i;
	for(i = 0; i < 16; i++) {
		m_round_key[i] = rand();
		e_round_key[i] = 0xFFFF & rand();
	}
}

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

//	set_bit(temp_vec,0,get_bit(bv,permutation[0]));
//	set_bit(temp_vec,1,get_bit(bv,permutation[1]));
//	set_bit(temp_vec,2,get_bit(bv,permutation[2]));
//	set_bit(temp_vec,3,get_bit(bv,permutation[3]));
//	set_bit(temp_vec,4,get_bit(bv,permutation[4]));
//	set_bit(temp_vec,5,get_bit(bv,permutation[5]));
//	set_bit(temp_vec,6,get_bit(bv,permutation[6]));
//	set_bit(temp_vec,7,get_bit(bv,permutation[7]));
//	set_bit(temp_vec,8,get_bit(bv,permutation[8]));
//	set_bit(temp_vec,9,get_bit(bv,permutation[9]));
//	set_bit(temp_vec,10,get_bit(bv,permutation[10]));
//	set_bit(temp_vec,11,get_bit(bv,permutation[11]));
//	set_bit(temp_vec,12,get_bit(bv,permutation[12]));
//	set_bit(temp_vec,13,get_bit(bv,permutation[13]));
//	set_bit(temp_vec,14,get_bit(bv,permutation[14]));
//	set_bit(temp_vec,15,get_bit(bv,permutation[15]));
//	set_bit(temp_vec,16,get_bit(bv,permutation[16]));
//	set_bit(temp_vec,17,get_bit(bv,permutation[17]));
//	set_bit(temp_vec,18,get_bit(bv,permutation[18]));
//	set_bit(temp_vec,19,get_bit(bv,permutation[19]));
//	set_bit(temp_vec,20,get_bit(bv,permutation[20]));
//	set_bit(temp_vec,21,get_bit(bv,permutation[21]));
//	set_bit(temp_vec,22,get_bit(bv,permutation[22]));
//	set_bit(temp_vec,23,get_bit(bv,permutation[23]));
//	set_bit(temp_vec,24,get_bit(bv,permutation[24]));
//	set_bit(temp_vec,25,get_bit(bv,permutation[25]));
//	set_bit(temp_vec,26,get_bit(bv,permutation[26]));
//	set_bit(temp_vec,27,get_bit(bv,permutation[27]));
//	set_bit(temp_vec,28,get_bit(bv,permutation[28]));
//	set_bit(temp_vec,29,get_bit(bv,permutation[29]));
//	set_bit(temp_vec,30,get_bit(bv,permutation[30]));
//	set_bit(temp_vec,31,get_bit(bv,permutation[31]));
//	bv_free(bv);
//	bv = temp_vec;
}

void feistel_round(bv_t left, bv_t right, short round) {
	int temp = unload(left);
	copy_vec(left, right);
	feistel_sub(right, round);
	feistel_perm(right);
	temp ^= unload(right);
	load(right, temp);
}
