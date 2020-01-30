//#include "host.h"
#include <stdio.h>
#include <limits.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#define COMPUTE_SIZE 64 // 64 needs 19k LUTs
#define HALO 2          // 2x stencil radius
#define BLOCK_SIZE (COMPUTE_SIZE + HALO)
#define SREG_SIZE (2 * BLOCK_SIZE + 2)
#define EPSILON 0.0001f

#define SIZE_X (8 * COMPUTE_SIZE)
#define SIZE_Y (8 * COMPUTE_SIZE)
#define SIZE_X_IN (SIZE_X + HALO)
#define SIZE_Y_IN (SIZE_Y + HALO)

#define PADDING 0

#define _DTYPE double

// Absolute difference of a and b
_DTYPE error(_DTYPE a, _DTYPE b)
{
    return a - b > 0 ? a - b : b - a;
}

// Randomly generate a _DTYPE number between -10 and 10.
_DTYPE rand_number()
{
    return rand() / (_DTYPE)RAND_MAX * 20.0f - 10.0f;
}

// Initialize the data for the problem.
void init_problem(_DTYPE *input, _DTYPE *output)
{
    //printf("Generating data...\n\r");
    for (size_t i = 0; i < SIZE_X_IN * SIZE_Y_IN; i++)
    {
        if (i % SIZE_X_IN == 0 || i % SIZE_X_IN == SIZE_X_IN - 1 || i <= SIZE_X_IN - 1 || i >= SIZE_X_IN * (SIZE_Y_IN - 1))
        {
            input[i] = 0.0f;
        }
        else
        {
            input[i] = i * .1f;
        }
    }
    for (size_t i = 0; i < SIZE_X * SIZE_Y; i++)
    {
        output[i] = 0.0f;
    }
}

// 2d Jacobi Stencil
_DTYPE stencil(_DTYPE *__restrict src, int x, int y)
{
    const int halo = 2;
    const int src_radius = SIZE_X + halo;

    _DTYPE stencil = (src[(y + 1) * src_radius + x] + src[(y + 1) * src_radius + x + 1] + src[(y + 1) * src_radius + x + 2] + src[(y)*src_radius + x + 1] + src[(y + 2) * src_radius + x + 1]) * 0.2;

    return stencil;
}

// Print matrix m of size x * y
void print_matrix(_DTYPE *m, unsigned int x, unsigned int y)
{
    for (size_t r = 0; r < x; r++)
    {
        for (size_t c = 0; c < y; c++)
        {
            printf("%*.1f ", 4, m[r * x + c]);
        }
        printf("\n");
    }
}

// Validate output by recalculating the stencil
int validate(double *input, double *output)
{
    int valid = 1;
    for (size_t r = 0; r < SIZE_Y; r++)
    {
        for (size_t c = 0; c < SIZE_X; c++)
        {
            if (error(stencil(input, c, r), output[(r + 2) * (SIZE_X + 2) + c + 2]) > EPSILON)
            {
                printf("invalid at: row: %li col: %li\n", r, c);
                return 0;
            }
        }
    }
    return valid;
}

int main()
{
    srand(13);

    const int out_size = SIZE_X + 2;
    const int blocks = SIZE_X / COMPUTE_SIZE;


    double src[SIZE_X_IN * SIZE_X_IN];
    double out[(SIZE_X + 2) * (SIZE_X + 2)];

    init_problem(src, out);

    for (int block_col = blocks - 1; block_col >= 0; block_col--)
{
    double sreg[SREG_SIZE];

	const int src_size = out_size;

	loop4:
	for (int row = 0; row < src_size; row++)
	{
		loop5:
		for (int col = 0; col < BLOCK_SIZE; col++)
	    {
			// compute src address
			int addr = row * src_size + block_col * COMPUTE_SIZE + col;

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

    printf("input:\n");
    print_matrix(src, SIZE_X_IN, SIZE_Y_IN);
    printf("output:\n");
    print_matrix(out, SIZE_X, SIZE_Y + 2);
    //print_matrix(_DTYPE *m, int x, int y);

    int valid = validate(src, out);
    printf("%s\n", valid ? "valid" : "invalid");
    
}