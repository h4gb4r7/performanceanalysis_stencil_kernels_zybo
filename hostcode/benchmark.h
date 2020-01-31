
#define STENCIL_FLOP 5

#ifdef _BENCH_DBL
#define _DTYPE double
#define DTYPE_STRING "double"
#endif

#ifdef _BENCH_FLT
#define _DTYPE float
#define DTYPE_STRING "float"
#endif

//#elif _BENCH_INT_
//#define _DTYPE int
//#define _DTYPESTRING_ "int"
#ifndef _DTYPE
#error "datatype undefined"
#endif

#ifdef _BENCH_NDRANGE
#define BENCH_TYPE_STRING "NDRange"
#endif

#ifdef _BENCH_NDRANGE_111
#define BENCH_TYPE_STRING "NDRange111"
#endif

#ifndef BENCH_TYPE_STRING
#error "kernel type undefined"
#endif