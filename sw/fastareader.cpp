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

#include "../common/fastareader.h"

namespace fastareader {

    std::string toString(const std::vector<alphabet_datatype>& seq) {
        const std::string alphabet = "ACGT";
        std::string result;
        result.reserve(seq.size());

        for (alphabet_datatype base : seq) {
            result.push_back(alphabet[base]);
        }

        return result;
    }

    std::pair< std::vector<std::vector<alphabet_datatype>>, std::vector<std::vector<alphabet_datatype>> > readFastaFile(const std::string& filename) {
        std::vector<std::vector<alphabet_datatype>> target;
        target.reserve(INPUT_SIZE * MAX_DIM);
        std::vector<std::vector<alphabet_datatype>> database;
        database.reserve(INPUT_SIZE * MAX_DIM);

        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "[FASTA READER] Error opening file: " << filename << std::endl;
            abort();
        }

        std::cout << "[FASTA READER] Begin to read FASTA from file: " << filename << std::endl;

        std::string line;
        std::vector<alphabet_datatype> currSeq;
        short seqCounter = 0;
        for(size_t i = 1; i < INPUT_SIZE * 2 + 1; ++i) {
            std::getline(file, line);
            if (line[0] == '>') {
                i--;
            } else {
                while (currSeq.size() < SEQ_SIZE) {
                    for (char c : line) {
                        currSeq.push_back(compression(c));
                    }
                    std::getline(file, line);
                }

                currSeq.push_back(4);
                currSeq.push_back(4);

                if(seqCounter == 0) {
                    target.push_back(currSeq);
                }
                else{
                    database.push_back(currSeq);
                }
                
                seqCounter = (seqCounter == 0) ? 1 : 0;
                currSeq.clear();
            }
            std::cout << "\r[FASTA READER] Reading: ";
            showProgressBar(i, INPUT_SIZE * 2);
        }
        std::cout << std::endl;

        std::cout << "[FASTA READER] Succesfully read " << INPUT_SIZE  << " sequences pairs." << std::endl;
        file.close();

        return {target, database}; 
    }

    alphabet_datatype compression(char letter) {
        switch (letter) {
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
    
    void showProgressBar(int progress, int total) {
    struct winsize w;
    int barWidth;

    // STDOUT_FILENO is the file descriptor for stdout
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        perror("ioctl");
        barWidth = 25;
    } else {
        barWidth = w.ws_col - 100;
    }

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
} // namespace fastareader

