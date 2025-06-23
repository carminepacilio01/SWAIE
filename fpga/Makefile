# MIT License

# Copyright (c) [2025]

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

ECHO=@echo
MAKEFLAGS += --no-print-directory

.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) ""
	$(ECHO) "  make compile"
	$(ECHO) "      Command to generate xo kernel file"
	$(ECHO) ""
	$(ECHO) "  make compile_clean"
	$(ECHO) "      Command to clean and generate xo kernel file"
	$(ECHO) ""
	$(ECHO) "  make clean"
	$(ECHO) "      Command to remove all the generated files."

PLATFORM ?= xilinx_vck5000_gen4x8_qdma_2_202220_1
# PLATFORM ?= xilinx_vck5000_gen4x8_xdma_2_202220_1

TARGET := hw

.phony: clean

################## hardware build 

XOCCFLAGS := --platform $(PLATFORM) -t $(TARGET)  -s -g

compile: data_reader_$(TARGET).xo output_sink_$(TARGET).xo 

# Use --optimize 3 to enable post-route optimizations. This may improve the bitstream but SIGNIFICANTLY increase compilation time

data_reader_$(TARGET).xo: ./data_reader.cpp
	v++ $(XOCCFLAGS) --kernel data_reader -c -o $@ $<

output_sink_$(TARGET).xo: ./output_sink.cpp
	v++ $(XOCCFLAGS) --kernel output_sink -c -o $@ $<

testbench: testbench/testbench.cpp
	g++ -std=c++17 -g -I. -I$(XILINX_HLS)/include -o testbench/$@.exe $^ ../sw/fastareader.cpp

run_testbench: testbench
	cd testbench && ./testbench.exe 
################## clean and compile
compile_clean: clean compile

################## clean up
clean:
	$(RM) -rf *.xo *.xclbin *.xclbin.info *.xclbin.link_summary *.jou *.log *.xo.compile_summary _x .Xil
