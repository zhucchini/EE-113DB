#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"
#include <limits.h>

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
/* void copy_vec(bv_t, bv_t)
 * makes a copy of the src bitvector and places it in dest
 *
 * bv: a pointer to the bitvector object
 */
void copy_vec(bv_t dest, bv_t src) {
    unsigned short i, len = src->len;
    unsigned char* bits = src->bits;

    resize(dest, src->num_bits);

    // copy all the data bitvector
    for (i = 0; i < len; i++) {
        dest->bits[i] = bits[i];
    }
    dest->num_bits = src->num_bits;
    dest->len = len;
}

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
    resize(bv,1);
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

/* void bit_append(bv_t, unsigned char)
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

/* void set_bit(bv_t, short, unsigned short)
 *   sets the bit at pos to a certain value
 *
 * bv:  a pointer to the bitvector object
 * pos: pos for where to set the bit
 * c:   a char containing the bit to append
 */
void set_bit(bv_t bv, short pos, unsigned short c) {
    unsigned char byte;
    short l_pos = pos / 8, b_pos = pos % 8;

    // get the byte containing the bit to change
    byte = bv->bits[l_pos];

    // set the desired bit to the desire value
    byte ^= (-c ^ byte) & (1 << b_pos);

    // set the byte
    bv->bits[l_pos] = byte;
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
    short num_bits = bv->num_bits % 8,
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

    resize(bv, 32);
    //unsigned char* bits = bv->bits;

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

int unload2(bv_t bv) {
	int d = 0;

	unsigned char* bits = bv->bits;

	d += bits[4]  << 24;
	d += bits[5] << 16;
	d += bits[6] << 8;
	d += (bits[7] & 0x0F) << 4;

	return d;
}

int vit_unload(bv_t bv) {
	int d = 0;

	unsigned char* bits = bv->bits;

	d += bits[4]  << 24;
	d += bits[3] << 16;
	d += bits[2] << 8;
	d += bits[1];

	return d;
}
/* unsigned char get(bv_t, short, short)
 *   gets the bits between the start and end as requested
 *
 * bv:    a pointer to the bitvector object
 * start: position at which to start reading
 * end:   position at which to end reading
 */
unsigned char get(bv_t bv, short start, short end) {
    unsigned char byte;
    unsigned char* bits = bv->bits;
    short a;
    short num_bits = bv->num_bits;

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

    // if the requested bits overlap two chars
    if(c_1 != c_2) {
        byte = (bits[c_1] >> a);
        byte += (bits[c_2] << (8-a)) & ((1 << (len+1)) - 1);

        return byte;
    } else {
        byte = bits[c_1];
        byte >>= a;
    }

    return byte & ((1 << (len+1)) - 1);
}

/* unsigned short get_bit(bv_t, short)
 *   gets the bit at pos
 *
 * bv:  a pointer to the bitvector object
 * pos: position of the bit to get
 */
unsigned char get_bit(bv_t bv, short pos) {
    unsigned char byte;
    short l_pos = pos / 8, b_pos = pos % 8;

    // get the byte containing the desired bit
    byte = bv->bits[l_pos];

    // get the desired bit
    byte >>= b_pos;
    byte &=  1;

    return byte;
}

/* unsigned short pop(bv_t)
 *   pops the last character off the bitvector
 *
 * bv:    a pointer to the bitvector object
 */
unsigned short pop(bv_t bv) {
    short num_bits = bv->num_bits - 1,
          leftover = (num_bits) % 8;

    unsigned short r = bv->bits[num_bits/8];
    r &= (1 << leftover);
    r >>= leftover;
    shift_right(bv, 1);

    return r;
}





// MAIN AND OTHER FUNCTIONS ----------------------------------------------------------//
// int main() {
//     /*
//     bv_t p_block = malloc (sizeof(struct bitvec));
//     bv_t e_block = malloc (sizeof(struct bitvec));
//     bv_t rs_block = malloc (sizeof(struct bitvec));
//     bv_t c_block = malloc (sizeof(struct bitvec));

//     bv_new(p_block, 32);
//     bv_new(e_block, 1);
//     bv_new(rs_block, 1);
//     bv_new(c_block, 1);

//     int test1 = (0xE9 << 8) + 0xC7;
//     load (p_block,test1);
//     printf("%d : %d \n", test1, unload(p_block));
//     print_vec(p_block);

//     unsigned char test2 = get(p_block,16,21);
//     printf("start: 16, end: 21, result: ");
//     printBits(&test2, 1, 8);

//     test2 = get(p_block,25,31);
//     printf("start: 25, end: 31, result: ");
//     printBits(&test2, 1, 8);

//     test2 = get(p_block,22,26);
//     printf("start: 22, end: 26, result: ");
//     printBits(&test2, 1, 8);

//     bv_free(p_block);
//     free(p_block);

//     bv_free(e_block);
//     free(e_block);

//     bv_free(rs_block);
//     free(rs_block);

//     bv_free(c_block);
//     free(c_block);

//     unsigned int temp = 0x7fffffff;
//     printf("test: %d  \n", temp);

//     unsigned int temp2 = temp*(120.0/MAX_SPEED);
//     printf("test2: %d \n", temp2);

//     temp2 = temp*(32.4/MAX_SPEED);
//     printf("test2: %d \n", temp2);

//     float temp3 = (float)temp2/(float)temp*MAX_SPEED;
//     printf("test3: %f \n", temp3);
//     */


//     bv_t bv = malloc (sizeof(struct bitvec));
//     bv_new(bv, 8);

//     bv_t dest = malloc (sizeof(struct bitvec));
//     bv_new(dest, 8);

//     // // TEST: linear_append + print_vec
//     // linear_append(bv, 0x2);
//     // linear_append(bv, 'p');
//     // linear_append(bv, 'p');
//     // linear_append(bv, 'l');
//     load(bv, 0x345);
//     printf("bv: ");
//     print_vec(bv);

//     // add_CRC(bv, bv->num_bits);
//     // printf("result: ");
//     // print_vec(bv);

//     // // TEST: pop
//     unsigned short r;
//     for (int i = 0; i < 32; i++) {
//         r = pop(bv);
//         printf("r: %d, ", r);
//         print_vec(bv);
//     }

//     // TEST: conv_encode
//     // conv_encode(dest, bv);
//     // print_vec(dest);

//     // // TEST: get
//     // unsigned char temp = get(bv, 2, 6);
//     // printBits(&temp, 1, 8);

//     // // TEST: shift_right
//     // shift_right(bv, 10);
//     // print_vec(bv);

//     // // TEST: circ_append
//     // circ_append(bv, 'm');
//     // print_vec(bv);

//     // // TEST: load
//     // load(bv, 0x74F);
//     // print_vec(bv);

//     // linear_append(dest, 'l');
//     // linear_append(dest, 'p');
//     // linear_append(dest, 'p');
//     // printf("dest: ");
//     // print_vec(dest);

//     // bitvec_xor(dest, bv);
//     // printf("dest ^ bv: ");
//     // print_vec(dest);

//     // int temp = hammingDistance(bv, dest);
//     // printf("hamming distance: %d \n", temp);

//     // int load = unload(bv);
//     // printf("load: %d \n", load);

//     bv_free(bv);
//     free(bv);

//     bv_free(dest);
//     free(dest);
// }

/* void add_CRC(bv_t, short)
 * 	appends the CRC-8-SAE-J1850 ED code
 * 	to data of size int32 or less
 */
void add_CRC(bv_t bv, short len) {
    unsigned int CRC_8 = 285;
    unsigned long remainder = unload(bv);
    short i;

    remainder <<= 8;

    for (i = len-1; i >= 0; i--) {
        if ((remainder >> (i+8))%2 == 1) {
            remainder ^= CRC_8 << (i);
        }
    }
    unsigned char r = 0xFF & remainder;
    linear_append(bv, r);
}

/* short add_CRC(bv_t, short)
 * 	checks the CRC-8-SAE-J1850 ED code
 * 	returns 1 (TRUE) or 0 (FALSE)
 */
short check_CRC(bv_t bv, short len) {
	char CRC = bv->bits[0];

    unsigned int CRC_8 = 285;
    unsigned long remainder = vit_unload(bv);
    short i;

    remainder <<= 8;

    for (i = len-1; i >= 0; i--) {
        if ((remainder >> (i+8))%2 == 1) {
            remainder ^= CRC_8 << (i);
        }
    }
    unsigned char r = 0xFF & remainder;

    return (CRC == r);
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
    copy_vec(temp, bv1);

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
