#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#include <clc.h>

#define BLOCK_SIZE 32
#define HALO 2 // 2x stencil radius
#define STENCIL_SIZE 3

__attribute__((reqd_work_group_size(BLOCK_SIZE, BLOCK_SIZE, 1)))
__kernel
void stencil(__global double *__restrict out,
		__constant double *__restrict src) {

	__local double buff_in[BLOCK_SIZE + HALO][BLOCK_SIZE + HALO];

	int src_size = get_global_size(0) + HALO;
	int out_size = get_global_size(0);

	int global_x = get_global_id(0);
	int global_y = get_global_id(1);

	int local_x = get_local_id(0);
	int local_y = get_local_id(1);

    int group_x = get_group_id(0);
    int group_y = get_group_id(1);

    int block_start_x = BLOCK_SIZE * group_x;
    int block_start_y = BLOCK_SIZE * group_y;

    // fetch all data
    if(local_x < (BLOCK_SIZE + HALO) / 2 && local_y < (BLOCK_SIZE + HALO) / 2)
    {
        buff_in[2*local_y][2*local_x]     = src[(block_start_y + 2 * local_y) * src_size + block_start_x + 2 * local_x];
        buff_in[2*local_y][2*local_x+1]   = src[(block_start_y + 2 * local_y) * src_size + block_start_x + 2 * local_x + 1];
        buff_in[2*local_y+1][2*local_x]   = src[(block_start_y + 2 * local_y + 1) * src_size + block_start_x + 2 * local_x];
        buff_in[2*local_y+1][2*local_x+1] = src[(block_start_y + 2 * local_y + 1) * src_size + block_start_x + 2 * local_x + 1];
    }

	barrier(CLK_LOCAL_MEM_FENCE);

	out[global_y * out_size + global_x] =
				   0.2 * (buff_in[local_y + 1][local_x    ]
						+ buff_in[local_y + 1][local_x + 1]
						+ buff_in[local_y + 1][local_x + 2]
						+ buff_in[local_y    ][local_x + 1]
						+ buff_in[local_y + 2][local_x + 1]);
}
