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

.PHONY: help all build_fpga compile_sw  pack build clean clean_aie clean_FPGA clean_hw clean_sw

help:
	@echo "Makefile Usage:"
	@echo "  make build_hw [TARGET=<hw|hw_emu>] SHELL_NAME=<qdma|xdma>"
	@echo ""
	@echo "  make build_sw SHELL_NAME=<qdma|xdma>"
	@echo ""
	@echo "  make clean"
	@echo ""

PLATFORM ?= xilinx_vck5000_gen4x8_qdma_2_202220_1
TARGET ?= hw

test:
	@echo "TARGET: $(TARGET)"
	@echo "SHELL_NAME: $(SHELL_NAME)"
	@echo "PLATFORM: $(PLATFORM)"
#

#
## Build (xclbin) objects for TARGET
compile: build_fpga compile_aie hw_link compile_sw
#
compile_aie:
	@make -C ./aie aie_compile SHELL_NAME=$(SHELL_NAME)
#
build_fpga:
	@make -C ./fpga compile TARGET=$(TARGET) PLATFORM=$(PLATFORM) SHELL_NAME=$(SHELL_NAME)
#
hw_link:
	@make -C ./linking all TARGET=$(TARGET) PLATFORM=$(PLATFORM) SHELL_NAME=$(SHELL_NAME)
#
## Build software object
compile_sw: 
	@make -C ./sw all 
#

NAME := $(TARGET)_build
#
pack:
	mkdir -p build
	mkdir -p build/$(NAME)
	@cp sw/host.exe build/$(NAME)/
	@cp linking/kernel_$(TARGET).xclbin build/$(NAME)/
#
build:
	@echo ""
	@echo "*********************** Building ***********************"
	@echo "- NAME          $(NAME)"
	@echo "- TARGET        $(TARGET)"
	@echo "- PLATFORM      $(PLATFORM)"
	@echo "- SHELL_NAME    $(SHELL_NAME)"
	@echo "********************************************************"
	@echo ""
	@make compile
	@make pack

# Clean objects
clean: clean_aie clean_fpga clean_hw clean_sw

clean_aie:
	@make -C ./aie clean

clean_fpga:
	@make -C ./fpga clean

clean_hw:
	@make -C ./linking clean

clean_sw: 
	@make -C ./sw clean
