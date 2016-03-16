#ifndef DES_ENCRYPT_H
#define DES_ENCRYPT_H



// DES tools
void generateKeys(int lkey, int rkey);
void feistel_sub(bv_t bv, short round);
void feistel_perm(bv_t bv);
void feistel_round(bv_t left, bv_t right, short round);

#endif



