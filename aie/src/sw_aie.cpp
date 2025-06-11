#include "sw_aie.h"
#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

void compute_sw(input_stream<int32_t>* restrict input, output_stream<int32_t>* restrict output){
    //first 4 numbers are loop size, match, mismatch score and gap_opening.
    aie::vector<int32_t,4> x = readincr_v4(input);
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
            aie::vector<int32_t,4> x = readincr_v4(input);
			target[j*4] = x[0];
            target[j*4+1] = x[1];
            target[j*4+2] = x[2];
            target[j*4+3] = x[3];
        }

        for(int j=0; j < SEQ_SIZE / 4; j++) {
            aie::vector<int32_t,4> x = readincr_v4(input);
			database[j*4] = x[0];
            database[j*4+1] = x[1];
            database[j*4+2] = x[2];
            database[j*4+3] = x[3];
        }

        for (int i = 1; i <= SEQ_SIZE; ++i) {
            for (int j = 1; j <= SEQ_SIZE; ++j) {
                int m = (target[i-1] == database[j-1]) ? match : mismatch;

                curr_row[j] = std::max(0, prev_row[j-1] + m);
                curr_row[j] = std::max(curr_row[j], prev_row[j] + gap);
                curr_row[j] = std::max(curr_row[j], curr_row[j-1] + gap);

                score = std::max(score, curr_row[j]);
            }
            for (int j = 0; j <= SEQ_SIZE; ++j) prev_row[j] = curr_row[j];
        }

        writeincr(output, score);
    }
}