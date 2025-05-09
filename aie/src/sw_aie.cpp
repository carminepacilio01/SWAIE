#include "sw_aie.h"
#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

//API REFERENCE for STREAM: 
// https://docs.amd.com/r/ehttps://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Reading-and-Advancing-an-Input-Streamn-US/ug1079-ai-engine-kernel-coding/Reading-and-Advancing-an-Input-Stream

void compute_sw(input_stream<int32_t>* restrict input, output_stream<int32_t>* restrict output)
{
    aie::vector<int32_t,4> x= readincr_v4(input);
    int res = x[0];
    writeincr(output,res);
}