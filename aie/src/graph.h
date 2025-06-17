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

#pragma once
#include <adf.h>
#include "sw_aie.h"

#define NUM_TILES 8

using namespace adf;

class my_graph: public graph
{

private:
	// ------kernel declaration------
	kernel sw_aie[NUM_TILES];

public:
	// ------Input and Output PLIO declaration------
	input_plio in_target[NUM_TILES];
	input_plio in_database[NUM_TILES];
	output_plio out[NUM_TILES];

	my_graph()
	{
		for (int i = 0; i < NUM_TILES; i++) {
			// ------kernel creation------
			sw_aie[i] = kernel::create(compute_sw);

			// Create PLIOs (optional: or use shared input)
			std::string in_tr_name  = "in_target_"  + std::to_string(i);
			std::string in_db_name  = "in_database_"  + std::to_string(i);
			std::string out_name = "out_" + std::to_string(i);

			in_target[i] = input_plio::create(in_tr_name, plio_32_bits, "data/input" + std::to_string(i) + ".txt");
			in_database[i] = input_plio::create(in_db_name, plio_32_bits, "data/input" + std::to_string(i) + ".txt");
			out[i] = output_plio::create(out_name, plio_32_bits, "data/output" + std::to_string(i) + ".txt");

			// ------kernel connection------
			// it is possible to have stream or window. This is just an example. Try both to see the difference
			connect<stream>(in_target[i].out[0], sw_aie[i].in[0]);
			connect<stream>(in_database[i].out[0], sw_aie[i].in[1]);
			connect<stream>(sw_aie[i].out[0], out[i].in[0]);

			// set kernel source and headers
			source(sw_aie[i]) = "src/sw_aie.cpp";
			headers(sw_aie[i]) = {"src/sw_aie.h","../common/common.h"};
			
			// set ratio
			runtime<ratio>(sw_aie[i]) = 0.9; // 90% of the time the kernel will be executed. This means that 1 AIE will be able to execute just 1 Kernel
		}

	};

};
