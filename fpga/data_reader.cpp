/******************************************
 *MIT License
 *
 *Copyright (c) [2025]
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
 *FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *SOFTWARE.
 ******************************************/

#include <iostream>
#include <vector>
#include <limits>
#include <string.h>
#include <ap_int.h>
#include "../common/common.h"
#include "hls_stream.h"

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;

typedef struct conf {
	int match;
	int mismatch;
	int gap_opening;
} conf_t;

const unsigned int depth_stream = DEPTH_STREAM;
const unsigned int no_couples_per_stream = NO_COUPLES_PER_STREAM;
const unsigned int unroll_f = 4;
const unsigned int num_pack = (PACK_SEQ << 1) + 1;

//the problem is here i think....
void computeSW(hls::stream<input_t> &reads_stream, hls::stream<ap_int<sizeof(int32_t) * 8 * 4>>& s, conf_t scoring) {
	ap_int<sizeof(int32_t)*8*4> tmp;

	#pragma HLS UNROLL factor=unroll_f
	for (int i = 0; i < SEQ_SIZE / 2; i++) {
		tmp.range(31,0) = reads_stream.read();
		tmp.range(63,32) = reads_stream.read();
		tmp.range(95,64) = reads_stream.read();
		tmp.range(127,96) = reads_stream.read();

		s.write(tmp);
	}
}

void read_input_data(input_t *input, hls::stream<input_t> &input_stream, int n, int num_couples) {

#pragma HLS INLINE off

	int iter;

	iter = (num_couples - (n)) < NO_COUPLES_PER_STREAM ?
			(num_couples - (n)) * ((PACK_SEQ << 1) + 1) :
			NO_COUPLES_PER_STREAM * ((PACK_SEQ << 1) + 1);

	for (int i = 0; i < iter; i++) {

		input_t tmp = input[n * ((PACK_SEQ << 1) + 1) + i];
		input_stream.write(tmp);
	}
}

void read_input_data_wrapper(input_t *input,
		hls::stream<input_t> &input_stream, int num_couples) {

	loop_read_input_data_wrapper: for (int n = 0; n < num_couples; n += NO_COUPLES_PER_STREAM)
#pragma HLS PIPELINE off
		read_input_data(input, input_stream, n, num_couples);

}

void dispatcher(hls::stream<input_t> &input_stream,
		hls::stream<input_t> &reads_stream, int num_couples) {

	int idx = 0;

	loop_dispatcher: for (int i = 0; i < num_couples; i++) {

		loop_dispatcher_inner: for (int j = 0; j < ((PACK_SEQ << 1) + 1); j++) {
#pragma HLS PIPELINE
			input_t tmp = input_stream.read();
			reads_stream.write(tmp);
		}
	}
}

void compute_wrapper(hls::stream<input_t> &reads_stream,
		hls::stream<ap_int<sizeof(int32_t) * 8 * 4>>& s, int num_couples, conf_t scoring) {

	int num_iter = num_couples;

	loop_compute_wrapper: for (int n = 0; n < num_iter; n++) {
#pragma HLS PIPELINE off
		computeSW(reads_stream, s, scoring);
	}
}

void alignment(input_t *input, int num_couples, conf_t scoring,
		hls::stream<input_t> &input_stream,
		hls::stream<ap_int<sizeof(int32_t) * 8 * 4>>& s,
		hls::stream<input_t> &reads_stream) {

#pragma HLS INLINE

	int tot_couples = num_couples;
	read_input_data_wrapper(input, input_stream, tot_couples);
	dispatcher(input_stream, reads_stream, tot_couples);
	compute_wrapper(reads_stream, s, tot_couples, scoring);
}


extern "C" {
    void data_reader(input_t *input,  conf_t scoring, int num_couples, hls::stream<ap_int<sizeof(int32_t) * 8 * 4>>& s) {
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem0 depth=m_axi_depth

#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=scoring bundle=control
#pragma HLS INTERFACE s_axilite port=num_couples bundle=control
#pragma HLS interface axis port=s

#pragma HLS DATAFLOW

	static hls::stream<input_t> input_stream("input_stream");
#pragma HLS STREAM variable=input_stream depth=no_couples_per_stream dim=1

	static hls::stream<input_t> reads_stream;
#pragma HLS STREAM variable=reads_stream depth=depth_stream dim=1
#pragma HLS BIND_STORAGE variable=reads_stream type=fifo impl=bram

	int tot_couples = num_couples;

	alignment(input, tot_couples, scoring, input_stream, s, reads_stream);
    }
}
