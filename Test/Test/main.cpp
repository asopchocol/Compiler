#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#include "address_map_arm.h"
#include "interrupt_ID.h"
#include "defines.h"

#define HW_REGS_BASE (0xFF200000)
#define HW_REGS_SPAN (0x00DF0000)
#define HW_REGS_MASK (HW_REGS_SPAN-1)
#define HEX_ZERO_THREE 0x20
#define TIMMER_ADDR 0xDEC600

int KEY_hold = 0;

void* HEX_virtual; // Lightweight bridge base address
volatile int* HEX_ptr, * KEY_ptr, * TIME_ptr; // virtual addresses

irq_handler_t irq_handler(int irq, void* dev_id, struct pt_regs* regs)
{

	int press = *(KEY_ptr + 3);
	*(KEY_ptr + 3) = press;

	if (press & KEY3) {
		press = 0;
		while (1) {
			if (press & KEY3) {
				break;
			}
			press = *(KEY_ptr + 3);
			*(KEY_ptr + 3) = press;
		}
	}
	return (irq_handler_t)IRQ_HANDLED;
}

static int __init initialize_pushbutton_handler(void)
{
	int value;
	int hex_conversions[16] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D,
	   0x7D, 0x07, 0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
	int counter = 2000000;

	int s1 = 0;
	int s2 = 0;
	int d1 = 0;
	int d2 = 0;


	// generate a virtual address for the FPGA lightweight bridge
	HEX_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);

	HEX_ptr = HEX_virtual + HEX3_HEX0_BASE; // virtual address for LEDR port
	//*HEX_ptr = HEX_ZERO_THREE; // turn on the leftmost light

	KEY_ptr = HEX_virtual + KEY_BASE; // virtual address for KEY port
	*(KEY_ptr + 3) = 0xF; // Clear the Edgecapture register
	*(KEY_ptr + 2) = 0xF; // Enable IRQ generation for the 4 buttons

	TIME_ptr = HEX_virtual + MPCORE_PRIV_TIMER;
	//*TIME_ptr = TIMMER_ADDR;

	value = request_irq(KEYS_IRQ, (irq_handler_t)irq_handler, IRQF_SHARED,
		"pushbutton_irq_handler", (void*)(irq_handler));

	*(TIME_ptr) = counter;
	*(TIME_ptr + 2) = 0b011;


	while (1) {
		while (*(TIME_ptr + 3) == 0)
			;
		*(TIME_ptr + 3) = 1;
		if (s1 == 10) { s2 += 1; s1 = 0; }
		if (s2 == 10) { d1 += 1; s2 = 0; }
		if (d1 == 10) { d2 += 1; d1 = 0; }
		if (d2 == 6) { d2 = 0; }
		int hex_value = hex_conversions[s1];
		hex_value |= hex_conversions[s2] << 8;
		hex_value |= hex_conversions[d1] << 16;
		hex_value |= hex_conversions[d2] << 24;
		*HEX_ptr = hex_value;
		s1++;
	}

	// Register the interrupt handler.

	return value;
}
static void __exit cleanup_pushbutton_handler(void)
{

	free_irq(KEYS_IRQ, (void*)irq_handler);
}
module_init(initialize_pushbutton_handler);
module_exit(cleanup_pushbutton_handler);
