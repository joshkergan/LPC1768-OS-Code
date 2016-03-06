/**
 * @file:   usr_proc.c
 * @brief:  Six user processes: proc1...6 to test memory blocking/unblocking 
 * @author: Yiqing Huang
 * @date:   2014/02/07
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 *       The test requires set_process_priority preemption works properly.
 *   
 * Two possible output unde the assumption that 
 * we have TWO memory blocks in the system.
 *
 * Expected UART output: (assuming memory block has ownership.):
 * ABCDE
 * FGHIJ
 * 01234
 * KLMNO
 * 56789
 * proc2: end of testing
 * proc3: 
 * proc4: 
 * proc5: 
 * proc6: 
 * proc3: 
 * proc4: 
 * proc5: 
 * proc6: 
 *
 * Expected UART output: (assuming shared memory among processes (no ownership))
 * ABCDE
 * FGHIJ
 * 01234
 * KLMNO
 * 56789
 * PQRST
 * 01234
 * UVWXY
 * 56789
 * ZABCD
 * 01234
 * ...... you see P1 and P2 keep alternating between each other, p3-p6 will never run
 * 
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=USR_SZ_STACK;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[0].m_priority   = MEDIUM;
	
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[1].m_priority   = HIGH;
	
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[2].m_priority   = LOW;
	
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[3].m_priority   = LOW;
	
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[4].m_priority   = LOW;
	
	g_test_procs[5].mpf_start_pc = &proc6;
	g_test_procs[5].m_priority   = LOW;
}


/**
 * @brief: a process that prints five uppercase letters
 *         and request a memory block.
 */
void proc1(void)
{
	int i = 0;
	void *p_mem_blk;
	
#ifdef DEBUG_0
			printf("proc1: starting...\n");
#endif
	
	while ( 1 ) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
#ifdef DEBUG_0
			//printf("proc1: p_mem_blk=0x%x\n", p_mem_blk);
#endif /* DEBUG_0 */
		}
		uart0_put_char('A' + i%26);
		i++;
	}
}

/**
 * @brief: a process that prints five numbers
 *         and then releases a memory block
 */
void proc2(void)
{
	int i = 0;
	int ret_val = 20;
	void *p_mem_blk;
	
#ifdef DEBUG_0
			printf("proc2: starting...\n");
#endif
	
	set_process_priority(PID_P2, MEDIUM);
	while ( 1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
#ifdef DEBUG_0
			//printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
			if ( ret_val == -1 ) {
				break;
			}
		}
		uart0_put_char('0' + i%10);
		i++;
	}
	uart0_put_string("proc2: end of testing\n\r");
	set_process_priority(PID_P2, LOWEST);
	while ( 1 ) {
		release_processor();
	}
}

void proc3(void)
{
	int i=0;
	void *p_mem_blks[30];
	
#ifdef DEBUG_0
			printf("proc3: starting...\n");
#endif
	
	for (i = 0; i < 30; i++) {
		uart0_put_string("proc3 requesting block\n");
	}
	set_process_priority(4, HIGH);
	set_process_priority(3, MEDIUM);
	for (i = 0; i < 30; i++) {
		uart0_put_string("proc3 releasing block\n");
	}
	
	while(1)
		release_processor();
}

void proc4(void)
{
	void *p_mem_blk;
	
#ifdef DEBUG_0
			printf("proc4: starting...\n");
#endif
	
	uart0_put_string("proc4 running\n");
	uart0_put_string("proc4 asking for memory block\n");
	p_mem_blk = request_memory_block();
	uart0_put_string("proc4 got memory block\n");
	uart0_put_string("proc4 releasing memory block\n");
	release_memory_block(p_mem_blk);
	
	uart0_put_string("proc4 finished\n");
	
	while(1)
		release_processor();
}
void proc5(void)
{
	int i=0;
	
#ifdef DEBUG_0
			printf("proc5: starting...\n");
#endif
	
	while(1) {
		if ( i < 2 )  {
			uart0_put_string("proc5: \n\r");
		}
		release_processor();
		i++;
	}
}
void proc6(void)
{
	int i=0;
	
#ifdef DEBUG_0
			printf("proc6: starting...\n");
#endif
	
	while(1) {
		if ( i < 2 )  {
			uart0_put_string("proc6: \n\r");
		}
		release_processor();
		i++;
	}
}
