#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "string.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];

int num_tests = 8;
int tests_passed = 0;
int blockedOnMem = 0;
int prempt = 0;
void *blocks[30] = {NULL};

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
}

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_stack_size=USR_SZ_STACK;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[0].m_priority   = LOW;
	
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
	unsigned char s_tests_passed[] = "G016_test: x/N tests OK\n\r";
	unsigned char s_tests_failed[] = "G016_test: y/N tests FAIL\n\r";
	unsigned char* s_end = "G016_test: END\n\r";
	
	s_tests_passed[11] = (char)(tests_passed + '0');
	s_tests_failed[11] = (char)((num_tests - tests_passed) + '0');
	s_tests_passed[13] = (char)(num_tests + '0');
	s_tests_failed[13] = (char)(num_tests + '0');
	
	uart0_put_string(s_tests_passed);
	uart0_put_string(s_tests_failed);
	uart0_put_string(s_end);
}


/**
 * @brief: Process that tests request and release memory in two blocks
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
	// Switch tests
	set_process_priority(PID_P3, MEDIUM);
	set_process_priority(PID_P2, HIGH);
	set_process_priority(PID_P1, LOW);
	while(1) {
		release_processor();
	}
}

/**
 * @brief: sets get and set process priority
 */
void proc2(void)
{
	int priority;
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
			
	prempt = 1;
	set_process_priority(PID_P6, HIGH);
	set_process_priority(PID_P2, LOW);
	while ( 1 ) {
		release_processor();
	}
}

void proc3(void)
{
	MSG_BUF* message;
	MSG_BUF* message2;
	MSG_BUF* result;
	int pid = PID_P5;
	int pid2 = PID_P3;
	
	message = (MSG_BUF*)receive_message(&pid);
	
	if(prempt)
		test_pass('7');
	else 
		test_fail('7');

	message2 = (MSG_BUF*) request_memory_block();

	message2->mtype = DEFAULT;

	strcpy("Message 2", message2->mtext);

	delayed_send(PID_P3, message, 3000);
	delayed_send(PID_P3, message2, 2000);

	result = receive_message(&pid2);
	if(strcmp("Message 2", result->mtext) == 0) {
		test_pass('8');
	} else {
		test_fail('8');
	}
	result = receive_message((int*)PID_P3);

	test_processes();
	while(1)
		release_processor();
}

void proc4(void)
{
	MSG_BUF *message;
	int pid = PID_P5;
	
	message = (MSG_BUF*) receive_message(&pid);

	if (strcmp(message->mtext, "Correct!") == 0) {
		test_pass('6');
	} else {
		test_fail('6');
	}
	
	set_process_priority(PID_P4, LOW);
	
	while(1)
		release_processor();
}
void proc5(void)
{
	int i=0;
	MSG_BUF* message;
	blockedOnMem = 1;
	
	while(blocks[i]) {
		release_memory_block(blocks[i]);
		i++;
	}
	release_processor();
	if(!prempt) {
		test_fail('5');
	}
	
	set_process_priority(PID_P4, HIGH);
	set_process_priority(PID_P3, HIGH);
	
	message = (MSG_BUF*) request_memory_block();
	
	message->mtype = DEFAULT;
	strcpy("Correct!", message->mtext);
	
	send_message(PID_P4, (void*)message);
	
	prempt = 0;
	set_process_priority(PID_P3, HIGH);
	set_process_priority(PID_P5, MEDIUM);
	
	prempt = 1;
	send_message(PID_P3, message);
	prempt = 0;
	
	set_process_priority(PID_P5, LOW);
	while(1) {
		release_processor();
	}
}

/**
 * @brief: tests premption and blocks on memory
 */
void proc6(void)
{
	int i = 0;
	if (prempt == 1) {
		test_pass('4');
	} else {
		test_fail('4');
	}
	prempt = 0;
	set_process_priority(PID_P5, HIGH);
	while(!blockedOnMem) {
		blocks[i] = request_memory_block();
		i++;
	}
	prempt = 1;
	if(blockedOnMem) {
		test_pass('5');
	}
	set_process_priority(PID_P6, LOW);
	while (1)
		release_processor();
}
