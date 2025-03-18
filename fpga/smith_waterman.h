#ifndef _SMITH_WATERMAN_H
#define _SMITH_WATERMAN_H

#define MAX_DIM (30+PADDING_SIZE)
#define SEQ_SIZE (MAX_DIM-PADDING_SIZE)
#define DEPTH_STREAM MAX_DIM
#define INPUT_SIZE 10
#define NO_COUPLES_PER_STREAM (DEPTH_STREAM/(PACK_SEQ*2))*NUM_CU

#define BITS_PER_CHAR 4
#define PORT_WIDTH 512
#define N_ELEM_BLOCK (PORT_WIDTH/BITS_PER_CHAR)
#define UNROLL_FACTOR 8
#define BLOCK_SIZE 32/BITS_PER_CHAR
#define PADDING_SIZE (BLOCK_SIZE)

#define NUM_TMP_WRITE 512

#define PACK_SEQ ((MAX_DIM*BITS_PER_CHAR-1)/PORT_WIDTH+1)
#define N_PACK (INPUT_SIZE*(PACK_SEQ*2+1))

#define NUM_CU 1

const int m_axi_depth=MAX_DIM*(PACK_SEQ*2+1);

#define UP 0
#define UP_LEFT -1
#define LEFT -1

#define INFTY -32768 / 2

typedef struct conf {
	int match;
	int mismatch;
	int gap_opening;
	int gap_extension;
} conf_t;

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;

#endif // _SMITH_WATERMAN_H
