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
    int match = 1;
    int mismatch = -1;
    int gap = -2;

    for(int iter=0; iter < INPUT_SIZE; iter++) {
        static uint32_t target[SEQ_SIZE];
        static uint32_t database[SEQ_SIZE];
        int prev_row[SEQ_SIZE+1] = {0};
        int curr_row[SEQ_SIZE+1] = {0};
        int32_t score = 0;

        for(int j=0; j < SEQ_SIZE / 4; j++) {
            aie::vector<int32_t,4> tmp_target = readincr_v4(in_target);
            aie::vector<int32_t,4> tmp_database = readincr_v4(in_database);
			
			target[j*4] = tmp_target[0];
            target[j*4+1] = tmp_target[1];
            target[j*4+2] = tmp_target[2];
            target[j*4+3] = tmp_target[3];

            database[j*4] = tmp_database[0];
            database[j*4+1] = tmp_database[1];
            database[j*4+2] = tmp_database[2];
            database[j*4+3] = tmp_database[3];
        }

        target[37*4] = readincr(in_target); 
        database[37*4] = readincr(in_database);
        target[37*4 + 1] = readincr(in_target); 
        database[37*4 + 1] = readincr(in_database);

        for (int i = 1; i <= SEQ_SIZE; ++i) {
            for (int j = 1; j <= SEQ_SIZE; ++j) {
                int m = (target[i - 1] == database[j - 1]) ? match : mismatch;

                int score_diag = prev_row[j - 1] + m;       // match/mismatch
                int score_up   = prev_row[j] + gap;         // deletion
                int score_left = curr_row[j - 1] + gap;     // insertion

                curr_row[j] = std::max({0, score_diag, score_up, score_left});
                score = std::max(score, curr_row[j]);
            }

            std::memcpy(prev_row, curr_row, (SEQ_SIZE + 1) * sizeof(int));
        }

        writeincr(output, score);
    }
}