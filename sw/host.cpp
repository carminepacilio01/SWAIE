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

#include "experimental/xrt_kernel.h"
#include "../common/common.h"

// For hw emulation, run in sw directory: source ./setup_emu.sh -s on

#define DEVICE_ID 2

#define arg_reader_input 0
#define arg_reader_scoring 1
#define arg_reader_size 2

#define arg_sink_output 1
#define arg_sink_size 2

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;
typedef struct conf {
	int match;
	int mismatch;
	int gap_opening;
} conf_t;

std::ostream& bold_on(std::ostream& os);
std::ostream& bold_off(std::ostream& os);
std::ostream& red(std::ostream& os);
std::ostream& green(std::ostream& os);  
std::ostream& reset(std::ostream& os);

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

	input_t input[PACK_SEQ] = {0};
	std::vector<int32_t> hw_score(INPUT_SIZE, 0);
	std::vector<int32_t> golden_score(INPUT_SIZE, 0);

	int cell_number;

///////////////////////////     LOADING XCLBIN      /////////////////////////// 

    if(argc < 2) {
		std::cerr << bold_on << red << "[SWAIE] Error: No xclbin file provided." << reset << std::endl;
		std::cerr << "Usage: " << argv[0] << " <xclbin_file>" << std::endl;

		return EXIT_FAILURE;
	}

    std::string xclbin_file = argv[1];

    // Load xclbin
    std::cout << bold_on << "[SWAIE] Loading xclbin file: " << xclbin_file << bold_off << std::endl;
    xrt::device device = xrt::device(DEVICE_ID);
    xrt::uuid xclbin_uuid;
    try {
        xclbin_uuid = xrt::uuid(device.load_xclbin(xclbin_file));
    } catch (const std::exception &e) {
        std::cerr << bold_on << red << "[SWAIE] Error loading xclbin: " << e.what() << reset << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << green << "[SWAIE] Bitstream Loaded Succesfully!" << reset << std::endl;

/////////////////////////		DATASET GENERATION 		////////////////////////////////////

	std::cout << "[SWAIE] Generating "<< INPUT_SIZE << " random sequence pairs..." << std::endl;
	// Generation of random sequences
    for(int i = 0; i < INPUT_SIZE; i++){

		//	generate rand sequences
		random_seq_gen(target[i], database[i]);
		
		cell_number += SEQ_SIZE * SEQ_SIZE;
	}

	// populating kernel datatypes

	//TO-DO: check here the creation of input buffer, how to pass it with fixed size, then also pass to AIE one stream for 
	// database and one fro target... fix size... fix configuration.
	alphabet_datatype compressed_input[(INPUT_SIZE*(SEQ_SIZE + PADDING_SIZE))*2];
	for(int n=0; n < INPUT_SIZE; n++){
		char tmp[MAX_DIM];
		copy_reversed_for: for (int i = 0; i < SEQ_SIZE; i++) {
			tmp[i] = target[n][SEQ_SIZE - i - 1];
		}
		for(int i = 0; i < SEQ_SIZE + PADDING_SIZE; i++){
			compressed_input[i+(2*n)*(SEQ_SIZE + PADDING_SIZE)] = compression(tmp[i]);
		}
	}

	for(int n=0; n < INPUT_SIZE; n++){
		for(int i = 0; i < SEQ_SIZE + PADDING_SIZE; i++){
			compressed_input[i+(2*n+1)*(SEQ_SIZE + PADDING_SIZE)] = compression(database[n][i]);
		}
	}

	for (int n = 0; n < size; n++) {

		int k = 0;
		for(int i = 0; i < PACK_SEQ*2 ; i++){
			for(int j = 0; j < 128; j++){
				input[n*(PACK_SEQ*2+1) + i].range((j+1)*BITS_PER_CHAR-1, j*BITS_PER_CHAR) = compressed_input[k+((SEQ_SIZE + PADDING_SIZE)*2)*n];
				k++;
			}
		}
	}

///////////////////////////     INITIAL2IZING THE BOARD     ///////////////////////////  

    // create kernel objects
    xrt::kernel data_reader  = xrt::kernel(device, xclbin_uuid, "data_reader");
    xrt::kernel output_sink  = xrt::kernel(device, xclbin_uuid, "output_sink");

    // get memory bank groups for device buffer - required for axi master input/ouput
    xrtMemoryGroup bank_output  = output_sink.group_id(arg_sink_output);
    xrtMemoryGroup bank_input  = data_reader.group_id(arg_reader_input);

    // create device buffers - if you have to load some data, here they are
    xrt::bo buffer_reader = xrt::bo(device, size * sizeof(int32_t), xrt::bo::flags::normal, bank_input); 
    xrt::bo buffer_output = xrt::bo(device, size * sizeof(int32_t), xrt::bo::flags::normal, bank_output); 

    // create runner instances
    xrt::run run_data_reader   = xrt::run(data_reader);
    xrt::run run_output_sink = xrt::run(output_sink);

    run_data_reader.set_arg(arg_reader_input, buffer_reader);
    run_data_reader.set_arg(arg_reader_scoring, scoring);
    run_data_reader.set_arg(arg_reader_size, size);

    // set sink_from_aie kernel arguments
    run_output_sink.set_arg(arg_sink_output, buffer_output);
    run_output_sink.set_arg(arg_sink_size, size);

    std::cout << bold_on << "[SWAIE] Running FPGA accelerator. \n" << bold_off;
    std::cout << "[SWAIE] Writing " << INPUT_SIZE << " sequences to accelarator. \n" << bold_off;
    // write data into the input buffer
    buffer_reader.write(input);
    buffer_reader.sync(XCL_BO_SYNC_BO_TO_DEVICE);

    auto start = std::chrono::high_resolution_clock::now();
    // run the kernel
    run_output_sink.start();
    run_data_reader.start();

    // wait for the kernel to finish
    run_data_reader.wait();
    run_output_sink.wait();
    auto stop = std::chrono::high_resolution_clock::now();
    std::cout << "[SWAIE] Reading results." << bold_off;
    // read the output buffer
    buffer_output.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
    buffer_output.read(hw_score.data()); 

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	float gcup = (double) (cell_number / (float)duration.count());
	
    std::cout << bold_on << green << "[SWAIE] Finished FPGA excecution." << reset << std::endl;
    std::cout << "\t -- FPGA Kernel executed in " << (float)duration.count() * 1e-6 << "ms" << std::endl;
	std::cout << "\t -- GCUPS: " << gcup << std::endl;

    /////////////////////////			TESTBENCH			////////////////////////////////////

	std::cout << bold_on << "[SWAIE] Running Software version." << bold_off << std::endl;;
	start = std::chrono::high_resolution_clock::now();

	for (int golden_rep = 0; golden_rep < INPUT_SIZE; golden_rep++) {
		golden_score[golden_rep] = compute_golden(target[golden_rep], database[golden_rep], wd, ws, gap_opening);
	}

	stop = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    gcup = (double) (cell_number / (float)duration.count());
	
	std::cout << "\t -- Software version executed in " <<  (float)duration.count() * 1e-6 << " ms " << std::endl;
    std::cout << "\t -- GCUPS: " << gcup << std::endl;

	////////test bench results
	std::cout << bold_on << "[SWAIE] Comparing results. \n" << bold_off << std::endl;
	bool test_score=true;
	for (int i=0; i < INPUT_SIZE; i++){
		if (hw_score[i]!=golden_score[i]){
            std::cout << bold_on << red << "[SWAIE] Test [" << i << "] FAILED: Output does not match reference." << reset << std::endl;
			printConf(target[i], database[i], ws, wd, gap_opening);
            std::cout << "HW: "<< hw_score[i] << ", SW: " << golden_score[i] << std::endl;
            test_score=false;
        }
	}

	if (test_score) std::cout << bold_on << green << "[SWAIE] Test PASSED: All outputs match are correct." << reset << std::endl;
	else std::cout << bold_on << red << "[SWAIE] Test FAILED: Some outputs do not match reference." << reset << std::endl;
	
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
	    std::vector< std::vector<int> > D(SEQ_SIZE+1, std::vector<int>(SEQ_SIZE+1, 0));
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

///////////// PRINTING FUNCTIONS //////////////

std::ostream& bold_on(std::ostream& os) {
    if (&os == &std::cout && isatty(fileno(stdout))) {
        return os << "\e[1m";
    }
    return os;
}

std::ostream& bold_off(std::ostream& os) {
    if (&os == &std::cout && isatty(fileno(stdout))) {
        return os << "\e[0m";
    }
    return os;
}

std::ostream& red(std::ostream& os) {
    return os << "\033[31m";
}

std::ostream& green(std::ostream& os) {
    return os << "\033[32m";
}

// Reset all attributes
std::ostream& reset(std::ostream& os) {
    return os << "\033[0m";
}
