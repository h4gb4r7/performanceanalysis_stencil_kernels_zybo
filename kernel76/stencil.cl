#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_khr_icd : enable
#include <clc.h>

#define COMPUTE_SIZE 128 // 256 requires more than 100% FFs
#define HALO 2 // 2x stencil radius
#define BLOCK_SIZE (COMPUTE_SIZE + HALO)
#define SREG_SIZE (2 * BLOCK_SIZE + 2)

__kernel __attribute__ ((reqd_work_group_size(1, 1, 1)))
void stencil(__global double *__restrict out,
                     __constant double *__restrict src,
                     int out_size)
{
        int block_col = get_group_id(0);

        loop4:
        for (int row = 0; row < out_size; row++)
        {
                loop5:
                __attribute__((xcl_pipeline_loop(1)))
                for (int col = 0; col < COMPUTE_SIZE; col++)
            {
                        // compute src address
                        int addr = row * out_size + block_col * COMPUTE_SIZE + col;
                        // copy
                        out[addr] = src[addr];
            }
        }
}
