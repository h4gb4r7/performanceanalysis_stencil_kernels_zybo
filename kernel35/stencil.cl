
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_khr_icd : enable
#include <clc.h>

#define COMPUTE_SIZE 16
#define HALO 2 // 2x stencil radius
#define BLOCK_SIZE (COMPUTE_SIZE + HALO)

__attribute__((reqd_work_group_size(BLOCK_SIZE, BLOCK_SIZE, 1)))
__attribute__((vec_type_hint(double)))
__kernel
void stencil(__global double *__restrict out,
		__constant double *__restrict src) {

	__local double buff_in[BLOCK_SIZE][BLOCK_SIZE]__attribute__((xcl_array_partition(cyclic, 2, 0)));

__attribute__((xcl_pipeline_workitems))
{
	int out_size = get_global_size(0);
	int src_size = get_global_size(0) + HALO;
	int global_x = get_global_id(0);
	int global_y = get_global_id(1);
	int local_x = get_local_id(0);
	int local_y = get_local_id(1);
	int block_start_x = COMPUTE_SIZE * get_group_id(0);
	int block_start_y = COMPUTE_SIZE * get_group_id(1);

	// load
	buff_in[local_y][local_x] = src[(block_start_y + local_y) * src_size + block_start_x + local_x];

	barrier(CLK_LOCAL_MEM_FENCE);

	if(local_x == 0 || local_y == 0 || local_x == BLOCK_SIZE -1 || local_y == BLOCK_SIZE -1) return;

	// compute & store
	out[(block_start_y + local_y - 1) * out_size + block_start_x + local_x - 1]
		= 0.2 * (buff_in[local_y ][local_x ]
			+ buff_in[local_y ][local_x + 1]
			+ buff_in[local_y ][local_x - 1]
			+ buff_in[local_y - 1][local_x ]
			+ buff_in[local_y + 1][local_x ]);
}
}
