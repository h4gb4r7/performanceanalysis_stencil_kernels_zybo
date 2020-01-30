#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_khr_icd : enable
#include <clc.h>

#define COMPUTE_SIZE 64 // 256 requires more than 100% FFs
#define HALO 2 // 2x stencil radius
#define BLOCK_SIZE (COMPUTE_SIZE + HALO)
#define SREG_SIZE (2 * BLOCK_SIZE + 2)

__kernel __attribute__ ((reqd_work_group_size(1, 1, 1)))
void stencil(__global double *__restrict out,
		     __constant double *__restrict src,
		     int out_size)
{
	double sreg[SREG_SIZE]__attribute__((xcl_array_partition(complete, 0)));

	int block_col = get_group_id(0);

	loop4:
	for (int row = 0; row < out_size; row++)
	{
		loop5:
		__attribute__((xcl_pipeline_loop(1)))
		for (int col = 0; col < BLOCK_SIZE; col++)
	    {
			// compute src address
			int addr = row * out_size + block_col * COMPUTE_SIZE + col;

			// load 1
			sreg[SREG_SIZE - 1] = src[addr];

			// compute & store (row=0, row=1, col=0, col=1 will be garbage)
			out[addr] = 0.2 * (
					sreg[0] +
					sreg[BLOCK_SIZE - 1] +
					sreg[BLOCK_SIZE    ] +
					sreg[BLOCK_SIZE + 1] +
					sreg[SREG_SIZE  - 2]);

            // shift
			loop6:
			for (int j = 0; j < SREG_SIZE - 1; j++)
			{
				sreg[j] = sreg[j + 1];
			}
	    }
	}
}
