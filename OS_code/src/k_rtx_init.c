/** 
 * @file:   k_rtx_init.c
 * @brief:  Kernel initialization C file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_rtx_init.h"
#include "uart_polling.h"
#include "uart.h"
#include "timer.h"
#include "k_memory.h"
#include "k_process.h"

void k_rtx_init(void)
{
	__disable_irq();
	timer_init(0); /* initialize timer 0 */
	timer_init(1);

	uart_irq_init(0);   // uart0, interrupt-driven 
	uart1_init();       // uart1, polling
	memory_init();
	process_init();
	__enable_irq();
	
	uart1_put_string("Initialization done\n\r"); 
	/* start the first process */	
	k_release_processor();
	__enable_irq();
}
