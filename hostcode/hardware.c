// implementation of Zybo Hardware manipulations
#include "host.h"

/*
SRCSEL 	5:4 	30 	0 	0 	Select the source used to generate the clock: 0x: Source for generated clock is IO PLL. 10: Source for generated clock is ARM PLL. 11: Source for generated clock is DDR PLL.
DIVISOR0 	13:8 	3f00 	8 	800 	Provides the divisor used to divide the source clock to generate the required generated clock frequency. First cascade divider.
DIVISOR1 	25:20 	3f00000 	1 	100000 	Provides the divisor used to divide the source clock to generate the required generated clock frequency. Second cascade divide
FPGA0_CLK_CTRL@0XF8000170 	31:0 	3f03f30 		100800 	PL Clock 0 Output control
*/

// sets and returns actual fpga frequency to a given state
// state ranges from 0 to 8 corresponding to the frequencies listed below
unsigned int set_frequency_state(unsigned int state)
{
    // clk / (DIVISOR0 = 8) = 250MHz (default max freq.)
    // sensibile divisors:
    // 40 : 50MHz
    // 30 : 66MHz
    // 20 : 100MHz
    // 15 : 133MHz
    // 13 : 153MHz
    // 10 : 200MHz
    // 8  : 250MHz
    // 7  : 285MHz
    // 6  : 333MHz
    // 5  : 400MHz
    // 4  : 500MHz
    const unsigned int basefreq = 2000;
    const unsigned int min_state = 0; // 50MHz
    const unsigned int max_state = 10; // 400MHz
    assert(state >= min_state);
    assert(state <= max_state);

    unsigned int divisor_arr[] = {40, 30, 20, 15, 13, 10, 8, 7, 6, 5, 4};
    unsigned int divisor = divisor_arr[state];

    // unlock the clock management:
    uint32_t *unlock_p = (uint32_t *)UNLOCK_SLCR_CTRL;
    *unlock_p = UNLOCK_SLCR;

    // set FPGA Frequencies
    // divisor0 mask FPGA0_CLK_DIVISOR0_MASK = 0x3f00
    // DIVISOR0 Bits: 13:8
    // DIVISOR1 is 1

    uint32_t *fpga_clk_ctrl = (uint32_t *)FPGA0_CLK_CTRL;
    *fpga_clk_ctrl &= ~(FPGA0_CLK_DIVISOR0_MASK); // set bits 8 to 13 to zero
    //printf("FPGA0_CLK_CTRL: %x\n", *fpga_clk_ctrl);

    //*fpga_clk_ctrl = (unsigned int) 0x00000000; // reset clock
    *fpga_clk_ctrl |= divisor << 8; // set clock divisor0

    // lock the clock management:
    uint32_t *lock_p = (uint32_t *)LOCK_SLCR_CTRL;
    *lock_p = LOCK_SLCR;

    return basefreq / divisor;
}

// change ddr pll frequency
double set_ddr_pll(unsigned int state)
{   
    const double basefrequency = 100/3;

    // unlock the clock management:
    uint32_t *unlock_p = (uint32_t *)UNLOCK_SLCR_CTRL;
    *unlock_p = UNLOCK_SLCR;
    //The process of changing clock frequencies is as follows:

    //1. Request the controller to place the DRAM into self refresh mode, by asserting ctrl_reg1.reg_ddrc_selfref_en.
    //const unsigned int DDR_PLL_CFG = 0XF8000114;
    //PLL_RESET 	0:0 	1 	1 	1 	PLL reset control: 0: de-assert (PLL operating) 1: assert (PLL held in reset)

    printf("1\n");
    const unsigned int DDR_PLL_CTRL = 0XF8000104;
    uint32_t *ddr_pll_ctrl_p = (uint32_t *)DDR_PLL_CTRL;
    *ddr_pll_ctrl_p |= 1 << 4; // PLL_BYPASS_FORCE 	4:4 	10 	1
    *ddr_pll_ctrl_p |= 1; // assert reset
    printf("2\n");

    //2. Wait until mode_sts_reg.ddrc_reg_operating_mode[1:0]== 11 indicating that the controller is in self refresh mode. 
    //In the case of LPDDR2 check that ddrc_reg_operating_mode[2:0]== 011.
    const unsigned int PLL_STATUS =	0XF800010C;
    uint32_t *pll_status_p = (uint32_t *)PLL_STATUS;
    //printf("%u", *pll_status_p);
    printf("3\n");
    while(((*pll_status_p) & 0b11) != 0b11) // 0b011
    {
        //printf("not in self refresh mode\n");
        printf("4\n");
        //sleep(1);
    }
    printf("5\n");
    
    //3. Change the clock frequency to the controller (see 10.6.1 Clock Operating Frequencies and 25.10.4 PLLs.).
    // 0x20 is the standart value (x32 = 1066MHz)

    //Field Name 	Bits 	Mask 	Value 	Shifted Value 	Description
    //PLL_FDIV 	18:12 	 	20 	20000 	Provide the feedback divisor for the PLL. Note: Before changing this value, the PLL must first be bypassed and then put into reset mode.
    *ddr_pll_ctrl_p &= ~(0x7f000); // set bits 18:12 to zero
    *ddr_pll_ctrl_p |= state << 12;

    //4. Update any registers which might be required to change for the new frequency. This includes static and dynamic registers. 
    //If the updated registers involve any of reg_ddrc_mr, reg_ddrc_emr, reg_ddrc_emr2 or reg_ddrc_emr3, then go to step 5. Otherwise go to step 6.

    //5. Assert reg_ddrc_soft_rstb to reset the controller. When the controller is taken out of reset, it re-initializes the DRAM. During initialization, 
    //the mode register values updated in step 4 are written to DRAM. Anytime after de-asserting reset, go to step 6.
    *ddr_pll_ctrl_p |= 1; // assert reset // TODO ??

    //6. Take the controller out of self refresh by de-asserting reg_ddrc_selfref_en.
    *ddr_pll_ctrl_p &= ~1; // deassert reset

    //Note:This sequence can be followed in general for changing DDRC settings, in addition to just clock frequencies.
    //Note:DRAM content preservation is not guaranteed when the controller is reset.
    // lock the clock management:
    uint32_t *lock_p = (uint32_t *)LOCK_SLCR_CTRL;
    *lock_p = LOCK_SLCR;

    return basefrequency * state;
}



// set the ddr2x and ddr3x divisors
// __attribute_deprecated__
unsigned int set_ddr_divisors(unsigned int state)
{
/*
Register Name 	Address 	Width 	Type 	Reset Value 	Description
DDR_CLK_CTRL 	0XF8000124 	32 	rw 	0x00000000 	--

Field Name 	Bits 	Mask 	Value 	Shifted Value 	Description
DDR_3XCLKACT 	0:0 	1 	1 	1 	DDR_3x Clock control: 0: disable, 1: enable
DDR_2XCLKACT 	1:1 	2 	1 	2 	DDR_2x Clock control: 0: disable, 1: enable
DDR_3XCLK_DIVISOR 	25:20 	3f00000 	2 	200000 	Frequency divisor for the ddr_3x clock
DDR_2XCLK_DIVISOR 	31:26 	fc000000 	3 	c000000 	Frequency divisor for the ddr_2x clock
DDR_CLK_CTRL@0XF8000124 	31:0 	fff00003 		c200003 	DDR Clock Control

Standart Values:
Clock Source DDRPLL
DDRPLL 1056MHz
DDR PLL CLK DIVISOR0 2
DDR PLL CLK DIVISOR1 N/A
DDR frequency: 528MHz

DDR frequency range: 200-667MHz 
*/
    const unsigned int DDR_CLK_CTRL = 0xF8000124; 
    const unsigned int DDR3X_CLK_DIVISOR_MASK = 0x3f00000; 
    const unsigned int DDR2X_CLK_DIVISOR_MASK = 0xfc000000; 
    const unsigned int DDRPLL = 1056; // MHz

    // unlock the clock management:
    uint32_t *unlock_p = (uint32_t *)UNLOCK_SLCR_CTRL;
    *unlock_p = UNLOCK_SLCR;

    uint32_t *ddr_clk_ctrl = (uint32_t *)DDR_CLK_CTRL;

    *ddr_clk_ctrl |= 1 << 0; // 	DDR_3x Clock control: 1: enable
    *ddr_clk_ctrl |= 1 << 1; // 	DDR_2x Clock control: 1: enable

    *ddr_clk_ctrl &= ~DDR3X_CLK_DIVISOR_MASK; // set bits 25:20 to zero
    *ddr_clk_ctrl &= ~DDR2X_CLK_DIVISOR_MASK; // set bits 31:26 to zero
    //printf("FPGA0_CLK_CTRL: %x\n", *fpga_clk_ctrl);

    unsigned int DDR3X_CLK_DIVISOR;
    unsigned int DDR2X_CLK_DIVISOR;

    state++;
    DDR3X_CLK_DIVISOR = state * 2; // DDR
    DDR2X_CLK_DIVISOR = state * 3; // HP AXI (should be 2/3 of DDR)
    

    *ddr_clk_ctrl |= DDR3X_CLK_DIVISOR << 20; // set ddr3x clock divisor
    *ddr_clk_ctrl |= DDR2X_CLK_DIVISOR << 26; // set ddr2x clock divisor

    *ddr_clk_ctrl |= 0 << 0; // 	DDR_3x Clock control: 0: disable
    *ddr_clk_ctrl |= 0 << 1; // 	DDR_2x Clock control: 0: disable

    // lock the clock management:
    uint32_t *lock_p = (uint32_t *)LOCK_SLCR_CTRL;
    *lock_p = LOCK_SLCR;

    return DDRPLL / DDR3X_CLK_DIVISOR;
}

// switch scu mode to enable insecure memory transaction
void switch_scu(int enable)
{
    switch (enable)
    {
    case SCU_ON:
        *((volatile char *)SCU_CONTROL_REGISTER) |= 1;
        break;
    default:
        *((volatile char *)SCU_CONTROL_REGISTER) &= ~1;
        break;
    }
}
