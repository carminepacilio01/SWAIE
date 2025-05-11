/*
MIT License

Copyright (c) 2023 Paolo Salvatore Galfano, Giuseppe Sorrentino

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
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_uuid.h"
#include "../common/common.h"

// For hw emulation, run in sw directory: source ./setup_emu.sh -s on

#define DEVICE_ID 2

#define arg_reader_input 0
#define arg_reader_scoring 1
#define arg_reader_size 2

#define arg_sink_output 1
#define arg_sink_size 2

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


int main(int argc, char *argv[]) {
    if(argc < 2) {
		return EXIT_FAILURE;
	}


///////////////////////////     LOADING XCLBIN      /////////////////////////// 

    std::string xclbin_file = argv[0];

    // Load xclbin
    std::cout << bold_on << "Loading xclbin file: " << xclbin_file << bold_off << std::endl;
    xrt::device device = xrt::device(DEVICE_ID);
    xrt::uuid xclbin_uuid;
    try {
        xclbin_uuid = xrt::uuid(device.load_xclbin(xclbin_file));
    } catch (const std::exception &e) {
        std::cerr << bold_on << red << "Error loading xclbin: " << e.what() << reset << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << green << "Bitstream Loaded Succesfully!" << reset << std::endl;

///////////////////////////     INITIALIZING THE BOARD     ///////////////////////////  

    // create kernel objects
    xrt::kernel data_reader  = xrt::kernel(device, xclbin_uuid, "setup_aie");
    xrt::kernel output_sink  = xrt::kernel(device, xclbin_uuid, "sink_from_aie");

    // get memory bank groups for device buffer - required for axi master input/ouput
    xrtMemoryGroup bank_output  = output_sink.group_id(arg_sink_output);
    xrtMemoryGroup bank_input  = data_reader.group_id(arg_reader_input);

    // create device buffers - if you have to load some data, here they are
    xrt::bo buffer_setup_aie= xrt::bo(device, size * sizeof(int32_t), xrt::bo::flags::normal, bank_input); 
    xrt::bo buffer_sink_from_aie = xrt::bo(device, size * sizeof(int32_t), xrt::bo::flags::normal, bank_output); 

    // create runner instances
    xrt::run run_data_reader   = xrt::run(data_reader);
    xrt::run run_output_sink = xrt::run(output_sink);

    // set setup_aie kernel arguments
    run_setup_aie.set_arg(arg_setup_aie_size,  size);
    run_setup_aie.set_arg(arg_setup_aie_input, buffer_setup_aie);

    // set sink_from_aie kernel arguments
    run_sink_from_aie.set_arg(arg_sink_from_aie_output, buffer_sink_from_aie);
    run_sink_from_aie.set_arg(arg_sink_from_aie_size, size);

//     // write data into the input buffer
//     buffer_setup_aie.write(nums);
//     buffer_setup_aie.sync(XCL_BO_SYNC_BO_TO_DEVICE);

//     // run the kernel
//     run_sink_from_aie.start();
//     run_setup_aie.start();

//     // wait for the kernel to finish
//     run_setup_aie.wait();
//     run_sink_from_aie.wait();

//     // read the output buffer
//     buffer_sink_from_aie.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
//     int32_t output_buffer[size];
//     buffer_sink_from_aie.read(output_buffer);
}