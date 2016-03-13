#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"

#define CODE_LENGTH      2
#define NUM_BITS        8

short state = 0;

// BITVECTOR CONSTRUCTOR --------------------------------------------------------------//
/* void bv_new(bv_t, short)
 *   constructor for the bitvector class
 *
 * bv_t: a pointer to the new bitvector object
 * len:  number of bits to be stored
 *
 * TO USE:
 * bv_t bv = malloc (sizeof(struct bitvec));
 * bv_new(bv, len);
 */
void bv_new(bv_t bv, short len) {
    // find number of chars to allocate
    short temp = len / 8;
    if (len % 8 != 0)
        temp++;

    // set the variables as necessary
    bv->num_bits = 0;
    bv->len      = temp;
    bv->bits     = (unsigned char*) malloc(temp * 8);
}


// BITVECTOR DESTRUCTOR --------------------------------------------------------------//
/* void bv_free(bv_t)
 *   destructor for the bitvector class
 *
 * bv_t: a pointer to the new bitvector object
 *
 * TO USE:
 * bv_free(bv);
 * free(bv);
 */
 void bv_free(bv_t bv) {
    free (bv->bits);
}


// BITVECTOR FUNCTIONS ---------------------------------------------------------------//
/* void print_vec(bv_t)
 * prints out characters bits in little endian, and characters in the order
 * they were placed
 * 
 * bv: a pointer to the bitvector object
 */
void print_vec(bv_t bv) {
    unsigned char* b = (unsigned char*) bv->bits;
    unsigned char  byte;
    int i, j, count = 0, n = bv->len, num_bits = bv->num_bits;

    for (i = 0; i < n; i++) {
        for (j = 7; j >= 0; j--) {
            byte = b[i] & (1<<j);
            byte >>= j;
            if (i*8 + j >= num_bits)
                printf("-");
            else
                printf("%u", byte);

            count++;
        }
        printf(" ");
    }

    printf("\n");
}

/* void clear_vec(bv_t)
 * "clears" the bitvector by setting num_bits to 0
 * 
 * bv: a pointer to the bitvector object
 */
void clear_vec(bv_t bv) {
    bv->num_bits = 0;
}

/* void append(bv_t, char)
 *   adds elements to bitvec with dynamic allocation
 *   add only as many bits as are significant
 *
 * bv:    a pointer to the bitvector object
 * c:     a char containing the bits to be appended
 * start: where the first significant bit is
 */
void append(bv_t bv, char c, short start) {
    unsigned char byte;
    short i;
    short num_bits = bv->num_bits, pos = num_bits/8 + 1;

    // increment the total number of bits in the bitvector
    bv->num_bits += start + 1;

    // append bytes to the bitvector
    for (i = start; i >= 0; i--) {
        if (pos * 8 == num_bits)
            pos++;

        byte = c & (1<<i);
        byte >>= i;

        bv->bits[pos-1] <<= 1;
        bv->bits[pos-1] += byte;
        num_bits++;
    }
}

/* void bit_append(bv_t, char)
 *   adds elements to bitvec with dynamic allocation
 *   add only as many bits as are significant
 *
 * bv: a pointer to the bitvector object
 * c:  a char containing the bits to be appended
 */
void bit_append(bv_t bv, unsigned char c) {
    short num_bits = bv->num_bits;
    
    // just in case we mess up and pass in a value larger than 1
    c %= 2;

    // check if we need to allocate more space
    if (num_bits % 8 == 0)
        resize(bv, num_bits + 1);

    append(bv, c, 0);
}

/* void linear_append(bv_t, char, short)
 *   adds elements to bitvec with dynamic allocation
 *   add only as many bits as are significant
 *
 * bv: a pointer to the bitvector object
 * c:  a char containing the bits to be appended
 */
void linear_append(bv_t bv, char c) {
    short start = 7, len = bv->len, num_bits = bv->num_bits;

    // check if we need to realloc space
    if (start + 1 > len * 8 - num_bits)
        resize(bv, num_bits + start + 1);

    append(bv, c, start);
}

/* void resize(bv_t, short)
 *   resizes the char array to support a different number of bits
 *
 * bv:   a pointer to the bitvector object
 * size: the number of bits that the char array of bv_t should support
 */
void resize(bv_t bv, short size) {    
    // don't realloc space if it already supports that many bits
    if (bv->len * 8 > size)
        return;

    short temp = size / 8;
    if (size % 8 != 0)
        temp++;

    bv->bits = realloc(bv->bits, temp * 8);
    bv->len = temp;
}

/* void shift_right(bv_t, short)
 *   shifts the bits in the array to the right
 *   because we want the most significant bits at the end, this function just
 *   removes the bits
 *
 * bv:    a pointer to the bitvector object
 * shift: number of bytes to shift the bits
 */
void shift_right(bv_t bv, short shift) {
    short num_bits = bv->num_bits,
    	  leftover = num_bits == 0 ? shift - 8 : shift - num_bits,
          len = bv->len;

    if (leftover > 0) {
        bv->bits[len-2] >>= leftover;
    }

    bv->num_bits -= shift;
}

/* void shift_left(bv_t, short)
 *   shifts the bits in the array to the left
 *
 * bv:    a pointer to the bitvector object
 * shift: number of bytes to shift the bits
 */
void shift_left(bv_t bv, short shift) {
    unsigned char overflow_old, overflow_new = 0, mask = 1;
    short i, start;
    short len = bv->len, 
          leftover = (bv->num_bits % 8) - shift;

    mask <<= shift;
    mask -= 1;

    // first one's strange
    if ((bv->num_bits) % 8 != 0 && leftover != 0) {
        overflow_new = overflow_old 
                     = (bv->bits[len-1] & (mask << leftover)) >> leftover;

        bv->bits[len-1] &= (1 << leftover) - 1;

        start = len - 2;
    } else {
        start = len - 1;
    }

    for (i = start; i >= 0; i--) {
        overflow_old = overflow_new;
        overflow_new = bv->bits[i] & mask;

        bv->bits[i] <<= shift;
        bv->bits[i] += overflow_old;
    }

    if (bv->num_bits > shift) {
        bv->num_bits -= shift;
    } else {
        bv->num_bits = 0;
    }
}

/* void circ_append(bv_t, c)
 *   appends as many bits as possible before shifting to append
 *
 * bv: a pointer to the bitvector object
 * c:  the char to append
 */
void circ_append(bv_t bv, char c) {
    short shift;
    short len = bv->len, num_bits = bv->num_bits;

    // check to see how much we need to shift the bitvector
    // then shift the bit vector
    shift = len * 8 - num_bits;
    printf("shift: %d \n", shift);
    if (shift > 0)
        shift_left(bv, shift);

    // do an append now that we have enough space
    append(bv, c, 7);
}

/* void interleave(bv_t*, short, short)
 *   interleaves 2 bitvectors together
 *
 * bv: a pointer to the bitvector object
 * n:  
 * block_size: size of how long the block should be
 */
void interleave(bv_t* bvs, short n, short block_size);

/* void bitvec_xor(bv_t, bv_t)
 *   xor's the two bitvectors, putting the result into dest
 *
 * dest: a pointer to the bitvector object, where the result will be stored
 * src:  a pointer to the second bitvetor object, with which dest will be xor-ed
 */
void bitvec_xor(bv_t dest, bv_t src) {
    short i, difference;
    short d_len = dest->len, s_len = src->len;

    // in case one length is larger than the other
    difference = d_len - s_len;

    if (d_len > s_len) {
        for (i = s_len-1; i >= 0; i--) {
            dest->bits[i+difference] ^= src->bits[i];
        }
    } else {
        for (i = d_len - 1; i >= 0; i--) {
            dest->bits[i] ^= src->bits[i-difference];
        }
    }
}

/* void bitvec_and(bv_t, bv_t)
 *   ands the two bitvectors, putting the result into dest
 *
 * dest: a pointer to the bitvector object, where the result will be stored
 * src:  a pointer to the second bitvetor object, with which dest will be xor-ed
 */
void bitvec_and(bv_t dest, bv_t src) {
    short i, difference;
    short d_len = dest->len, s_len = src->len;

    // in case one length is larger than the other
    difference = d_len - s_len;

    if (d_len > s_len) {
        for (i = s_len-1; i >= 0; i--) {
            dest->bits[i+difference] &= src->bits[i];
        }
    } else {
        for (i = d_len - 1; i >= 0; i--) {
            dest->bits[i] &= src->bits[i-difference];
        }
    }
}

/* void bitvec_or(bv_t, bv_t)
 *   ors the two bitvectors, putting the result into dest
 *
 * dest: a pointer to the bitvector object, where the result will be stored
 * src:  a pointer to the second bitvetor object, with which dest will be xor-ed
 */
void bitvec_or(bv_t dest, bv_t src) {
    short i, difference;
    short d_len = dest->len, s_len = src->len;

    // in case one length is larger than the other
    difference = d_len - s_len;

    if (d_len > s_len) {
        for (i = s_len-1; i >= 0; i--) {
            dest->bits[i+difference] |= src->bits[i];
        }
    } else {
        for (i = d_len - 1; i >= 0; i--) {
            dest->bits[i] |= src->bits[i-difference];
        }
    }
}

/* void load(bv_t, int f)
 *   loads the bits from an int into the bitvector
 *
 * bv: a pointer to the bitvector object
 * f:  the integer to load
 */
void load(bv_t bv, int d) {
    int mask = 0xFF;
    unsigned char c;

    //resize(bv, 32);
    unsigned char* bits = bv->bits;

    clear_vec(bv);
    c = (unsigned char) (d >> 24);
    //bits[0] = c;
    linear_append(bv,c);

    c = (unsigned char) (d >> 16) & mask;
    //bits[1] = c;
    linear_append(bv,c);

    c = (unsigned char) (d >> 8) & mask;
    //bits[2] = c;
    linear_append(bv,c);

    c = (unsigned char) (d & mask);
    //bits[3] = c;
    linear_append(bv,c);


}

/* int unload(bv_t)
 *   grabs 32 bits from the bitvector
 *
 * bv:    a pointer to the bitvector object
 */
int unload(bv_t bv) {
    int d = 0;

    unsigned char* bits = bv->bits;

    d += bits[0]  << 24;
    d += bits[1] << 16;
    d += bits[2] << 8;
    d += bits[3];

    return d;
}

/* unsigned short get(bv_t, short, short)
 *   gets the bits between the start and end as requested
 *
 * bv:    a pointer to the bitvector object
 * start: position at which to start reading
 * end:   position at which to end reading
 */
unsigned char get(bv_t bv, short start, short end) {
    unsigned char byte;
    unsigned char* bits = bv->bits;
    short a, b;
    short l_pos = bv->len - 1, num_bits = bv->num_bits,
          leftover = (8 - (num_bits % 8)) % 8;

    // make sure that start + end don't cause out of bound errors
    if (start < 0 || end > num_bits)
        return 0;

    // make sure that the range to get is at most 16 bits
    if (end - start > 15)
        return 0;

    // must've input start and end in the wrong order
    // therefore, get bits from end->start by flipping the order
    if (end < start) {
        short temp = end;
        end = start;
        start = temp;
    }

    short len = end-start;
    short c_1 = start/8;
    short c_2 = end/8;
    a = (start % 8);
    b = (end % 8);

    // if the requested bits overlap two chars
    if(c_1 != c_2) {
    	byte = (bits[c_1] >> a);
    	byte += (bits[c_2] << (8-a)) & ((1 << (len+1)) - 1);

    	return byte;
    } else {
    	if (c_1 == l_pos) {
    		a -= leftover;
    		b -= leftover;
    	}
    	byte = bits[c_1] >> a;
    }

    return byte & ((1 << (len+1)) - 1);
}

/* unsigned short pop(bv_t)
 *   pops the last character off the bitvector
 *
 * bv:    a pointer to the bitvector object
 */
unsigned short pop(bv_t bv) {
    short num_bits = bv->num_bits,
          leftover = (num_bits - 1) % 8;

    unsigned short r = bv->bits[num_bits/8];
    r &= (1 << leftover);
    r >>= leftover;
    shift_right(bv, 1);

    return r;
}


// CONVOLUTIONAL CODE FUNCTIONS ------------------------------------------------------//
/* void updateState(unsigned char)
 *   updates the state based on the current bit
 *
 * n can take the values:
 *    0             update state with a 0
 *    1             update state with a 1
 *    otherwise     state will be set to 0
 */
void updateState(unsigned char n) {
    if (n != 0 && n != 1) {
        state = 0;
        return;
    }

    state >>= (CODE_LENGTH - 1);
    state += (n << (CODE_LENGTH - 1));
}

/* void encode(bv_t, unsigned char)
 *   helper function to compute the convolutional code
 *
 * dest: bitvector that we append to
 * bit:  bit that we should add
 */
void encode(bv_t dest, unsigned char bit) {
    unsigned char u = bit;
    short i, b;

    // just in case we pass in bit > 1
    bit %= 2;

    // calculate the first parity bit
    // p0 = u + x[1] + x[2] + ... + x[CODE_LENGTH-1]
    for (i = CODE_LENGTH - 1; i >= CODE_LENGTH-2; i--) {
        b = (state & (1 << i)) >> i;
        u += b;
    }

    // add the first parity bit to the destination
    u %= 2;
    bit_append(dest, u);

    // calculate the rest of the parity bits
    // p[n] = u + x[n]
    for (i = 0; i < CODE_LENGTH - 1; i++) {
        b = (state & (1 << i)) >> i;
        u = (bit + b) % 2;

        bit_append(dest, u);
    }
}

/* void conv_encode(bv_t, bv_t)
 *   adds parity bits to the code
 *
 * dest: location of the bitvector with parity bits added
 *       space to be allocated in parent function
 * src:  original bitvector without the parity bits
 */
void conv_encode(bv_t dest, bv_t src) {
    unsigned char byte;
    short i, j;
    short len = src->len, num_bits = src->num_bits,
          leftover = 8 + num_bits - (len * 8), start = 7;

    // clear the destination to make sure that it's empty when we start adding
    clear_vec(dest);

    // loop through each of the chars in the chars array
    for (i = 0; i < len; i++) {
        if (i == len-1)
            start = leftover - 1;

        // loop through each of the bits in the bit array
        for (j = start; j >= 0; j--) {
            byte = src->bits[i] & (1<<j);
            byte >>= j;

            encode(dest, byte);
            updateState(byte);
        }
    }
}

/* void puncture(bv_t, bv_t)
 *   punctures the code
 *
 * dest: bitvector to place the punctured code in
 * src:  bitvector containing the encoded bits
 */
void puncture(bv_t dest, bv_t src) {
    unsigned char byte;
    short i, j, count = 0;
    short len = src->len, num_bits = src->num_bits,
          leftover = 8 + num_bits - (len * 8), start = 7;

    // clear the destination to make sure that it's empty when we start adding
    clear_vec(dest);

    /* do things based on state
     * for RATE = 2 and RATE = 3, the matrices act essentially the same way:
     *
     * for RATE = 2:
     *     the matrix is:
     *          1   0
     *          1   1
     *     where the 3rd bit is taken out
     *
     * for RATE = 3:
     *     the matrix is:
     *          1   0   1
     *          1   1   0
     *     where the 3rd bit & every 6th bit is taken out
     *
     * these essentially lead up to the same result, so i'm not going to distinguish
     * between the two
     */
    for (i = 0; i < len; i++) {
        if (i == len-1)
            start = leftover - 1;

        // loop through each of the bits in the bit array
        for (j = start; j >= 0; j--) {
            // increment the count
            count++;

            // append only when it's not the 3rd bit (for every 4 bits)
            if (count % 4 != 3) {
                byte = src->bits[i] & (1<<j);
                byte >>= j;

                bit_append(dest, byte);
            }
        }
    }
}

/* short hammingDistance(bv_t, bv_t)
 *   calculates the hamming distance between the two bitvectors
 */
unsigned short hammingDistance(bv_t bv1, bv_t bv2) {
    short i, j, leftover;
    short num_bits = bv1->num_bits, len = bv1->len, start = 7;
    unsigned short count = 0;

    // create a temporary bitvector to use for the xor (so we don't overwrite info)
    bv_t temp = malloc (sizeof(struct bitvec));
    bv_new(temp, num_bits);

    // copy all the data to the temporary bitvector
    for (i = 0; i < len; i++) {
        temp->bits[i] = bv1->bits[i];
    }
    temp->num_bits = num_bits;
    temp->len = bv1->len;

    // xor it with the second bitvector
    bitvec_xor(temp, bv2);

    // set variables for the for loop
    len = temp->len;
    leftover = 8 + temp->num_bits - (len * 8);

    // sum the bits
    for (i = 0; i < len; i++) {
        if (i == len-1)
            start = leftover - 1;

        for (j = start; j >= 0; j--) {
            count += (temp->bits[i] >> j) & 1;
        }
    }

    // free the temporary bitvector
    bv_free(temp);
    free(temp);

    return count;
}


void printBits(char* const string, int const n, int bitsToRead) {
    unsigned char* b = (unsigned char*) string;
    unsigned char  byte;
    int i, j;

    // don't try to read more bits than you can
    if (bitsToRead > 8 || bitsToRead < 1) bitsToRead = 8;

    for (i = 0; i < n; i++) {
        for (j = bitsToRead - 1; j >= 0; j--) {
            byte = b[i] & (1<<j);
            byte >>= j;
            printf("%u", byte);
        }
        printf(" ");
    }

    printf("\n");
}

void interleaveBits(char* const string, char* result, const int n, int bitsToRead) {
    int i, j;
    int count = 0, num_bits = 0;
    char temp = 0;
    unsigned short byte;

    // don't try to read more bits than you can
    if (bitsToRead > 8 || bitsToRead < 1) bitsToRead = 8;

    // interleave the bits
    // results in big endian 
    for (j = bitsToRead - 1; j >= 0; j--) {
        for (i = 0; i < n; i++) {
            // get the last bit
            byte = string[i] & (1<<j);
            byte >>= j;

            // left shift temp by 1, then add the next bit
            temp <<= 1;
            temp += byte;

            num_bits++;

            // store the number in the result buffer and reset temp
            if (num_bits > bitsToRead - 1) {
                result[count] = temp;
                temp = 0;
                count++;
                num_bits = 0;
            }
        }
    }
}
