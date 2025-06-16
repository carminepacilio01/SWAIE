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

.PHONY: help

help::
	$(ECHO) "Makefile Usage:"
	$(ECHO) "  make all"
	$(ECHO) ""
	$(ECHO) "  make clean"
	$(ECHO) "      Command to remove all the generated files."
	$(ECHO) ""

.phony: clean

################## software build for XRT Native API code
CXXFLAGS := -std=c++17 -Wno-deprecated-declarations
CXXFLAGS += -I$(XILINX_XRT)/include -I$(XILINX_HLS)/include

LDFLAGS := -L$(XILINX_XRT)/lib 
LDFLAGS += -luuid
LDFLAGS += $(LDFLAGS) -lxrt_coreutil -pthread -lOpenCL -lrt -lstdc++ 
LIB := ../sw/fastareader.cpp

EXECUTABLE := host.exe
XCLBIN := kernel_$(TARGET).xclbin
HOST_SRCS := host.cpp

all: build_sw

build_sw: $(EXECUTABLE)

run_sw:
	./$(EXECUTABLE) $(XCLBIN)

#Eventually add LIBS and CFLAGS
$(EXECUTABLE): $(HOST_SRCS)
	$(CXX) -o $(EXECUTABLE) $^ $(CXXFLAGS) $(LDFLAGS) $(LIB)
	@rm -f ./$(XCLBIN)
	@ln -s ../linking/$(XCLBIN) 

################## clean up
clean:
	$(RM) -r _x .Xil *.ltx *.log *.jou *.info host_overlay.exe *.xo *.xo.* *.str *.xclbin .run *.wdb *.json *.wcfg *.protoinst *.csv
	
