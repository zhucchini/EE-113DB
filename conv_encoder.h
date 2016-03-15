#ifndef CONV_ENCODER_H
#define CONV_ENCODER_H

#define CODE_LENGTH      2
#define NUM_STATES       4
#define FALSE			 0
#define TRUE			 1


// convolutional encoder
void updateState(unsigned char n);
void setPuncturing(short s);
void clearState();
void setState(short s);
short getState();
void encode(bv_t dest, unsigned char bit);
void conv_encode(bv_t dest, bv_t src);
void puncture(bv_t dest, bv_t src);

#endif
