/******************************************
*MIT License
*
# *Copyright (c) [2025]
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
#include <random>
#include <time.h>

#include "xcl/xcl2.hpp"
#include "../common/common.h"

typedef struct conf {
	int match;
	int mismatch;
	int gap_opening;
	int gap_extension;
} conf_t;

typedef ap_uint<BITS_PER_CHAR> alphabet_datatype;
typedef ap_uint<PORT_WIDTH> input_t;

void printConf(char *target, char *database, int ws, int wd, int gap_opening, int enlargement);
int compute_golden(int lenT, char *target, int lenD, char *database, int wd, int ws, int gap_opening, int enlargement);
void random_seq_gen(int lenT, char *target, int lenD, char *database);
int gen_rnd(int min, int max);
alphabet_datatype compression(char letter);

int main(int argc, char* argv[]){

    if(argc < 2) {
		std::cout << "Usage: " << argv[0] <<" <xclbin>" << std::endl;
		return EXIT_FAILURE;
	}

    srandom(static_cast<unsigned>(time(0)));

	int num_couples = INPUT_SIZE;

	const int wd 			=  1; 	// match score
	const int ws 			= -1;	// mismatch score
	const int gap_opening	= -3;
	const int enlargement 	= -1;

	conf_t local_conf;
	local_conf.match 			= wd;
	local_conf.mismatch			= ws;
	local_conf.gap_opening		= gap_opening + enlargement;
	local_conf.gap_extension	= enlargement;

    cl_int 				err;
    cl::Context 		context;
    cl::Kernel 			data_reader;
	cl::Kernel 			output_sink;
    cl::CommandQueue 	q;

	std::vector< int, aligned_allocator<int> > 		lenT(INPUT_SIZE);
    std::vector< int, aligned_allocator<int> > 		lenD(INPUT_SIZE);
    std::vector< int, aligned_allocator<int> > 		score(INPUT_SIZE);

	char target[INPUT_SIZE][MAX_DIM];
	char database[INPUT_SIZE][MAX_DIM];

	input_t input[N_PACK] = {0};

	int cell_number;

/////////////////////////		DATASET GENERATION 		////////////////////////////////////

	std::cout << "Generating "<< INPUT_SIZE << " random sequence pairs." << std::endl;
	///////Generation of random sequences
	// Generation of random sequences
    for(int i = 0; i < INPUT_SIZE; i++){

		//	generate random sequences
		//	length of the sequences
		lenT[i] = (int)  gen_rnd(SEQ_SIZE - 10, SEQ_SIZE - 2);
		lenD[i] = (int)  gen_rnd(SEQ_SIZE - 10, SEQ_SIZE - 2);

		lenT[i] += 1;
		lenD[i] += 1;

		//	generate rand sequences
		target[i][0] = database[i][0] = '-';
		target[i][lenT[i]] = database[i][lenD[i]] = '\0';
		random_seq_gen(lenT[i], target[i], lenD[i], database[i]);
		
		cell_number += lenD[i] * lenT[i];
	}

	// populating kernel datatypes
	alphabet_datatype compressed_input[(INPUT_SIZE*(SEQ_SIZE + PADDING_SIZE))*2];
	for(int n=0; n < INPUT_SIZE; n++){
		char tmp[MAX_DIM];
		copy_reversed_for: for (int i = 0; i < lenT[n]; i++) {
			tmp[i] = target[n][lenT[n] - i - 1];
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


	for (int n = 0; n < num_couples; n++) {
		int* input_lengths = (int*)&input[n*(PACK_SEQ*2+1)];

		input_lengths[0] = lenT[n];
		input_lengths[1] = lenD[n];

		int k = 0;
		for(int i = 0; i < PACK_SEQ*2 ; i++){
			for(int j = 0; j < 128; j++){
				input[1 + n*(PACK_SEQ*2+1) + i].range((j+1)*BITS_PER_CHAR-1, j*BITS_PER_CHAR) = compressed_input[k+((SEQ_SIZE + PADDING_SIZE)*2)*n];
				k++;
			}
		}
	}

/////////////////////////		OPENCL CONFIGURATION 		////////////////////////////////////

	std::cout << "Programmin FPGA Device. " << std::endl;;

   	std::string binaryFile = argv[1]; // prendo il bitstream 
    auto fileBuf = xcl::read_binary_file(binaryFile); // leggi bitstream
    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};

    auto devices = xcl::get_xil_devices(); // lista di devices

    bool valid_device = false;

    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, data_reader= cl::Kernel(program, "data_reader", &err));
			OCL_CHECK(err, output_sink= cl::Kernel(program, "output_sink", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    } if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

	// Create device buffers
    cl::Buffer input_buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(input_t)*N_PACK, input);
	std::cout <<("Copying input sequences on the FPGA. \n");

    // Data will be migrated to kernel space
    err = q.enqueueMigrateMemObjects({input_buffer}, 0); /*0 means from host*/
	q.finish();

/////////////////////////		KERNEL EXCECUTION 		////////////////////////////////////

	// Set the arguments for kernel execution
	OCL_CHECK(err, err = data_reader.setArg(0, input_buffer));
	OCL_CHECK(err, err = data_reader.setArg(1, local_conf));
	OCL_CHECK(err, err = data_reader.setArg(2, num_couples));

	if (err != CL_SUCCESS) {
		std::cout << "Error: Failed to set kernel arguments! " << err << std::endl;
		std::cout << "Test failed" << std::endl;
		return EXIT_FAILURE;;
	}

	std::cout <<("Running FPGA accelerator. \n");
	
	//Launch the Kernels
	auto start = std::chrono::high_resolution_clock::now();
    q.enqueueTask(krnl);
	q.finish();
	auto stop = std::chrono::high_resolution_clock::now();

	if (err != CL_SUCCESS) {
		std::cout << "Error: Failed to launch kernel(s)! " << err << std::endl;
		std::cout << "Test failed" << std::endl;
		return EXIT_FAILURE;;
	}
	
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	float gcup = (double) (cell_number / (float)duration.count());
	
	//Data from Kernel to Host
    err = q.enqueueMigrateMemObjects({input_buffer}, CL_MIGRATE_MEM_OBJECT_HOST);  
	if (err != CL_SUCCESS) {
		std::cout << "Error: Failed to retrive objects from kernel(s)!" << err << std::endl;
		std::cout << "Test failed" << std::endl;
		return EXIT_FAILURE;;
	}
	q.finish();

	std::cout << "Finished FPGA excecution." << std::endl;
    std::cout << "FPGA Kernel executed in " << (float)duration.count() * 1e-6 << "ms" << std::endl;
	std::cout << "GCUPS: " << gcup << std::endl;

/////////////////////////			TESTBENCH			////////////////////////////////////
	int golden_score[INPUT_SIZE];

	std::cout << "Running Software version." << std::endl;;
	start = std::chrono::high_resolution_clock::now();

	for (int golden_rep = 0; golden_rep < INPUT_SIZE; golden_rep++) {
		golden_score[golden_rep] = compute_golden(lenT[golden_rep], target[golden_rep], lenD[golden_rep], database[golden_rep], wd, ws, gap_opening, enlargement);
	}

	stop = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	
	std::cout << "Software version executed in " <<  (float)duration.count() * 1e-6 << " ms " << std::endl;

	////////test bench results
	std::cout << "Comparing results. \n" << std::endl;
	bool test_score=true;
	for (int i=0; i < INPUT_SIZE; i++){
		if (score[i]!=golden_score[i]){
			printConf(target[i], database[i], ws, wd, gap_opening, enlargement);
            std::cout << "HW: "<< score[i] << ", SW: " << golden_score[i] << std::endl;
            test_score=false;
        }
	}

	if (test_score) 
		std::cout << "Test PASSED: Output matches reference." << std::endl;
	else {
		std::cout << "Test FAILED: Output does not match reference." << std::endl;
	}
	

	return 0;
}

//	Prints the current configuration
void printConf(char *target, char *database, int ws, int wd, int gap_opening, int enlargement) {
	std::cout << std::endl << "+++++++++++++++++++++" << std::endl;
	std::cout << "+ Sequence A: [" << strlen(target) << "]: " << target << std::endl;
	std::cout << "+ Sequence B: [" << strlen(database) << "]: " << database << std::endl;
	std::cout << "+ Match Score: " << wd << std::endl;
	std::cout << "+ Mismatch Score: " << ws << std::endl;
	std::cout << "+ Gap Opening: " << gap_opening << std::endl;
	std::cout << "+ Enlargement: " << enlargement << std::endl;
	std::cout << "+++++++++++++++++++++" << std::endl;
}

int gen_rnd(int min, int max) {
     // Using random function to get random double value
    return (int) min + rand() % (max - min + 1);
}

void random_seq_gen(int lenT, char *target, int lenD, char *database) {

	char alphabet[4] = {'A', 'C', 'G', 'T'};
	int i;
	for(i = 1; i < lenT; i++){
		int tmp_gen = gen_rnd(0, 3);
		target[i] = alphabet[tmp_gen];
	}

	for(i = 1; i < lenD; i++){
		int tmp_gen = gen_rnd(0, 3);
		database[i] = alphabet[tmp_gen];
	}
}

int compute_golden(int lenT, char *target, int lenD, char *database, int wd, int ws, int gap_opening, int enlargement) {
	// Inizializza le matrici D, P, Q
	    std::vector< std::vector<int> > D(lenT, std::vector<int>(lenD, 0));
	    std::vector< std::vector<int> > P(lenT, std::vector<int>(lenD, std::numeric_limits<int>::min() / 2));
	    std::vector< std::vector<int> > Q(lenT, std::vector<int>(lenD, std::numeric_limits<int>::min() / 2));

	    int gap_penalty = gap_opening + enlargement * 1;
	    int max_score = 0;

	    for (int i = 1; i < lenT; ++i) {
	        for (int j = 1; j < lenD; ++j) {
	            // Calcola P[i][j]
	            P[i][j] = std::max(P[i-1][j] + enlargement, D[i-1][j] + gap_penalty);

	            // Calcola Q[i][j]
	            Q[i][j] = std::max(Q[i][j-1] + enlargement, D[i][j-1] + gap_penalty);

	            // Calcola D[i][j]
	            int match = (target[i] == database[j]) ? wd : ws;
	            D[i][j] = std::max(0, D[i-1][j-1] + match);
	            D[i][j] = std::max(D[i][j], P[i][j]);
	            D[i][j] = std::max(D[i][j], Q[i][j]);

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
