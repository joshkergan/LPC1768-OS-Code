#include "rtx.h"
#include "stress_test.h"
#include "printf.h"

PROC_INIT g_stress_procs[NUM_STRESS_PROCS];

void set_stress_procs(void) {
	int i;
	
	for (i = 0; i < NUM_STRESS_PROCS; i++) {
		g_stress_procs[i].m_stack_size = USR_SZ_STACK;
	}
	
	g_stress_procs[0].m_pid = PID_A;
	g_stress_procs[0].mpf_start_pc = &process_A;
	g_stress_procs[0].m_priority = HIGH;
	
	g_stress_procs[1].m_pid = PID_B;
	g_stress_procs[1].mpf_start_pc = &process_B;
	g_stress_procs[1].m_priority = HIGH;
	
	g_stress_procs[2].m_pid = PID_C;
	g_stress_procs[2].mpf_start_pc = &process_C;
	g_stress_procs[2].m_priority = HIGH;
}

void process_A(void) {
	dprintf("Starting process A\n\r");
	while (1) {
		dprintf("Running process A\n\r");
		release_processor();
	}
}

void process_B(void) {
	//MSG_BUF *message;
	dprintf("Starting process B\n\r");
	while (1) {
		dprintf("Running process B\n\r");
		release_processor();
		//message = receive_message(NULL);
		//send_message(PID_C, message);
	}
}

void process_C(void) {
	dprintf("Starting process C\n\r");
	while (1) {
		dprintf("Running process C\n\r");
		release_processor();
	}
}
