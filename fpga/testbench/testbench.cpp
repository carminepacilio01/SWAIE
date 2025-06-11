/*
MIT License

Copyright (c) 2025 Carmine Pacilio

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <limits>
#include <ap_int.h>
#include <random>
#include <time.h>
#include <chrono>

#include "../common/common.h"

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;

typedef struct conf {
	int match;
	int mismatch;
	int gap_opening;
} conf_t;


void printConf(std::vector<char>& target, std::vector<char>& database, int ws, int wd, int gap_opening);
int compute_golden(std::vector<char>& target, std::vector<char>& database, int wd, int ws, int gap_opening);
void random_seq_gen(std::vector<char>& target, std::vector<char>& database);
int gen_rnd(int min, int max);
alphabet_datatype compression(char letter);

int main(int argc, char *argv[]) {

    srandom(static_cast<unsigned>(time(0)));

	int size = INPUT_SIZE;

	const int wd 			=  1; 	// match score
	const int ws 			= -1;	// mismatch score
	const int gap_opening	= -2;

	conf_t scoring;
	scoring.match 			= wd;
	scoring.mismatch			= ws;
	scoring.gap_opening		= gap_opening;

	std::vector< std::vector<char> > target(INPUT_SIZE, std::vector<char>(MAX_DIM));
	std::vector< std::vector<char> > database(INPUT_SIZE, std::vector<char>(MAX_DIM));

	std::vector<int32_t> hw_score(INPUT_SIZE, 0);
	std::vector<int32_t> golden_score(INPUT_SIZE, 0);

	int cell_number;

/////////////////////////		DATASET GENERATION 		////////////////////////////////////

	std::cout << "[SWAIE TESTBENCH] Generating "<< INPUT_SIZE << " random sequence pairs..." << std::endl;
	// Generation of random sequences
    for(int i = 0; i < INPUT_SIZE; i++){

		//	generate rand sequences
		random_seq_gen(target[i], database[i]);
		cell_number += SEQ_SIZE * SEQ_SIZE;
	}

/////////////////////////			TESTBENCH			////////////////////////////////////

	std::cout << "[SWAIE TESTBENCH] Running Software version." << std::endl;;
	auto start = std::chrono::high_resolution_clock::now();

	for (int golden_rep = 0; golden_rep < INPUT_SIZE; golden_rep++) {
		golden_score[golden_rep] = compute_golden(target[golden_rep], database[golden_rep], wd, ws, gap_opening);
	}

	auto stop = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    auto gcup = (unsigned) cell_number / (float)duration.count();
	
	std::cout << "\t -- Software version executed in " <<  (float)duration.count() * 1e-6 << " ms " << std::endl;
    std::cout << "\t -- GCUPS: " << gcup << std::endl;

	////////test bench results
	// std::cout << bold_on << "[SWAIE] Comparing results. \n" << bold_off << std::endl;
	// bool test_score=true;
	// for (int i=0; i < INPUT_SIZE; i++){
	// 	if (hw_score[i]!=golden_score[i]){
    //         std::cout << bold_on << red << "[SWAIE] Test [" << i << "] FAILED: Output does not match reference." << reset << std::endl;
	// 		printConf(target[i], database[i], ws, wd, gap_opening);
    //         std::cout << "HW: "<< hw_score[i] << ", SW: " << golden_score[i] << std::endl;
    //         test_score=false;
    //     }
	// }

	// if (test_score) std::cout << bold_on << green << "[SWAIE] Test PASSED: All outputs match are correct." << reset << std::endl;
	// else std::cout << bold_on << red << "[SWAIE] Test FAILED: Some outputs do not match reference." << reset << std::endl;
	
	return 0;
}

///////////// UTILITY FUNCTIONS //////////////

//	Prints the current configuration
void printConf(std::vector<char>& target, std::vector<char>& database, int ws, int wd, int gap_opening){
	std::cout << "+++ Sequence A: [" << target.size() << "]: " << std::string(target.begin(), target.end()) << std::endl;
	std::cout << "+++ Sequence B: [" << database.size() << "]: " << std::string(database.begin(), database.end()) << std::endl;
	std::cout << "+++ Match Score: " << wd << std::endl;
	std::cout << "+++ Mismatch Score: " << ws << std::endl;
	std::cout << "+++ Gap Opening: " << gap_opening << std::endl;
}

int gen_rnd(int min, int max) {
     // Using random function to get random double value
    return (int) min + rand() % (max - min + 1);
}

void random_seq_gen(std::vector<char>& target, std::vector<char>& database){

	char alphabet[4] = {'A', 'C', 'G', 'T'};
	int i;
	for(i = 0; i < SEQ_SIZE; i++){
		int tmp_gen = gen_rnd(0, 3);
		target[i] = alphabet[tmp_gen];
	}

	for(i = 0; i < SEQ_SIZE; i++){
		int tmp_gen = gen_rnd(0, 3);
		database[i] = alphabet[tmp_gen];
	}
}

int compute_golden(std::vector<char>& target, std::vector<char>& database, int wd, int ws, int gap_opening){
        std::vector<std::vector<int>> D(SEQ_SIZE + 1, std::vector<int>(SEQ_SIZE + 1, 0));
	    int max_score = 0;

	     for (int i = 1; i < SEQ_SIZE+1; ++i) {
	        for (int j = 1; j < SEQ_SIZE+1; ++j) {
                int m = (target[i-1] == database[j-1]) ? wd : ws;
                D[i][j] = std::max(0, D[i-1][j-1] + m);
                D[i][j] = std::max(D[i][j], D[i-1][j] + gap_opening);
                D[i][j] = std::max(D[i][j], D[i][j-1] + gap_opening);

	            // Aggiorna lo score massimo
	            max_score = std::max(max_score, D[i][j]);
	        }
	    }

	    return max_score;
}

alphabet_datatype compression(char letter) {
    switch (letter) {
		case '-':
			return 4;
        case 'A':
            return 0;
        case 'C':
            return 1;
        case 'G':
            return 2;
        case 'T':
            return 3;
    }

    return -1;
}