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
 *IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
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

const unsigned int depth_stream = DEPTH_STREAM;
const unsigned int no_couples_per_stream = NO_COUPLES_PER_STREAM;
const unsigned int unroll_f = 2;
const unsigned int num_pack = (PACK_SEQ << 1) + 1;

void write_score(hls::stream<int> &final_score_stream, int n,
    input_t *output, int num_couples, int &to_send) {

#pragma HLS INLINE off
    static int tmp[NUM_TMP_WRITE];
#pragma HLS BIND_STORAGE variable=tmp type=ram_1p impl=bram

    tmp[n & (NUM_TMP_WRITE - 1)] = final_score_stream.read();

    if ((n > 0 && (((n + 1) & (NUM_TMP_WRITE - 1)) == 0)) || n == num_couples - 1) {
        int iter = (to_send >= NUM_TMP_WRITE) ? NUM_TMP_WRITE : to_send;

        for (int i = 0; i < iter; i++) {
#pragma HLS pipeline
            output[n + i + 1 - iter] = tmp[i];
        }
        to_send -= iter;
    } 
}

void write_score_wrapper(hls::stream<int> &score_local_stream, int num_couples,
    input_t *output) {
    int to_send = num_couples;

    loop_write_score_wrapper: for (int n = 0; n < num_couples; n++) {
    #pragma HLS PIPELINE off
        write_score(score_local_stream, n, output, num_couples, to_send);
    }
}

void collector(hls::stream<int32_t> input_stream[NUM_TILES],
    hls::stream<int> &final_score_stream, int num_couples) {

    loop_collector: for (int i = 0; i < num_couples / NUM_TILES; i++) {
		 loop_collector_inner: for (int j = 0; j < NUM_TILES; j++) {
 #pragma HLS PIPELINE
			int tmp = input_stream[j].read();
			final_score_stream.write(tmp);
		 }
	 }
}

extern "C" {
    
    void output_sink(hls::stream<int32_t> input_stream[NUM_TILES], input_t* output, int num_couples){
    
#pragma HLS interface axis port=input_stream

#pragma HLS INTERFACE m_axi port=output depth=m_axi_depth offset=slave bundle=gmem1
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS interface s_axilite port=num_couples bundle=control
#pragma HLS interface s_axilite port=return bundle=control

#pragma HLS DATAFLOW

        static hls::stream<int> final_score_stream;
#pragma HLS STREAM variable=final_score_stream depth=no_couples_per_stream dim=1

        collector(input_stream, final_score_stream, num_couples);
        write_score_wrapper(final_score_stream, num_couples, output);

    }
}
