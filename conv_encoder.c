#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"
#include "conv_encoder.h"

short state = 0;
short isOdd = FALSE;
short punctured = FALSE;

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

/* void clearState()
 * state will be set to 0
 */
void clearState() {
	state = 0;
}

/* void setState(short)
 * state will be set to s
 */
void setState(short s) {
	state = s;
}

/* void getState(short)
 * returns state
 */
short getState() {
	return state;
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

		if(!punctured || !isOdd)
			bit_append(dest, u);
	}


    isOdd = !isOdd;
    updateState(bit);
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
    isOdd = FALSE;

    // loop through each of the chars in the chars array
    for (i = 0; i < len; i++) {
        if (i == len-1)
            start = leftover - 1;

        // loop through each of the bits in the bit array
        for (j = start; j >= 0; j--) {
            byte = src->bits[i] & (1<<j);
            byte >>= j;

            encode(dest, byte);
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

