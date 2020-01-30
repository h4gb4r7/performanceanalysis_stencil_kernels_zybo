#ifndef _ZYBO_HOST_H
#define _ZYBO_HOST_H

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <inttypes.h>
#include <string.h>

#include "benchmark.h"

#define BASE_ADDRESS 0x43C00000		 // address of interface to first compute unit
#define INTF_ADDRESS_DIFF 0x00010000 // address of interface to next compute unit
#define NON_CACHE_ABLE 0x10c06		 // for setting the value of the control signal non cache able

typedef struct
{
	volatile char *control;
#ifdef _BENCH_NDRANGE
/*
 * NDRange Kernel interface:
 * with 1 output (out) and 1 input (src) signal
 * 
 * Offsets to base address 0x43C00000
 *
 -- 0x00 : Control signals
 --        bit 0  - ap_start (Read/Write/COH)
 --        bit 1  - ap_done (Read/COR)
 --        bit 2  - ap_idle (Read)
 --        bit 3  - ap_ready (Read)
 --        bit 7  - auto_restart (Read/Write)
 --        others - reserved
 -- 0x10 : Data signal of global_size_x
 -- 0x18 : Data signal of global_size_y
 -- 0x20 : Data signal of global_size_z
 -- 0x28 : Data signal of group_id_x
 -- 0x30 : Data signal of group_id_y
 -- 0x38 : Data signal of group_id_z
 -- 0x40 : Data signal of global_offset_x
 -- 0x48 : Data signal of global_offset_y
 -- 0x50 : Data signal of global_offset_z
 -- 0x58 : Data signal of out
 -- 0x60 : Data signal of src
 */
	volatile int *global_size_x;
	volatile int *global_size_y;
	volatile int *global_size_z;
	volatile int *group_id_x;
	volatile int *group_id_y;
	volatile int *group_id_z;
	volatile int *global_offset_x;
	volatile int *global_offset_y;
	volatile int *global_offset_z;
	volatile int *out_addr;
	volatile int *src_addr;
#endif
/*
	NDRange(1,1,1) Hardware Interface
0x10 : Data signal of group_id_x
--        bit 31~0 - group_id_x[31:0] (Read/Write)
-- 0x14 : reserved
-- 0x18 : Data signal of group_id_y
--        bit 31~0 - group_id_y[31:0] (Read/Write)
-- 0x1c : reserved
-- 0x20 : Data signal of group_id_z
--        bit 31~0 - group_id_z[31:0] (Read/Write)
-- 0x24 : reserved
-- 0x28 : Data signal of out_r
--        bit 31~0 - out_r[31:0] (Read/Write)
-- 0x2c : reserved
-- 0x30 : Data signal of src
--        bit 31~0 - src[31:0] (Read/Write)
-- 0x34 : reserved
-- 0x38 : Data signal of out_size
--        bit 31~0 - out_size[31:0] (Read/Write)
*/
#ifdef _BENCH_NDRANGE_111
	volatile int *group_id_x;
	volatile int *group_id_y;
	volatile int *group_id_z;
	volatile int *out_addr;
	volatile int *src_addr;
	volatile int *out_size;
#endif
#ifdef _BENCH_SINGLETASK
/*
 * Single-Task Kernel interface:
 * 1 output (out) and 1 input (src) signal
 * 
 * Offsets to base address 0x43C00000
 *
 -- 0x00 : Control signals
 --        bit 0  - ap_start (Read/Write/COH)
 --        bit 1  - ap_done (Read/COR)
 --        bit 2  - ap_idle (Read)
 --        bit 3  - ap_ready (Read)
 --        bit 7  - auto_restart (Read/Write)
 --        others - reserved
 -- 0x10 : Data signal of local_size_x
 -- 0x18 : Data signal of local_size_y
 -- 0x20 : Data signal of local_size_z
 -- 0x28 : Data signal of out
 -- 0x30 : Data signal of src
 -- 0x38 : Data signal of out_size
 // 0x40 : Data signal of block_row
// 0x48 : Data signal of block_col
 */
	volatile int *local_x;
	volatile int *local_y;
	volatile int *local_z;
	volatile int *out_addr;
	volatile int *src_addr;
	volatile int *out_size;
	volatile int *block_row;
	volatile int *block_col;
#endif
	//volatile int *out_radius;
} interface_t;
//#else
//#error "bench type undefined"


// SCU_Control_Register (0xF8F00000)
// 1 = SCU enable.
// 0 = SCU disable. This is the default setting.
#define SCU_ON 1
#define SCU_OFF 0
#define SCU_CONTROL_REGISTER 0xF8F00000

// Adresses and Values for FPGA Clock manipulation
// Unlock write protection of System Level Control Registers (SLCR)
#define UNLOCK_SLCR_CTRL 0xF8000008
#define UNLOCK_SLCR 0xDF0D
#define LOCK_SLCR_CTRL 0xF8000004
#define LOCK_SLCR 0x767B

#define FPGA0_CLK_CTRL 0XF8000170
#define FPGA0_CLK_DIVISOR0_MASK 0x3f00

void switch_scu(int);
void init_problem();
void nd_range_kernel_exec(interface_t *intf, size_t intf_cnt, size_t offset_x, size_t offset_y);
void init_interface(interface_t *intf, unsigned int base_address);
void set_buffers(interface_t *intf, _DTYPE *input, _DTYPE *output);
int poll_device(interface_t *intf);
void start_device(interface_t *intf);
//void print_matrix(_DTYPE *m, int x, int y);
_DTYPE rand_number();
_DTYPE error(_DTYPE a, _DTYPE b);
int validate(_DTYPE *__restrict a, _DTYPE *__restrict b);
_DTYPE stencil(_DTYPE *__restrict src, int x, int y);
void print_control(interface_t *intf);

#endif