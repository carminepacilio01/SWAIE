/******************************************
*MIT License
*
# *Copyright (c) [2025] [Carmine Pacilio]
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
#include "smith_waterman.h"
#include <time.h>

void sw_maxi(input_t *input_output,  conf_t scoring, int num_couples);
void printConf(char *seqA, char *seqB, int ws, int wd, int gap_opening, int enlargement);
void fprintMatrix(int *P, int *Dsw_testbench, int *Q, int lenA, int lenB);
int compute_golden(int lenA, char *seqA, int lenB, char *seqB, int wd, int ws, int gap_opening, int enlargement);
void random_seq_gen(int lenA, char *seqA, int lenB, char *seqB);
int gen_rnd(int min, int max);
void reverse_str(int len, char *str);
alphabet_datatype compression(char letter);

using namespace std;

int main(){
	srandom(2);
	int num_couples = INPUT_SIZE;
	clock_t start, end;

	//	match score
	const int wd = 1;
	//	mismatch score
	const int ws = -1;

	const int gap_opening = -3;
	const int enlargement = -1;

	int count_failed = 0;
	//--------------------------------------------------------------------------------//
	const int n = INPUT_SIZE;
	int lenA[INPUT_SIZE];
	int lenB[INPUT_SIZE];
	char seqA[INPUT_SIZE][MAX_DIM];
	char seqB[INPUT_SIZE][MAX_DIM];
	input_t input_output[N_PACK] = {0};

	for(int i = 0; i < n; i++){
		//	generate random sequences
		//	length of the sequences
		lenA[i] = SEQ_SIZE; // (int)  gen_rnd(SEQ_SIZE - 10, SEQ_SIZE - 2);
		lenB[i] = SEQ_SIZE; // (int)  gen_rnd(SEQ_SIZE - 10, SEQ_SIZE - 2);

//		lenA[i] += 1;
//		lenB[i] += 1;

		//	generate rand sequences
		seqA[i][0] = seqB[i][0] = '-';
		seqA[i][lenA[i]] = seqB[i][lenA[i]] = '\0';
		random_seq_gen(lenA[i], seqA[i], lenB[i], seqB[i]);

		//	Printing current configuration
		//printConf(seqA[i], seqB[i], ws, wd, gap_opening, enlargement);
	}

	int golden_score[INPUT_SIZE];
	int n_ops = 0;

	double mean_golden_time = 0;
	double mean_golden_gcup = 0;
	for (int golden_rep = 0; golden_rep < n; golden_rep++) {
		golden_score[golden_rep] = compute_golden(lenA[golden_rep], seqA[golden_rep], lenB[golden_rep], seqB[golden_rep], wd, ws, gap_opening, enlargement);
	}

	conf_t local_conf;
	local_conf.match 			= wd;
	local_conf.mismatch			= ws;
	local_conf.gap_opening		= gap_opening + enlargement;
	local_conf.gap_extension	= enlargement;

	//	computing result using kernel
    alphabet_datatype compressed_input[(INPUT_SIZE*(SEQ_SIZE + PADDING_SIZE))*2];
	for(int n=0; n < INPUT_SIZE; n++){
		std::cout << "target["<<n<<"]: ";
		char tmp[MAX_DIM];
		copy_reversed_for: for (int i = 0; i < lenA[n]; i++) {
			tmp[i] = seqA[n][lenA[n] - i - 1];
		}
		for(int i = 0; i < SEQ_SIZE + PADDING_SIZE; i++){
			compressed_input[i+(2*n)*(SEQ_SIZE + PADDING_SIZE)] = compression(tmp[i]);
			std::cout << compressed_input[i+(2*n)*(SEQ_SIZE + PADDING_SIZE)] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
	for(int n=0; n < INPUT_SIZE; n++){
		std::cout << "database["<<n<<"]: ";
		for(int i = 0; i < SEQ_SIZE + PADDING_SIZE; i++){
			compressed_input[i+(2*n+1)*(SEQ_SIZE + PADDING_SIZE)] = compression(seqB[n][i]);
			std::cout << compressed_input[i+(2*n+1)*(SEQ_SIZE + PADDING_SIZE)] << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;


	for (int n = 0; n < num_couples; n++) {
		int* input_lengths = (int*)&input_output[n*(PACK_SEQ*2+1)];

		input_lengths[0] = lenA[n];
		input_lengths[1] = lenB[n];

		int k = 0;
		for(int i = 0; i < PACK_SEQ*2 ; i++){
			for(int j = 0; j < 128; j++){
				input_output[1 + n*(PACK_SEQ*2+1) + i].range((j+1)*BITS_PER_CHAR-1, j*BITS_PER_CHAR) = compressed_input[k+((SEQ_SIZE + PADDING_SIZE)*2)*n];
				k++;
			}
		}
	}

	start = clock();
	sw_maxi(input_output, local_conf, num_couples);
	end = clock();

	//	Score results
	for (int i = 0; i < INPUT_SIZE; i++)
	{
		if(input_output[i] == golden_score[i]){
			//cout << "TEST " << i + 1 << " PASSED !" << endl;
		}else{
			printConf(seqA[i], seqB[i], ws, wd, gap_opening, enlargement);
			cout << "score: " << input_output[i] << " golden_score: " << golden_score[i] << endl;
			cout << "TEST " << i + 1 << " FAILED !!!" << endl;
			count_failed++;
		}
	}

	cout << "Failed tests: " << count_failed << " Passed tests: " << n - count_failed << endl;
	double cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("Execution time: %f ms\n", mean_golden_time * 1e3);

	return 0;
}

//	Prints the current configuration
void printConf(char *seqA, char *seqB, int ws, int wd, int gap_opening, int enlargement) {
	cout << endl << "+++++++++++++++++++++" << endl;
	cout << "+ Sequence A: [" << strlen(seqA) << "]: " << seqA << endl;
	cout << "+ Sequence B: [" << strlen(seqB) << "]: " << seqB << endl;
	cout << "+ Match Score: " << wd << endl;
	cout << "+ Mismatch Score: " << ws << endl;
	cout << "+ Gap Opening: " << gap_opening << endl;
	cout << "+ Enlargement: " << enlargement << endl;
	cout << "+++++++++++++++++++++" << endl;
}

int gen_rnd(int min, int max) {
     // Using random function to get random double value
    return (int) min + rand() % (max - min + 1);
}

void random_seq_gen(int lenA, char *seqA, int lenB, char *seqB) {

	int i, j;
	for(i = 1; i < lenA; i++){
		int tmp_gen = gen_rnd(0, 3);
		seqA[i] = (tmp_gen == 0) ? 'A' :
				  (tmp_gen == 1) ? 'C' :
			      (tmp_gen == 2) ? 'G' : 'T';
	}

	for(i = 1; i < lenB; i++){
		int tmp_gen = gen_rnd(0, 3);
		seqB[i] = (tmp_gen == 0) ? 'A' :
				  (tmp_gen == 1) ? 'C' :
				  (tmp_gen == 2) ? 'G' : 'T';
	}
}

int compute_golden(int lenA, char *seqA, int lenB, char *seqB, int wd, int ws, int gap_opening, int enlargement) {
	// Inizializza le matrici D, P, Q
	    std::vector< std::vector<int> > D(lenA, std::vector<int>(lenB, 0));
	    std::vector< std::vector<int> > P(lenA, std::vector<int>(lenB, std::numeric_limits<int>::min() / 2));
	    std::vector< std::vector<int> > Q(lenA, std::vector<int>(lenB, std::numeric_limits<int>::min() / 2));

	    int gap_penalty = gap_opening + enlargement * 1;
	    int max_score = 0;

	    for (int i = 1; i < lenA; ++i) {
	        for (int j = 1; j < lenB; ++j) {
	            // Calcola P[i][j]
	            P[i][j] = std::max(P[i-1][j] + enlargement, D[i-1][j] + gap_penalty);

	            // Calcola Q[i][j]
	            Q[i][j] = std::max(Q[i][j-1] + enlargement, D[i][j-1] + gap_penalty);

	            // Calcola D[i][j]
	            int match = (seqA[i] == seqB[j]) ? wd : ws;
	            D[i][j] = std::max(0, D[i-1][j-1] + match);
	            D[i][j] = std::max(D[i][j], P[i][j]);
	            D[i][j] = std::max(D[i][j], Q[i][j]);

	            // Aggiorna lo score massimo
	            max_score = std::max(max_score, D[i][j]);
	        }
	    }

	    return max_score;
}

void reverse_str(int len, char *str) {
    int start = 0;
    int end = len;
    while (start < end) {
        std::swap(str[start], str[end]);
        start++;
        end--;
    }
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
