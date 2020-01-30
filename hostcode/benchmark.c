// implementation of benchmark specific functionality
#include "host.h"
const char *CSV_FORMAT = "id,iteration,kernel_type,data_type,dtype_size,frequency,problem_size_bytes,problem_size_x,problem_size_y,wg_size_x,wg_size_y,compute_units,valid_compute,time_us,flops,pipeline_workitems,pipeline_loop,vectorize_hint,unroll_hint,local_memory";

void free_list(char **list)
{
    for (char *iter = *list; NULL != iter; iter++)
    {
        free(iter);
    }
}

void print_csv(char **list)
{
    while (*list)
    {
        printf("%s", *list++);
    }
}

char **attach_result(char *result, char **list, size_t results)
{
    list[results - 1] = result;
    list = reallocarray(list, results + 1, sizeof(char *));
    list[results] = NULL;
    return list;
}

// print the results of one measurement formatted in csv
// csv format is the following:
char *result_csv(unsigned int kernel_id,
                 size_t iteration,
                 size_t dtype_bytes,
                 unsigned int freq,
                 int valid,
                 double time_s)
{
    char buffer[1024];
    snprintf(buffer, 1024, "%d,%d,%s,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%f,%f,%d,%d,%d,%d,%d\n",
             kernel_id,
             iteration,
             BENCH_TYPE_STRING, //kernel type
             DTYPE_STRING,      //data type
             dtype_bytes,
             freq,
             SIZE_X * SIZE_Y * dtype_bytes, //problem size #bytes
             SIZE_X,
             SIZE_Y,
             WG_SIZE_X,
             WG_SIZE_Y,
             COMPUTE_UNITS,
             valid,
             time_s * 1e6,                                    // time[us]
             (double)SIZE_X * SIZE_Y * STENCIL_FLOP / time_s, //FLOPS
             PIPELINE_WORKITEMS,
             PIPELINE_LOOP,
             VECTORIZE_HINT,
             UNROLL_HINT,
             LOCAL_MEMORY);

    return strndup(buffer, 512);
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
            input[i] = rand_number();
        }
    }
    for (size_t i = 0; i < (SIZE_X + 2) * (SIZE_Y + 2); i++)
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

// Randomly generate a _DTYPE number between -10 and 10.
_DTYPE rand_number()
{
    return rand() / (_DTYPE)RAND_MAX * 20.0f - 10.0f;
}

// Absolute difference of a and b
_DTYPE error(_DTYPE a, _DTYPE b)
{
    return a - b > 0 ? a - b : b - a;
}


// Validate output by recalculating the stencil
int validate(double *input, double *output)
{
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
    return 1;
}

// Validate output by recalculating the stencil
int validate2(_DTYPE *__restrict input, _DTYPE *__restrict output)
{
    int valid = 1;
    for (size_t r = 0; r < SIZE_Y; r++)
    {
        for (size_t c = 0; c < SIZE_X; c++)
        {
            if (error(stencil(input, c, r), output[r * (SIZE_X + PADDING) + c]) > EPSILON)
            {
                if (valid)
                    printf("invalid at: row: %li col: %li\n", r, c);
                valid = 0;
            }
        }
    }
    return valid;
}

// recalculate stencil for debug
void calculate_debug(_DTYPE *__restrict input, _DTYPE *__restrict output)
{
    for (size_t i = 0; i < SIZE_X; i++)
    {
        for (size_t j = 0; j < SIZE_Y; j++)
        {
           output[j * (SIZE_X + PADDING) + i] = stencil(input, i, j);
        }
    }
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
