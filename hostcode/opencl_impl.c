// implementation of opencl and kernel interface specific functionalities
#include "host.h"

// connect the hardware
void init_interface(interface_t *intf, unsigned int base_address)
{
	intf->control = (volatile char *)base_address;
#ifdef _BENCH_NDRANGE
	intf->global_size_x = (volatile int *)(base_address + 0x10);
	intf->global_size_y = (volatile int *)(base_address + 0x18);
	intf->global_size_z = (volatile int *)(base_address + 0x20);
	intf->group_id_x = (volatile int *)(base_address + 0x28);
	intf->group_id_y = (volatile int *)(base_address + 0x30);
	intf->group_id_z = (volatile int *)(base_address + 0x38);
	intf->global_offset_x = (volatile int *)(base_address + 0x40);
	intf->global_offset_y = (volatile int *)(base_address + 0x48);
	intf->global_offset_z = (volatile int *)(base_address + 0x50);
	intf->out_addr = (volatile int *)(base_address + 0x58);
	intf->src_addr = (volatile int *)(base_address + 0x60);
	*(intf->global_size_x) = SIZE_X;
	*(intf->global_size_y) = SIZE_Y;
	*(intf->global_size_z) = 0;
	*(intf->group_id_x) = 0;
	*(intf->group_id_y) = 0;
	*(intf->group_id_z) = 0;
	*(intf->global_offset_x) = 0;
	*(intf->global_offset_y) = 0;
	*(intf->global_offset_z) = 0;
#endif
#ifdef _BENCH_NDRANGE_111
	intf->group_id_x = (volatile int *)(base_address + 0x10);
	intf->group_id_y = (volatile int *)(base_address + 0x18);
	intf->group_id_z = (volatile int *)(base_address + 0x20);
	intf->out_addr = (volatile int *)(base_address + 0x28);
	intf->src_addr = (volatile int *)(base_address + 0x30);
	intf->out_size = (volatile int *)(base_address + 0x38);

	/*
	// used for baseline kernel
	intf->out_addr = (volatile int *)(base_address + 0x10);
	intf->src_addr = (volatile int *)(base_address + 0x18);
	intf->out_size = (volatile int *)(base_address + 0x20);
	*/
	
	*(intf->group_id_x) = 0;
	*(intf->group_id_y) = 0;
	*(intf->group_id_z) = 0;
	
	*(intf->out_size) = SIZE_X + PADDING;
	//printf("setting out_size: %i\n", SIZE_X + PADDING);
#endif
#ifdef _BENCH_SINGLETASK
	intf->local_x = (volatile int *)(base_address + 0x10);
	intf->local_y = (volatile int *)(base_address + 0x18);
	intf->local_z = (volatile int *)(base_address + 0x20);
	intf->out_addr = (volatile int *)(base_address + 0x28);
	intf->src_addr = (volatile int *)(base_address + 0x30);
	intf->out_size = (volatile int *)(base_address + 0x38);
	intf->block_row = (volatile int *)(base_address + 0x40);
	intf->block_col = (volatile int *)(base_address + 0x48);
	*(intf->local_x) = 1; // needs to be 1 for single task
	*(intf->local_y) = 1; // needs to be 1 for single task
	*(intf->local_z) = 1; // needs to be 1 for single task
	*(intf->out_size) = SIZE_X + PADDING; // only quadratic matrices allowed in this implementation
#endif
}

// set input and output buffers
void set_buffers(interface_t *intf, _DTYPE *input, _DTYPE *output)
{
	*(intf->src_addr) = (unsigned int)input;
	*(intf->out_addr) = (unsigned int)output;
}

// Execute intf_cnt Kernels in parallel
// TODO: maybe change logic for idle (also in poll_device)
void kernel_exec(interface_t *intf, size_t intf_cnt)
{
#ifdef _BENCH_SINGLETASK
	assert(!(SIZE_X % KERNEL_SIZE));
	assert(!(SIZE_Y % KERNEL_SIZE));
	/*
	// needed for baseline single-task kernels
	start_device(&intf[0]);
	while (!poll_device(&intf[0]))
			;
	return;
	*/
	int jobs_x = SIZE_X / KERNEL_SIZE;
	int jobs_y = SIZE_Y / KERNEL_SIZE;
	int current_job = 0;
	int job_cnt = jobs_x * jobs_y;

	while (current_job < job_cnt)
	{
		*(intf[0].block_row) = current_job % jobs_x;
		*(intf[0].block_col) = current_job / jobs_x;
		start_device(&intf[0]);
		while (!poll_device(&intf[0]))
				;
		current_job++;
	}
	
#endif

//#else
//#error "bench type undefined"
#ifdef _BENCH_NDRANGE_111
	assert(!(SIZE_X % WG_SIZE_X));
	assert(!(SIZE_Y % WG_SIZE_Y));
	
	/*
	// needed for baseline single-task kernels
	start_device(&intf[0]);
	while (!poll_device(&intf[0]))
			;
	return;
	*/

	int jobs_x = SIZE_X / WG_SIZE_X;
	//int jobs_y = SIZE_Y / WG_SIZE_Y;
	int jobs_y = 1;

	int started[intf_cnt];

	for (size_t i = 0; i < intf_cnt; i++)
	{
		started[i] = 0;
	}
	int current_job = 0;
	int job_cnt = jobs_x * jobs_y;

	while (current_job < job_cnt)
	{
		// check compute unit status and execute kernel if ready
		for (size_t i = 0; i < intf_cnt; i++)
		{
			if (poll_device(&intf[i]) || started[i] == 0)
			{
				started[i] = 1;
				*(intf[i].group_id_x) = jobs_x - (current_job % jobs_x) - 1;
				//printf("group id: %i\n", jobs_x - (current_job % jobs_x) - 1);
				*(intf[i].group_id_y) = current_job / jobs_x;
				start_device(&intf[i]);
				current_job++;
			}
		}
		if (current_job != job_cnt)
			continue;
		// wait for all compute units to finish when all jobs have been scheduled
		for (size_t i = 0; i < intf_cnt; i++)
		{
			if (!started[i])
				continue;
			else
				while (!poll_device(&intf[i]))
					;
		}
	}

#endif

#ifdef _BENCH_NDRANGE
	assert(!(SIZE_X % WG_SIZE_X));
	assert(!(SIZE_Y % WG_SIZE_Y));
	int jobs_x = SIZE_X / WG_SIZE_X;
	int jobs_y = SIZE_Y / WG_SIZE_Y;

	int started[intf_cnt];

	for (size_t i = 0; i < intf_cnt; i++)
	{
		started[i] = 0;
	}
	int current_job = 0;
	int job_cnt = jobs_x * jobs_y;

	while (current_job < job_cnt)
	{
		// check compute unit status and execute kernel if ready
		for (size_t i = 0; i < intf_cnt; i++)
		{
			if (poll_device(&intf[i]) || started[i] == 0)
			{
				started[i] = 1;
				*(intf[i].group_id_x) = current_job % jobs_x;
				*(intf[i].group_id_y) = current_job / jobs_x;
				start_device(&intf[i]);
				current_job++;
			}
		}
		if (current_job != job_cnt)
			continue;
		// wait for all compute units to finish when all jobs have been scheduled
		for (size_t i = 0; i < intf_cnt; i++)
		{
			if (!started[i])
				continue;
			else
				while (!poll_device(&intf[i]))
					;
		}
	}

#endif
}

void print_control(interface_t *intf)
{
	printf("Start: %i Done: %i Idle: %i Ready: %i\n",
		   *(intf->control) & 1,  // bit 0  - ap_start (Read/Write/COH)
		   *(intf->control) & 2,  // bit 1  - ap_done (Read/COR)
		   *(intf->control) & 4,  // bit 2  - ap_idle (Read)
		   *(intf->control) & 8); // bit 3  - ap_ready (Read)
}

int poll_device(interface_t *intf)
{
	// bit 2 is set when done, bit 3 is set when idle (not started yet)
	return *(intf->control) & 2;
}

void start_device(interface_t *intf)
{
	*(intf->control) = *(intf->control) | 1; // set bit 1 to start FPGA
}
