#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_khr_icd : enable
#include <clc.h>

#define COMPUTE_SIZE 64
#define HALO 2 // 2x stencil radius
#define BLOCK_SIZE (COMPUTE_SIZE + HALO)
#define SREG_SIZE (2 * BLOCK_SIZE + 1)

__kernel
__attribute__ ((xcl_dataflow))
__attribute__ ((reqd_work_group_size(1, 1, 1)))
void stencil(__global double *__restrict out, __constant double *__restrict src,
                int out_size) {
        double buff_in[BLOCK_SIZE][BLOCK_SIZE]__attribute__((xcl_array_partition(cyclic, 2, 0)));
        double buff_out[COMPUTE_SIZE * COMPUTE_SIZE];

        const int src_size = out_size + HALO;

        int block_col = get_group_id(0);
        int block_row = get_group_id(1);

        loop0: for (int row = 0; row < BLOCK_SIZE; row++) {
                __attribute__ ((xcl_pipeline_loop(1)))
                loop1: for (int col = 0; col < BLOCK_SIZE; col++) {
                        // compute src address
                        int src_addr = (block_row * COMPUTE_SIZE + row) * src_size
                                        + block_col * COMPUTE_SIZE + col;

                        buff_in[row][col] = src[src_addr];
                }
        }

        loop2: for (int row = 0; row < COMPUTE_SIZE; row++) {
                __attribute__ ((xcl_pipeline_loop(1)))
                loop3: for (int col = 0; col < COMPUTE_SIZE; col++) {
                        // compute & store
                        buff_out[row * COMPUTE_SIZE + col] = (double) 0.2
                                        * (buff_in[row + 1][col] + buff_in[row + 1][col + 1]
                                                        + buff_in[row + 1][col + 2] + buff_in[row][col + 1]
                                                        + buff_in[row + 2][col + 1]);
                }
        }

        loop4: for (int row = 0; row < COMPUTE_SIZE; row++) {
                __attribute__ ((xcl_pipeline_loop(1)))
                loop5: for (int col = 0; col < COMPUTE_SIZE; col++) {
                        // compute out address
                        int out_addr = (block_row * COMPUTE_SIZE + row) * out_size
                                        + block_col * COMPUTE_SIZE + col;

                        // burst write all rows
                        out[out_addr] = buff_out[row * COMPUTE_SIZE + col];
                }
        }
}
