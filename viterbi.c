#include <stdio.h>
#include <stdlib.h>
#include "bit_vector.h"
#include "conv_encoder.h"
#include "viterbi.h"

short puncturedRec = TRUE;
short isOddRec = FALSE;

short bm[NUM_CODES];
int   pm[NUM_STATES] = {0};
bv_t  paths[NUM_STATES];

int pm_next[NUM_STATES];
bv_t paths_next[NUM_STATES];

short HammingDistance[NUM_CODES][NUM_CODES];
short Transitions[NUM_STATES][2];
short InverseTransitions[NUM_STATES][2];
short Trellis[NUM_STATES][NUM_STATES] = {-1};
short Decode[NUM_STATES][NUM_STATES] = {-1};

/** void setupDecoder(void)
/  Uses the current Convolutional Encoder to generate
/  the appropriate tables for an efficient Viterbi decoder
/  Should be called in main() before interrupt initialized
*/
void setupDecoder() {

	setPuncturing(FALSE);

	int i;
	int j;
	bv_t a = malloc (sizeof(struct bitvec));
	bv_new(a, 1);
	bv_t b = malloc (sizeof(struct bitvec));
	bv_new(b, 1);

	//*** this finds the properties of each state
	//generate the trellis (messages produced by state transitions)
	//generate decoding given two states
	for(i = 0; i < NUM_STATES; i++) {
		  for(j = 0; j < NUM_STATES; j++) {
			Trellis[i][j] = -1;
			Decode[i][j] = -1;
			if(j < 2)
				InverseTransitions[i][j] = -1;
		  }
	}
	// Trellis[0][0] = Trellis[1][2] = 0;
	// Trellis[1][0] = Trellis[0][2] = 3;
	// Trellis[2][1] = Trellis[3][3] = 1;
	// Trellis[3][1] = Trellis[2][3] = 2;

	for(i = 0; i < NUM_STATES; i++) {
		clear_vec(a);
		clear_vec(b);

		//get message and next state for input 0
		setState(i);
		encode(a,0);
		Transitions[i][0] = getState();
		if(InverseTransitions[getState()][0] == -1)
			InverseTransitions[getState()][0] = i;
		else
			InverseTransitions[getState()][1] = i;
		Trellis[i][getState()] = get(a,0,1);
		Decode[i][getState()] = 0;

		//get message and next state for input 1
		setState(i);
		encode(b,1);
		Transitions[i][1] = getState();
		if(InverseTransitions[getState()][0] == -1)
			InverseTransitions[getState()][0] = i;
		else
			InverseTransitions[getState()][1] = i;
		Trellis[i][getState()] = get(b,0,1);
		Decode[i][getState()] = 1;

		//initialize the decoded survivor paths
		paths[i] = malloc(sizeof(struct bitvec));
		bv_new(paths[i], 32);
		paths_next[i] = malloc (sizeof(struct bitvec));
		bv_new(paths_next[i], 32);

	}

	// for(i = 0; i < NUM_CODES; i++) {
	// 	  for(j = 0; j < 2; j++) {
	// 		printf("%d: %d -> %d \n", Decode[i][Transitions[i][j]], i, Transitions[i][j]);
	// 	  }
	// }
	// for(i = 0; i < NUM_CODES; i++) {
	// 	  for(j = 0; j < 2; j++) {
	// 		  printf("%d: %d -> %d \n", Decode[InverseTransitions[i][j]][i], InverseTransitions[i][j], i);
	// 	  }
	// }
	for(i = 0; i < NUM_CODES; i++) {
		for(j = 0; j < NUM_CODES; j++) {
		  printf("%d ", Trellis[i][j]);
		}

		printf("\n");
	}

	printf("Hamming distance \n");
	//pre-generate hamming distances for all messages
	for(i = 0; i < NUM_CODES; i++) {
		  load(a,i);
		  for(j = 0; j < NUM_CODES; j++) {
		    load(b,j);
		    HammingDistance[i][j] = hammingDistance(a,b);
		    printf("%d ", HammingDistance[i][j]);
		  }
		  printf("\n");
	}
	
	//reset the encoder and decoder
	clearState();
	vit_dec_reset();

	bv_free(a);
	bv_free(b);
	free(a);
	free(b);

	//set puncturing (only accomodates 2/3 puncture)
	setPuncturing(puncturedRec);
}

/*  void vit_dec_bmh(short)
 *   Hard decision branch metrics
 *   Generate branch metrics based on Hamming Distances
 *   accomodates 2/3 rate puncturing if needed
 */
void vit_dec_bmh(short m_hat) {
	short i;
	//generate branch metrics
	if(puncturedRec && isOddRec) {
//	  for(i = 0; i < NUM_CODES; i++) {
//		  bm[i] = ((i >> 1) % 2) ^ (m_hat % 2);
//		  printf("bm%d: %d \n",i,bm[i]);
//	  }
		bm[1] = bm[0] = (m_hat%2); //((i >> 1) % 2) ^ (m_hat % 2);
		bm[3] = bm[2] = !(m_hat%2);
	}
	else {
	  // for(i = 0; i < NUM_CODES; i++) {
		 //  bm[i] = HammingDistance[m_hat][i];
		 //  printf("bm%d: %d \n",i,bm[i]);
	  // }
		bm[0] = HammingDistance[m_hat][0];
		bm[1] = HammingDistance[m_hat][1];
		bm[2] = HammingDistance[m_hat][2];
		bm[3] = HammingDistance[m_hat][3];
	}
		  for(i = 0; i < NUM_CODES; i++) {
			  //printf("bm%d: %d \n",i,bm[i]);
		  }
	isOddRec = !isOddRec;
}

/* void vit_dec_bms(short, float)
 * Soft decision branch metrics
 * - Generate branch metrics base on Euclidean Distances
 * Incomplete!
 */
void vit_dec_bms(short m_hat, float pr){

}

/* vit_dec_ACS(short)
 *  Add Compare Select Step
 * - Given branch metrics (based on hard or soft decoding)
 * - Find path metrics and survivor paths
 * - Must be called for each state but split up for interrupt compatibility
 */
void vit_dec_ACS(short i) {
  //short i;

  //generate path metrics using ACS



  //for(i = 0; i < NUM_STATES; i++) {h)
	  //get candidates
	  short index1 = InverseTransitions[i][0];
	  int path1 = pm[index1] + bm[Trellis[index1][i]];
	  short index2 = InverseTransitions[i][1];
	  int path2 = pm[index2] + bm[Trellis[index2][i]];
	  //printf("i=%d with index1: %d, Decode: %d, pm: %d, bm: %d, path1: %d \n", i,
	  //	index1, Decode[index1][i], pm[index1], bm[Trellis[index1][i]], path1);
	 // printf("i=%d with index2: %d, Decode: %d, pm: %d, bm: %d, path2: %d \n", i,
	  //	index2, Decode[index2][i], pm[index2], bm[Trellis[index2][i]], path2);

	  short decoded_bit;

	  //compare and select, update survivor paths
	  if(path1 <= path2) {
		  decoded_bit = Decode[index1][i];
		  pm_next[i] = path1;
		  copy_vec(paths_next[i], paths[index1]);
	  }
	  else {
		  decoded_bit = Decode[index2][i];
		  pm_next[i] = path2;
		  copy_vec(paths_next[i], paths[index2]);
	  }
	  bit_append(paths_next[i], decoded_bit);
	  //print_vec(paths_next[i]);
  //}
	  //update_paths();
}

/* void update_paths(void)
 *  Used after completing calls to vit_dec_ACS
 *  -hardcoded with unrolled loop for 4 state encoder
 *  -only part that needs to be manually adjusted for other encoders
 */
void update_paths(/*short i*/) {
	//short i;
 /* for(i = 0; i < NUM_STATES; i++) {
	  pm[i] = pm_next[i];
	  paths[i] = paths_next[i];
  }*/
	pm[0] = pm_next[0];
	pm[1] = pm_next[1];
	pm[2] = pm_next[2];
	pm[3] = pm_next[3];

	copy_vec(paths[0], paths_next[0]);
	copy_vec(paths[1], paths_next[1]);
	copy_vec(paths[2], paths_next[2]);
	copy_vec(paths[3], paths_next[3]);
}

/* void vit_dec_get(bv_t)
 *  Returns the minimum weight path so far
 *  -Note that an O(n) search is here where an O(log(n)) would be better
 *  -This function should be updated for better efficiency with more states
 */
void vit_dec_get(bv_t dest) {
	int min_index = 0;
	int min_value = pm[0];
	int i;

	for(i = 1; i < NUM_STATES; i++) {
		if(pm[i] < min_value) {
			min_value = pm[i];
			min_index = i;
		}
	}

	copy_vec(dest, paths[min_index]);

}


//Testing Function
void vit_dec(bv_t dest, bv_t src) {
	//printf("%d\n", Trellis[0][2]);

  clear_vec(dest);

  short i;
  short num_bits = src->num_bits;

  vit_dec_reset();

  // for each bit in src, find the ML set of paths ending at each state
  i = num_bits-1;
  while(i >= 0) {
	  if(puncturedRec && isOddRec) {
		  //printf("m = %d\n", get_bit(src,i));
		  vit_dec_bmh(get_bit(src,i));
		  i--;
	  }
	  else {
		  //printf("m = %d\n", get(src,i-1,i));
		  vit_dec_bmh(get(src,i-1,i));
		  i -= 2;
	  }
	  //vit_dec_ACS();
  }

  //choose ML path based on node with smallest accumulated pm
  vit_dec_get(dest);

}

/* void vit_dec_reset(void)
 *  Resets the decoder for new message
 *  -another part that is hardcoded so needs to be changed for more states
 *  -sets up decoder with assumption that initial state is 0,0
 */
void vit_dec_reset() {
	isOddRec = FALSE;
//	int i;
//	for(i = 0; i < NUM_STATES; i++) {
//		pm[i] = NUM_STATES+1;
//		clear_vec(paths[i]);
//	}
	pm[0] = 0;
	pm[1] = NUM_STATES+1;
	pm[2] = NUM_STATES+1;
	pm[3] = NUM_STATES+1;
	clear_vec(paths[0]);
	clear_vec(paths[1]);
	clear_vec(paths[2]);
	clear_vec(paths[3]);
}

