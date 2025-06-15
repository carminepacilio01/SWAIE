/******************************************
*MIT License
*
# *Copyright (c) Carmine Pacilio [2025]
*
*Permission is hereby granted, free of charge, to any person obtaining a copy
*of this software and associated documentation files (the "Software"), to deal
*in the Software without restriction, including without limitation the rights
*to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*copies of the Software, and to permit persons to whom the Software is
*furnished to do so, subject to the following conditions:
*
*The above copyright notice and this permission notice shall be included in all
*copies or substantial portions of the Software.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*SOFTWARE.
******************************************/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_DIM (150+PADDING_SIZE)
#define SEQ_SIZE (MAX_DIM-PADDING_SIZE)
#define DEPTH_STREAM MAX_DIM
#define INPUT_SIZE 1000
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

#define MATCH 1
#define MISMATCH -1
#define GAP_OPENING -2

#endif // CONSTANTS_H
