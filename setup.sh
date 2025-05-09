VITIS=/tools/Xilinx/Vitis/2023.2/settings64.sh
XRT=/opt/xilinx/xrt_2023.2/setup.sh

source $VITIS
source $XRT
export LD_LIBRARY_PATH=/opt/xilinx/xrt_2023.2/lib:$LD_LIBRARY_PATH
export XILINX_XRT=/opt/xilinx/xrt_2023.2
export XILINX_VITIS=/tools/Xilinx/Vitis/2023.2