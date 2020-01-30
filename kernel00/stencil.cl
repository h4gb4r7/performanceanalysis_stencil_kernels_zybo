#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#include <clc.h>

#define BLOCK_SIZE 32
#define HALO 2 // 2x stencil radius

__attribute__((reqd_work_group_size(BLOCK_SIZE, BLOCK_SIZE, 1)))
__kernel
void
stencil(
                __global double *__restrict out,
                __constant double *__restrict src)
{
        int src_size = get_global_size(0) + HALO;
        int out_size = get_global_size(0);

        int global_x = get_global_id(0);
        int global_y = get_global_id(1);

        out[global_y * out_size + global_x] =
                           0.2 * (src[(global_y + 1) * src_size + global_x]
                                        + src[(global_y + 1) * src_size + global_x + 1]
                                        + src[(global_y + 1) * src_size + global_x + 2]
                                        + src[(global_y) * src_size + global_x + 1]
                                        + src[(global_y + 2) * src_size + global_x + 1]);
}
