#ifndef VITERBI_H
#define VITERBI_H

#define CODE_LENGTH     2
#define NUM_CODES		4 // = 2^CODE_LENGTH
#define STATE_VARS		2
#define NUM_STATES      4 // = 2^STATE_VARS
#define TRUE            1
#define FALSE           0
#define INT_MAX			0x7FFFFFFF




// viterbi decoder
void setupDecoder();
void vit_dec();
void vit_dec_bmh(short m_hat);
void vit_dec_bms(short m_hat, float pr);
void vit_dec_ACS(short i);
void vit_dec_get(bv_t dest);
void vit_dec_reset();
void update_paths();

#endif
