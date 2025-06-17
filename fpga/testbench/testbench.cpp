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
#include "../common/fastareader.h"

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;

void printConf(const std::vector<alphabet_datatype>& target, const std::vector<alphabet_datatype>& database);
int compute_golden(const std::vector<alphabet_datatype>& target, const std::vector<alphabet_datatype>& database);
void showProgressBar(int progress, int total);
std::string toString(const std::vector<alphabet_datatype>& seq);

int main(int argc, char *argv[]) {
	std::cout << "[SWAIE TESTBENCH] Starting testbench." << std::endl;;

	std::string filename;
	std::vector<int> golden_score(INPUT_SIZE, 0);

	if(argc < 2) filename = "SRR33920980.fasta";
	else filename = argv[1];

/////////////////////////			TESTBENCH			////////////////////////////////////

	std::cout << "[SWAIE TESTBENCH] Reading dataset version." << std::endl;

	auto result = fastareader::readFastaFile(filename);
	auto& target = std::get<0>(result);
	auto& database = std::get<1>(result);

	std::cout << "[SWAIE TESTBENCH] Running golden version." << std::endl;

	for (int golden_rep = 0; golden_rep < INPUT_SIZE; golden_rep++) {
		std::cout << "\r[SWAIE TESTBENCH] Aliging: ";
		showProgressBar(golden_rep + 1, INPUT_SIZE);
		golden_score[golden_rep] = compute_golden(target[golden_rep], database[golden_rep]);
	}

	std::cout << std::endl;
	std::cout << "[SWAIE TESTBENCH] Golden version executed succesfully." << std::endl;

	std::cout << "[SWAIE TESTBENCH] Generating AIE input file." << std::endl;
	std::ofstream in_target("../../aie/data/in_target.txt", std::ios::out | std::ios::trunc);
	std::ofstream in_database("../../aie/data/in_database.txt", std::ios::out | std::ios::trunc);
    if (!in_target || !in_database) {
        std::cerr << "[SWAIE TESTBENCH] Error opening file(s) for writing." << std::endl;
        return EXIT_FAILURE;
    }
	for(size_t i = 0; i < INPUT_SIZE; ++i) {
		std::cout << "\r[SWAIE TESTBENCH] Writing sequence: ";
		// in_target << target[i][0] << std::endl;
		// in_target << target[i][1] << std::endl;
		// in_database << database[i][0] << std::endl;
		// in_database << database[i][1] << std::endl;
		for(size_t j = 0; j < MAX_DIM; ++j) {
			in_target << target[i][j] << std::endl;
			in_database << database[i][j] << std::endl;
		}
		showProgressBar(i+1, INPUT_SIZE);
	}
	in_target.close();
	in_database.close();

	std::cout << std::endl;
	std::cout << "[SWAIE TESTBENCH] Running AIE simulation." << std::endl;
	
	int ret = std::system("make -C ../../aie aie_simulate_x86");

    if (ret != 0) {
        std::cerr << "[SWAIE TESTBENCH] Make target failed with code: " << ret << std::endl;
        return ret;
    }

	std::cout << "[SWAIE TESTBENCH] Analyzing results." << std::endl;
	std::ifstream infile("../../aie/x86simulator_output/data/out_score.txt");
	if (!infile) {
		std::cerr << "[SWAIE TESTBENCH] Error opening output file." << std::endl;
		return EXIT_FAILURE;
	}

	for(size_t i = 0; i < INPUT_SIZE; ++i) {
		int aie_score;
		infile >> aie_score;
		if(aie_score != golden_score[i]) {
			std::cout << "\033[1;31m[SWAIE TESTBENCH] ✖ Mismatch detected during simulation! \033[0m" << std::flush; 
			std::cout << "- occured at aligment ["<< i << "]: AIE score = " << aie_score << ", Golden score = " << golden_score[i] << std::endl;
			printConf(target[i], database[i]);
			return EXIT_FAILURE;
		}
		std::cout << "\r[SWAIE TESTBENCH] Comparing sequence: ";
		showProgressBar(i+1, INPUT_SIZE);
	}
	std::cout << std::endl;
	std::cout << "\033[1;32m[SWAIE TESTBENCH] ✔ All scores match! \033[0m" << std::endl;
	std::cout << "[SWAIE TESTBENCH] Simulation completed successfully!" << std::endl;

	return 0;
}

///////////// UTILITY FUNCTIONS //////////////

//	Prints the current configuration
void printConf(const std::vector<alphabet_datatype>& target, const std::vector<alphabet_datatype>& database){
	std::cout << "+++ Sequence Target: [" << target.size() << "]: " << toString(target) << std::endl;
	std::cout << "+++ Sequence Database: [" << database.size() << "]: " << toString(database) << std::endl;
	std::cout << "+++ Match Score: " << MATCH << std::endl;
	std::cout << "+++ Mismatch Score: " << MISMATCH << std::endl;
	std::cout << "+++ Gap Opening: " << GAP_OPENING << std::endl;
}

int compute_golden(const std::vector<alphabet_datatype>& target, const std::vector<alphabet_datatype>& database){
	std::vector<std::vector<int>> D(SEQ_SIZE + 1, std::vector<int>(SEQ_SIZE + 1, 0));
	D.shrink_to_fit();
	int max_score = 0;
	for (int i = 1; i < SEQ_SIZE+1; ++i) {
		for (int j = 1; j < SEQ_SIZE+1; ++j) {
			int m = (target[i-1] == database[j-1]) ? MATCH : MISMATCH;
			D[i][j] = std::max(0, D[i-1][j-1] + m);
			D[i][j] = std::max(D[i][j], D[i-1][j] + GAP_OPENING);
			D[i][j] = std::max(D[i][j], D[i][j-1] + GAP_OPENING);

			max_score = std::max(max_score, D[i][j]);
		}
	}

	return max_score;
}

std::string toString(const std::vector<alphabet_datatype>& seq) {
    const std::string alphabet = "ACGT";
    std::string result;
    result.reserve(seq.size());

    for (alphabet_datatype base : seq) {
        result.push_back(alphabet[base]);
    }

    return result;
}

void showProgressBar(int progress, int total) {
    const int barWidth = 25;
    float ratio = static_cast<float>(progress) / total;
    int pos = static_cast<int>(barWidth * ratio);

    std::cout << "[";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "▒";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(ratio * 100.0) << " %\r";
    std::cout.flush();
}