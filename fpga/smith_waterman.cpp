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

 typedef struct conf {
	int match;
	int mismatch;
	int gap_opening;
	int gap_extension;
} conf_t;

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;
 
 const unsigned int depth_stream = DEPTH_STREAM;
 const unsigned int no_couples_per_stream = NO_COUPLES_PER_STREAM / NUM_CU;
 const unsigned int unroll_f = 2;
 const unsigned int num_pack = (PACK_SEQ << 1) + 1;
 
 void computeSW(hls::stream<input_t> &reads_stream,
		 hls::stream<int> &targetLength_stream,
		 hls::stream<int> &databaseLength_stream,
		 hls::stream<int> &score_local_stream, conf_t scoring) {
 
	 int lenT = targetLength_stream.read();
	 int lenD = databaseLength_stream.read();
	 
	 input_t p_input_output[PACK_SEQ << 1];
 #pragma HLS ARRAY_PARTITION variable=p_input_output dim=1 complete
 
 
	 read_data_from_couple: for (int i = 0; i < num_pack - 1; i++) {
 #pragma HLS PIPELINE
		 p_input_output[i] = reads_stream.read();
	 }
 
	 alphabet_datatype target[MAX_DIM];
	 alphabet_datatype database[MAX_DIM];
 
	 std::cout << "target: ";
	 int k = 0;
	 for (int i = 0; i < PACK_SEQ; i++) {
		 auto packed_data = p_input_output[i];
		 for (int j = 0; j < 128; j++) {
			 if (k < lenT) {
				 target[k] = packed_data.range(
					 (j + 1) * BITS_PER_CHAR - 1,
					 j * BITS_PER_CHAR
				 );
				 std::cout << target[k] << " ";
				 k++;
			 }
		 }
	 }
	 std::cout << std::endl;
	 std::cout << "database: ";
	 k = 0;
	 for (int i = PACK_SEQ; i < 2 * PACK_SEQ; i++) {
		 auto packed_data = p_input_output[i];
		 for (int j = 0; j < 128; j++) {
			 if (k < lenD) {
				 database[k] = packed_data.range(
					 (j + 1) * BITS_PER_CHAR - 1,
					 j * BITS_PER_CHAR
				 );
				 std::cout << database[k] << " ";
				 k++;
			 }
		 }
	 }
	 std::cout << std::endl;
 
 
 
 
 //	initialize local buffer for the 3 dependency
	 short p_buffer[3][MAX_DIM];
 #pragma HLS ARRAY_PARTITION variable=p_buffer type=block factor=3 dim=1
 #pragma HLS ARRAY_PARTITION variable=p_buffer type=cyclic factor=unroll_f dim=2
 
	 short q_buffer[3][MAX_DIM];
 #pragma HLS ARRAY_PARTITION variable=q_buffer type=block factor=3 dim=1
 #pragma HLS ARRAY_PARTITION variable=q_buffer type=cyclic factor=unroll_f dim=2
 
	 short d_buffer[3][MAX_DIM];
 #pragma HLS ARRAY_PARTITION variable=d_buffer type=block factor=3 dim=1
 #pragma HLS ARRAY_PARTITION variable=d_buffer type=cyclic factor=unroll_f dim=2
 
	 init_buffers_loop: for(int index = 0; index < MAX_DIM; index++) {
 #pragma HLS PIPELINE  II=1
		 p_buffer[0][index] = 0;
		 q_buffer[0][index] = 0;
		 d_buffer[0][index] = 0;
 
		 p_buffer[1][index] = 0;
		 q_buffer[1][index] = 0;
		 d_buffer[1][index] = 0;
 
		 p_buffer[2][index] = 0;
		 q_buffer[2][index] = 0;
		 d_buffer[2][index] = 0;
	 }
 
	 // Computation of the score by calling PEs
	 const short max_diag_len = (lenT < lenD) ? lenT : lenD; // the maximum value diag_len can reach
	 const short t_diag = lenT + lenD - 1;
	 short diag_len = 0; // the number of elements on the current diagonal
	 short n_diag_repeat = (lenT > lenD) ? (lenT - lenD) + 1 : (lenD - lenT) + 1;
	 short max_score = 0;
	 short score_l[MAX_DIM];
 
	 int diag_index = 0;
	 int database_cursor = 0;
	 int target_cursor = 0;
	 int score_index = 0;
 
	 compute_score_loop: for (int num_diag = 0; num_diag <  MAX_DIM * 2 - 1; num_diag++) {
		 if(num_diag < t_diag){
			 if (num_diag < lenT) {
				 if (diag_len < max_diag_len) {
					 diag_len++;
					 database_cursor = 0;
					 target_cursor = num_diag;
				 } else if (n_diag_repeat > 0) {
					 database_cursor = 0;
					 target_cursor = num_diag;
					 n_diag_repeat--;
					 if (n_diag_repeat == 0) {
						 diag_len--;
					 }
				 }
			 } else if (n_diag_repeat > 0) {
				 database_cursor++;
				 target_cursor = lenT + database_cursor - 1;
				 n_diag_repeat--;
				 if (n_diag_repeat == 0) {
					 diag_len--;
				 }
			 } else {
				 database_cursor++;
				 target_cursor = lenT + database_cursor - 1;
				 diag_len--;
			 }
 
			 // PE
			 compute_diagonal_loop: for (int j = 0; j < MAX_DIM; ++j) {
 #pragma HLS DEPENDENCE variable=p_buffer array inter false
 #pragma HLS DEPENDENCE variable=q_buffer array inter false
 #pragma HLS DEPENDENCE variable=d_buffer array inter false
 #pragma HLS DEPENDENCE variable=score_l inter false
 #pragma HLS DEPENDENCE variable=score_l intra false
 #pragma HLS UNROLL factor=unroll_f
 
				 if(j < diag_len) {
					 const int current_index = database_cursor + j;
					 const int two_diag = (diag_index == 0) ? 1 : (diag_index == 1) ? 2 : 0;
					 const int one_diag = (diag_index == 0) ? 2 : diag_index - 1;
 
					 const ap_uint<4> t = target[lenT - 1 - (target_cursor - current_index)];
					 const ap_uint<4> d = database[current_index];
					 short match = (t == d) ? scoring.match : scoring.mismatch;
 
					 const short tmp_dUP = d_buffer[one_diag][current_index + UP] + scoring.gap_opening;
					 const short tmp_pUP = p_buffer[one_diag][current_index + UP] + scoring.gap_extension;
					 const short tmp_dLEFT = d_buffer[one_diag][current_index + LEFT] + scoring.gap_opening;
					 const short tmp_dUPLEFT = d_buffer[two_diag][current_index + UP_LEFT];
					 const short tmp_qLEFT = q_buffer[one_diag][current_index + LEFT] + scoring.gap_extension;
					 short tmp_p, tmp_q, tmp_d;
 
					 if (t == 4) {
						 tmp_d = 0;
						 tmp_p = INFTY;
						 tmp_q = INFTY;
						 score_l[j] = 0;
					 } else if (d == 4) {
						 tmp_d = 0;
						 tmp_p = INFTY;
						 tmp_q = INFTY;
						 score_l[j] = 0;
					 } else {
						 tmp_p = (tmp_dUP < tmp_pUP) ? tmp_pUP : tmp_dUP;
						 tmp_q = (tmp_dLEFT < tmp_qLEFT) ? tmp_qLEFT : tmp_dLEFT;
						 tmp_d = (tmp_dUPLEFT + match < 0) ? 0 : tmp_dUPLEFT + match;
						 tmp_d = (tmp_d < tmp_p) ? tmp_p : tmp_d;
						 tmp_d = (tmp_d < tmp_q) ? tmp_q : tmp_d;
 
						 score_l[j] = tmp_d;
					 }
 
					 d_buffer[diag_index][current_index] = tmp_d;
					 p_buffer[diag_index][current_index] = tmp_p;
					 q_buffer[diag_index][current_index] = tmp_q;
 
				 }
			 }
 
			 //write out scoree
			 short tmp_score = 0;
			 max_score_loop: for(int i = 0; i < MAX_DIM; i++){
				 if(i < diag_len){
					 tmp_score = (score_l[i] > tmp_score) ? score_l[i] : tmp_score;
				 }
			 }
 
			 max_score = (tmp_score > max_score) ? tmp_score : max_score;
			 score_index = 0;
			 diag_index = (diag_index + 1) % 3;
		 }
 
	 }
 
	 score_local_stream.write(max_score);
 }
 
 void read_input_data(input_t *input_output,
		 hls::stream<input_t> &input_output_stream,
		 hls::stream<int> &targetLength_stream,
		 hls::stream<int> &databaseLength_stream, int n, int num_couples) {
 
 #pragma HLS INLINE off
 
	 int iter;
 
	 iter = (num_couples - (n)) < NO_COUPLES_PER_STREAM ?
			 (num_couples - (n)) * ((PACK_SEQ << 1) + 1) :
			 NO_COUPLES_PER_STREAM * ((PACK_SEQ << 1) + 1);
 
	 for (int i = 0; i < iter; i++) {
 
		 input_t tmp = input_output[n * ((PACK_SEQ << 1) + 1) + i];
 
		 if (i % ((PACK_SEQ << 1) + 1) == 0) {
 
			 int *seq_length = (int*) &tmp;
 
			 targetLength_stream.write(seq_length[0]);
			 databaseLength_stream.write(seq_length[1]);
 
		 } else {
			 input_output_stream.write(tmp);
		 }
	 }
 }
 
 void read_input_data_wrapper(input_t *input_output,
		 hls::stream<input_t> &input_output_stream,
		 hls::stream<int> &targetLength_stream,
		 hls::stream<int> &databaseLength_stream, int num_couples) {
 
	 loop_read_input_data_wrapper: for (int n = 0; n < num_couples; n += NO_COUPLES_PER_STREAM)
 #pragma HLS PIPELINE off
		 read_input_data(input_output, input_output_stream, targetLength_stream,
				 databaseLength_stream, n, num_couples);
 
 }
 
 void dispatcher(hls::stream<input_t> &input_output_stream,
		 hls::stream<input_t> reads_stream[NUM_CU], int num_couples,
		 hls::stream<int> &targetLength_stream,
		 hls::stream<int> &databaseLength_stream,
		 hls::stream<int> tlen_local_stream[NUM_CU],
		 hls::stream<int> dlen_local_stream[NUM_CU]) {
 
	 int idx = 0;
 
	 loop_dispatcher: for (int i = 0; i < num_couples;
			 i++, idx = idx < (NUM_CU - 1) ? (idx + 1) : 0) {
 
		 loop_dispatcher_inner: for (int j = 0; j < ((PACK_SEQ << 1) + 1); j++) {
 #pragma HLS PIPELINE
 
			 if (j == 0) {
 
				 int tmp_1 = targetLength_stream.read();
				 tlen_local_stream[idx].write(tmp_1);
 
				 int tmp_2 = databaseLength_stream.read();
				 dlen_local_stream[idx].write(tmp_2);
 
			 } else {
 
				 input_t tmp = input_output_stream.read();
				 reads_stream[idx].write(tmp);
			 }
		 }
	 }
 }
 
 void compute_wrapper(hls::stream<input_t> &reads_stream,
		 hls::stream<int> &targetLength_stream,
		 hls::stream<int> &databaseLength_stream,
		 hls::stream<int> &score_local_stream, int num_couples, conf_t scoring) {
 
	 int num_iter = num_couples / NUM_CU;
 
	 loop_compute_wrapper: for (int n = 0; n < num_iter; n++) {
 #pragma HLS PIPELINE off
		 computeSW(reads_stream, targetLength_stream, databaseLength_stream,
				 score_local_stream, scoring);
	 }
 }
 
 void collector(hls::stream<int> score_local_stream[NUM_CU],
		 hls::stream<int> &final_score_stream, int num_couples) {
 
	 loop_collector: for (int i = 0; i < num_couples / NUM_CU; i++) {
		 loop_collector_inner: for (int j = 0; j < NUM_CU; j++) {
 #pragma HLS PIPELINE
			 int tmp = score_local_stream[j].read();
			 final_score_stream.write(tmp);
		 }
	 }
 
 }
 
 void write_score(hls::stream<int> &final_score_stream, int n,
		 input_t *input_output, int num_couples, int &to_send) {
 
 #pragma HLS INLINE off
	 static int tmp[NUM_TMP_WRITE];
 #pragma HLS BIND_STORAGE variable=tmp type=ram_1p impl=bram
 
	 tmp[n & (NUM_TMP_WRITE - 1)] = final_score_stream.read();
 
	 if ((n > 0 && (((n + 1) & (NUM_TMP_WRITE - 1)) == 0))
			 || n == num_couples - 1) {
		 int iter = (to_send >= NUM_TMP_WRITE) ? NUM_TMP_WRITE : to_send;
 
		 for (int i = 0; i < iter; i++) {
 #pragma HLS pipeline
			 input_output[n + i + 1 - iter] = tmp[i];
		 }
 
		 to_send -= iter;
	 }
 
 }
 
 void write_score_wrapper(hls::stream<int> &score_local_stream, int num_couples,
		 input_t *input_output) {
	 int to_send = num_couples;
 
	 loop_write_score_wrapper: for (int n = 0; n < num_couples; n++) {
 #pragma HLS PIPELINE off
		 write_score(score_local_stream, n, input_output, num_couples, to_send);
	 }
 }
 
 void alignment(input_t *input_output, int num_couples, conf_t scoring,
		 hls::stream<input_t> &input_output_stream,
		 hls::stream<int> &targetLength_stream,
		 hls::stream<int> &databaseLength_stream,
		 hls::stream<int> score_local_stream[NUM_CU],
		 hls::stream<input_t> reads_stream[NUM_CU],
		 hls::stream<int> &final_score_stream,
		 hls::stream<int> tlen_local_stream[NUM_CU],
		 hls::stream<int> dlen_local_stream[NUM_CU] ) {
 
 #pragma HLS INLINE
 
	 int tot_couples = num_couples;
 
	 read_input_data_wrapper(input_output, input_output_stream,
			 targetLength_stream, databaseLength_stream, tot_couples);
 
	 dispatcher(input_output_stream, reads_stream, tot_couples,
			 targetLength_stream, databaseLength_stream, tlen_local_stream,
			 dlen_local_stream);
 
	 for (int i = 0; i < NUM_CU; i++) {
 #pragma HLS unroll
		 compute_wrapper(reads_stream[i], tlen_local_stream[i], dlen_local_stream[i],
				 score_local_stream[i], tot_couples, scoring);
 
	 }
 
	 collector(score_local_stream, final_score_stream, tot_couples);
	 write_score_wrapper(final_score_stream, tot_couples, input_output);
 
 }
 
 //////////////////MASTER AXI
 //extern "C" {
	 void sw_maxi(input_t *input_output,  conf_t scoring, int num_couples) {
 #pragma HLS INTERFACE s_axilite port=return bundle=control
 
 #pragma HLS INTERFACE m_axi port=input_output offset=slave bundle=gmem0 depth=m_axi_depth
 #pragma HLS INTERFACE s_axilite port=input_output bundle=control
 #pragma HLS INTERFACE s_axilite port=scoring bundle=control
 #pragma HLS INTERFACE s_axilite port=num_couples bundle=control
 
 #pragma HLS DATAFLOW
 
	 static hls::stream<input_t> input_output_stream("input_output_stream");
 #pragma HLS STREAM variable=input_output_stream depth=no_couples_per_stream dim=1
 
	 static hls::stream<input_t> reads_stream[NUM_CU];
 #pragma HLS STREAM variable=reads_stream depth=depth_stream dim=1
 #pragma HLS BIND_STORAGE variable=reads_stream type=fifo impl=bram
 
	 static hls::stream<int> targetLength_stream("targetLength_stream");
 #pragma HLS STREAM variable=targetLength_stream depth=no_couples_per_stream dim=1
 
	 static hls::stream<int> databaseLength_stream("databaseLength_stream");
 #pragma HLS STREAM variable=databaseLength_stream depth=no_couples_per_stream dim=1
 
	 static hls::stream<int> tlen_local_stream[NUM_CU];
 #pragma HLS STREAM variable=tlen_local_stream depth=no_couples_per_stream dim=1
 
	 static hls::stream<int> dlen_local_stream[NUM_CU];
 #pragma HLS STREAM variable=dlen_local_stream depth=no_couples_per_stream dim=1
 
	 static hls::stream<int> score_local_stream[NUM_CU];
 #pragma HLS STREAM variable=score_local_stream depth=no_couples_per_stream dim=1
 
	 static hls::stream<int> final_scores_stream("final_scores_stream");
 #pragma HLS STREAM variable=final_scores_stream depth=no_couples_per_stream dim=1
 
	 int tot_couples = num_couples;
 
	 alignment(input_output, tot_couples, scoring, input_output_stream,
			 targetLength_stream, databaseLength_stream, score_local_stream,
			 reads_stream, final_scores_stream, tlen_local_stream,
			 dlen_local_stream);
 
	 }
 //}
 