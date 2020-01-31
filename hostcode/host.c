//#define DEBUG
#define _BENCH_DBL
//#define _BENCH_FLT
//#define _BENCH_NDRANGE
#define _BENCH_NDRANGE_111
//#define _BENCH_SINGLETASK

const unsigned int PADDING = 2;
const unsigned int PADDING_Y = 2;
const unsigned int KERNEL_ID = 49;
const unsigned int PIPELINE_WORKITEMS = 0;
const unsigned int PIPELINE_LOOP = 1;
const unsigned int VECTORIZE_HINT = 0;
const unsigned int UNROLL_HINT = 0;
const unsigned int LOCAL_MEMORY = 0;
const unsigned int COMPUTE_UNITS = 1;
const unsigned int WG_SIZE_X = 128;
const unsigned int WG_SIZE_Y = 128;
const unsigned int KERNEL_SIZE = 128;
const unsigned int VEC = 1; // elements in a vector

const unsigned int HALO = 2;

const unsigned int SEED = 13;
const double EPSILON = 0.001f;

const unsigned int MAX_CLK_STATE = 9;
const unsigned int MIN_CLK_STATE = 0;
const unsigned int SIZE_START = 1;
const unsigned int SIZE_END = 8; // 9 = end quadratic

// global variables & implementation includes (lazyness)
unsigned int SIZE_X;
unsigned int SIZE_Y;
unsigned int SIZE_X_IN;
unsigned int SIZE_Y_IN;

#include "../../../host.h"
#include "../../../opencl_impl.c"
#include "../../../benchmark.c"
#include "../../../hardware.c"

// xilinx and bsp specific headers
#include "platform.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_cache_l.h"
#include "xparameters.h"
#include "xtime_l.h"
#include "xil_types.h"

const unsigned int FIXED_LEN = 4096 * 4 / sizeof(_DTYPE);

const unsigned int INPUT_SIZE[][2] = {{32, 32}, {64, 64}, {128, 128}, {256, 256}, 
									{512, 512}, {1024, 1024}, {2048, 2048}, {4096, 4096}};

void malloc_err(const char *msg)
{
	printf("ERROR! %s: not enough memory available\n", msg);
	cleanup_platform();
	exit(1);
}

int main()
{
	init_platform();
	switch_scu(SCU_ON);

	srand(SEED);
	int valid = 0;
	size_t iteration = 0;

	char *result;
	char **list_result = malloc(sizeof(char *));
	list_result[0] = NULL;

	_DTYPE *input = NULL;
	_DTYPE *output = NULL;

	size_t size_elem = sizeof(INPUT_SIZE) / (2 * sizeof(unsigned int));
	size_t max_ddr_capacity = 1024 * 1024 * 1024 / 4; // 1GB / 4 = 256MB
	//printf("ddr: %u, max: %u", max_ddr_capacity, 8192 * 8192 * sizeof(_DTYPE));

	for (size_t y = SIZE_START; y < size_elem && y < SIZE_END; y++)
	{
		SIZE_X = INPUT_SIZE[y][0];
		SIZE_X_IN = SIZE_X + VEC * HALO;
		SIZE_Y = INPUT_SIZE[y][1];
		SIZE_Y_IN = SIZE_Y + HALO;
		if (SIZE_X * SIZE_Y * sizeof(_DTYPE) > max_ddr_capacity
			|| (SIZE_X == SIZE_Y && y)
			|| SIZE_X < WG_SIZE_X           // can't run workgroup on smaller input yet
			|| SIZE_Y < WG_SIZE_Y)
			continue;

		input = malloc(SIZE_X_IN * SIZE_Y_IN * sizeof(_DTYPE));
		output = malloc((SIZE_X + PADDING) * (SIZE_Y + PADDING_Y) * sizeof(_DTYPE));
		if (!input || !output)
			malloc_err("alloc data");

		// initialize compute units:
		interface_t intf[COMPUTE_UNITS];
		unsigned int base_address[COMPUTE_UNITS];
		for (size_t i = 0; i < COMPUTE_UNITS; i++)
		{
			base_address[i] = BASE_ADDRESS + i * INTF_ADDRESS_DIFF;
			Xil_SetTlbAttributes(base_address[i], NON_CACHE_ABLE);
			init_interface(&intf[i], base_address[i]);
			set_buffers(&intf[i], input, output);
		}

		for (int i = MIN_CLK_STATE; i < MAX_CLK_STATE; i++)
		{
			init_problem(input, output);

			//print_matrix(output, SIZE_X + PADDING, SIZE_Y + PADDING);
			// ensure that memory has been written
			Xil_DCacheFlush();

			unsigned int freq = set_frequency_state(i);
			printf("Starting OpenCL kernel execution at %uMHz (%u , %u) ...\n\r", freq, SIZE_X, SIZE_Y);

			XTime tStart, tEnd;
			XTime_GetTime(&tStart);

			kernel_exec(intf, COMPUTE_UNITS);

			XTime_GetTime(&tEnd);

			Xil_DCacheInvalidate();

			valid = validate(input, output);

			double time_s = (double)(tEnd - tStart) / (COUNTS_PER_SECOND / 1); // us *= 1000000
			printf("DONE! (%s) time: %.2fs\n", valid ? "valid" : "fail", time_s);

			result = result_csv(KERNEL_ID, iteration, sizeof(_DTYPE), freq, valid, time_s);
			if (!result)
				malloc_err("alloc result");
			list_result = attach_result(result, list_result, ++iteration);
			if (!list_result)
				malloc_err("alloc list");
		}
#ifdef DEBUG
		print_matrix(output, SIZE_X + PADDING, SIZE_Y + PADDING);
		//print_matrix(output, 32, 32);
		break;
#endif
		free(input);
		input = NULL;
		free(output);
		output = NULL;
	}

	printf("\nBENCHMARK END\n CSV DATA:\n\n");
	cleanup_platform();

	printf("%s\n", CSV_FORMAT);
	print_csv(list_result);
	free_list(list_result);
	return 0;
}
