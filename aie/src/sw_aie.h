#pragma once
#include <adf.h>

void compute_sw(input_stream<int32_t>* restrict in_target, input_stream<int32_t>* restrict in_database, 
    output_stream<int32_t>* restrict output);