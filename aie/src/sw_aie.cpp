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

#include "sw_aie.h"
#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

void compute_sw(input_stream<int32_t>* restrict in_target, input_stream<int32_t>* restrict in_database, 
    output_stream<int32_t>* restrict output) {

    constexpr int32_t match = MATCH;
    constexpr int32_t mismatch = MISMATCH;
    constexpr int32_t gap_opening = GAP_OPENING;

    for(int iter=0; iter < INPUT_SIZE / NUM_TILES; iter++) {

		int32_t target[MAX_DIM] = {0};
		int32_t database[MAX_DIM] = {0};
        alignas(32) int32_t prev_row[SEQ_SIZE+1] = {0};
        alignas(32) int32_t curr_row[SEQ_SIZE+1] = {0};

        int32_t score = 0;

        for (int i = 0; i < MAX_DIM; i += 4) {
            aie::vector<int32_t, 4> tr_vec = readincr_v4(in_target);
            aie::vector<int32_t, 4> db_vec = readincr_v4(in_database);

			aie::store_v(target + i, tr_vec);
			aie::store_v(database + i, db_vec);
        }

		for (int i = 1; i <= SEQ_SIZE; ++i) {
			for (int j = 1; j <= SEQ_SIZE; ++j) {
				int m = (target[i - 1] == database[j - 1]) ? MATCH : MISMATCH;

				int score_diag = prev_row[j - 1] + m;       // match/mismatch
				int score_up   = prev_row[j] + GAP_OPENING;         // deletion
				int score_left = curr_row[j - 1] + GAP_OPENING;     // insertion

				curr_row[j] = std::max({0, score_diag, score_up, score_left});
				score = std::max(score, curr_row[j]);
			}

			for (int i = 0; i < (SEQ_SIZE + 1); i += 4) {
                auto vec = aie::load_v<4>(curr_row + i);
                aie::store_v(prev_row + i, vec);
            }
		}

        writeincr(output, score);
    }
}