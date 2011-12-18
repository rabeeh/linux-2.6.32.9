/* IDT clock configurations header file */

#ifndef _INC_IDT5V49EE503_H
#define _INC_IDT5V49EE503_H

/* PLL ID inside the clock chip */
typedef enum 
{
	IDT_PLL_0 = 0,
	IDT_PLL_1 = 1,
	IDT_PLL_2 = 2,
	IDT_PLL_3 = 3,
	IDT_PLL_NUM

} idt_clock_id_t;

/* Output pin ID where the PLL is connected */
typedef enum 
{
	IDT_OUT_ID_0 = 0,
	IDT_OUT_ID_1 = 1,
	IDT_OUT_ID_2 = 2,
	IDT_OUT_ID_3 = 3,
	IDT_OUT_ID_6 = 6,
	IDT_OUT_ID_INV

} idt_output_id_t;

struct idt_data {
	int		clock0_enable; /* set to 1 to enable clock 0*/
	idt_output_id_t clock0_out_id; /* idt output id of clock 0 */
	idt_clock_id_t	clock0_pll_id; /* pll to use for clock 0 */
	int		clock1_enable; /* set to 1 to enable clock 1*/
	idt_output_id_t clock1_out_id; /* idt output id of clock 1 */
	idt_clock_id_t	clock1_pll_id; /* pll to use for clock 1 */
};

#endif /* _INC_IDT5V49EE503_H */
