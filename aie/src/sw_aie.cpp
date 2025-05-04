#include "my_kernel_1.h"
#include "common.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

//API REFERENCE for STREAM: 
// https://docs.amd.com/r/ehttps://docs.amd.com/r/en-US/ug1079-ai-engine-kernel-coding/Reading-and-Advancing-an-Input-Streamn-US/ug1079-ai-engine-kernel-coding/Reading-and-Advancing-an-Input-Stream

void cpmpute_sw(input_stream<int32_t>* restrict input, output_stream<int32_t>* restrict output)
{
    // read from one stream and write to another
    aie::vector<int32_t,4> x= readincr_v4(input); // the first number tells me how many loops I have to perform
    int res = x[0];
    writeincr(output,res);
}