#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "string.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

int tests_passed = 0;
int tests_failed = 0;
int blockedOnMem = 0;

void test_pass(unsigned char test_num) {
	unsigned char test_string[] = "G016_Test: test n OK\n\r";
	test_string[16] = test_num;
	uart0_put_string(test_string);
	tests_passed++;
}

void test_fail(unsigned char test_num) {
	unsigned char test_string[] = "G016_Test: test m FAIL\n\r";
	test_string[16] = test_num;
	uart0_put_string(test_string);
	tests_failed++;
}

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=USR_SZ_STACK;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[0].m_priority   = HIGH;
	
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[1].m_priority   = LOW;
	
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[2].m_priority   = LOW;
	
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[3].m_priority   = LOW;
	
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[4].m_priority   = LOW;
	
	g_test_procs[5].mpf_start_pc = &proc6;
	g_test_procs[5].m_priority   = LOW;
}

void test_processes(void) {
	
}


/**
 * @brief: a process that prints five uppercase letters
 *         and request a memory block.
 */
void proc1(void)
{
	void *p_mem_blk_1, *p_mem_blk_2;
	int ret_val_1, ret_val_2;
	blockedOnMem = 0;
#ifdef DEBUG_0
	printf("proc1: starting...\n");
#endif
	uart0_put_string("G016_test: START\n\r");
	p_mem_blk_1 = request_memory_block();
	p_mem_blk_2 = request_memory_block();
#ifdef DEBUG_0
	printf("mem_1: 0x%x\n", p_mem_blk_1);
	printf("mem_2: 0x%x\n", p_mem_blk_2);
#endif
	if(p_mem_blk_1 != p_mem_blk_2) {
		test_pass('1');
	} else {
		test_fail('1');
	}
	ret_val_1 = release_memory_block(p_mem_blk_1);
	ret_val_2 = release_memory_block(p_mem_blk_2);
	if(ret_val_1 == RTX_OK && ret_val_2 == RTX_OK) {
		test_pass('2');
	} else {
		test_fail('2');
	}
	while(1) {
		release_processor();
	}
	set_process_priority(PID_P3, MEDIUM);
	set_process_priority(PID_P6, HIGH);
	//set_process_priority(PID_P2, HIGH);
	set_process_priority(PID_P1, LOW);
}

/**
 * @brief: a process that prints five numbers
 *         and then releases a memory block
 */
void proc2(void)
{
	int i = 0;
	int ret_val = 20;
	int priority;
	void *p_mem_blk;
	void *blocks[30];
	blockedOnMem = 0;
	
#ifdef DEBUG_0
			printf("proc2: starting...\n");
#endif
	
	set_process_priority(PID_P1, LOW);
	set_process_priority(PID_P6, MEDIUM);
	set_process_priority(PID_P2, HIGH);
	set_process_priority(PID_P3, LOW);
	priority = get_process_priority(PID_P6);
	if(priority == MEDIUM){
		test_pass('3');
	}
	else{
		test_fail('3');
	}
	
	while (1) {
		if ( i != 0 && i%5 == 0 ) {
			uart0_put_string("\n\r");
#ifdef DEBUG_0
			//printf("proc2: ret_val=%d\n", ret_val);
#endif /* DEBUG_0 */
			if ( ret_val == -1 ) {
				break;
			}
		}		
		blockedOnMem = 1;
		for(i=0; blockedOnMem; i++){
			//requesting memory until it's out
			//printf("requesting mem \n\r");
			printf("requesting mem %d\n\r", i);
			blocks[i]=request_memory_block();
		}
		
		for(i=0;i<28;i++){
			release_memory_block(blocks[i]);
			printf("releasing mem %d\n\r", i);
		}
		
	}
	uart0_put_string("proc2: end of testing\n\r");
	set_process_priority(PID_P2, LOWEST);
	set_process_priority(PID_P4, HIGH);
	while ( 1 ) {
		release_processor();
	}
}

void proc3(void)
{
	void *p_mem_blks[30];
	
#ifdef DEBUG_0
			printf("proc3: starting...\n");
#endif
	set_process_priority(PID_P3, LOW);
#ifdef DEBUG_0
	printf("proc3: done\n");
#endif
	while(1)
		release_processor();
}

void proc4(void)
{
	void *p_mem_blk;
	MSG_BUF *message;
	int send;
	blockedOnMem = 0;
#ifdef DEBUG_0
			printf("proc4: starting...\n");
#endif
	message = request_memory_block();
	strcpy("abc", message->mtext);
	send = send_message(PID_P5, message);
	
	if(send==RTX_OK){
		test_pass('5');
	}
	else{
		test_fail('5');
	}
	
	
#ifdef DEBUG_0
			printf("proc4: finished\n");
#endif
	
	while(1)
		release_processor();
}
void proc5(void)
{
	int i=0;
	blockedOnMem = 0;
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
	void* memBlock;
	
	blockedOnMem = 0;
#ifdef DEBUG_0
			printf("proc6: starting...\n");
#endif
	memBlock = request_memory_block();
	set_process_priority(PID_P2, HIGH);
	set_process_priority(PID_P6, LOW);
	release_processor();
	if(blockedOnMem){
		//this should be the next process running after blocking 2
		test_pass('4');
	}
	else{
		test_fail('4');
	}
	release_memory_block(memBlock);
	
	
	while(1) {
		if ( i < 2 )  {
			uart0_put_string("proc6: \n\r");
		}
		release_processor();
		i++;
	}
}
