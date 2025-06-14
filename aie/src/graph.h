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

using namespace adf;

class my_graph: public graph
{

private:
	// ------kernel declaration------
	kernel sw_aie;

public:
	// ------Input and Output PLIO declaration------
	input_plio in_target;
	input_plio in_database;
	output_plio out_1;

	my_graph()
	{
		// ------kernel creation------
		sw_aie = kernel::create(compute_sw); // the input is the kernel function name

		// ------Input and Output PLIO creation------
		// I argument: a name, that will be used to refer to the port in the block design
		// II argument: the type of the PLIO that will be read/written. Test both plio_32_bits and plio_128_bits to verify the difference
		// III argument: the path to the file that will be read/written for simulation

		in_target = input_plio::create("in_target", plio_32_bits, "data/in_target.txt");
		in_database = input_plio::create("in_database", plio_32_bits, "data/in_database.txt");
		out_1 = output_plio::create("out_plio_1", plio_32_bits, "data/out_score.txt");

		// ------kernel connection------
		// it is possible to have stream or window. This is just an example. Try both to see the difference
		connect<stream>(in_target.out[0], sw_aie.in[0]);
		connect<stream>(in_database.out[0], sw_aie.in[1]);
		connect<stream>(sw_aie.out[0], out_1.in[0]);
		// set kernel source and headers
		source(sw_aie)  = "src/sw_aie.cpp";
		headers(sw_aie) = {"src/sw_aie.h","../common/common.h"};// you can specify more than one header to include

		// set ratio
		runtime<ratio>(sw_aie) = 0.9; // 90% of the time the kernel will be executed. This means that 1 AIE will be able to execute just 1 Kernel
	};

};
