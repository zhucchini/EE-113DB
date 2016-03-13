#ifndef VECTOR_H
#define VECTOR_H

// BITVECTOR DEFINITION ---------------------------------------------------------------//
struct bitvec{
    unsigned char* bits;     // where the bits are stored
    short num_bits; // number of bits being used in the char array
    short len;      // size of the char array
};
typedef struct bitvec* bv_t;


// FUNCTION DEFINITIONS ---------------------------------------------------------------//
// bitvector constructor
void bv_new(bv_t bv, short len);

// bitvector destructor
void bv_free(bv_t bv);

// bitvector functions
void print_vec(bv_t bv);
void clear_vec(bv_t bv);
void append(bv_t bv, char c, short start);
void bit_append(bv_t bv, unsigned char c);
void linear_append(bv_t bv, char c);
void resize(bv_t bv, short size);
void shift_right(bv_t bv, short shift);
void shift_left(bv_t bv, short shift);
void circ_append(bv_t bv, char c);
void interleave(bv_t* bvs, short n, short block_size);
void bitvec_xor(bv_t dest, bv_t src);
void bitvec_and(bv_t dest, bv_t src);
void bitvec_or(bv_t dest, bv_t src);
void load(bv_t bv, int d);
int  unload(bv_t bv);
unsigned char get(bv_t bv, short start, short end);
unsigned short pop(bv_t bv);


// general functions
void printBits(char* const string, int const n, int bitsToRead);
void interleaveBits(char* const string, char* result, const int n, int bitsToRead);

// convolutional encoder
void updateState(unsigned char n);
void encode(bv_t dest, unsigned char bit);
void conv_encode(bv_t dest, bv_t src);
void puncture(bv_t dest, bv_t src);
unsigned short hammingDistance(bv_t bv1, bv_t bv2);


#endif
