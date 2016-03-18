#include "rtx.h"
#include "stress_test.h"
#include "string.h"
#include "printf.h"

PROC_INIT g_stress_procs[NUM_STRESS_PROCS];

void set_stress_procs(void) {
	int i;
	
	for (i = 0; i < NUM_STRESS_PROCS; i++) {
		g_stress_procs[i].m_stack_size = USR_SZ_STACK;
	}
	
	g_stress_procs[0].m_pid = PID_A;
	g_stress_procs[0].mpf_start_pc = &process_A;
	g_stress_procs[0].m_priority = LOW;
	
	g_stress_procs[1].m_pid = PID_B;
	g_stress_procs[1].mpf_start_pc = &process_B;
	g_stress_procs[1].m_priority = LOW;
	
	g_stress_procs[2].m_pid = PID_C;
	g_stress_procs[2].mpf_start_pc = &process_C;
	g_stress_procs[2].m_priority = MEDIUM;
}

void process_A(void) {
	MSG_BUF *p;
	int sender;
	int num;

	dprintf("Starting process A\n\r");

	p = request_memory_block();
	p->mtype = KCD_REG;
	strcpy("%Z", p->mtext);
	send_message(PID_KCD, p);
	while (1) {
		//dprintf("Running process A\n\r");
		p = receive_message(&sender);
		if (is_substring("%Z", p->mtext)) {
			release_memory_block(p);
			break;
		} else {
			release_memory_block(p);
		}
	}

	num = 0;
	while (1) {
		p = request_memory_block();
		p->mtype = COUNT_REPORT;
		p->M_USER_INT = num;
		send_message(PID_B, p);
		num++;
		release_processor();
	}
}

void process_B(void) {
	MSG_BUF *message;
	dprintf("Starting process B\n\r");
	while (1) {
		//dprintf("Running process B\n\r");
		message = receive_message(NULL);
		send_message(PID_C, message);
	}
}

void process_C(void) {
	MSG_BUF *start = NULL;
	MSG_BUF *end = NULL;
	MSG_BUF *p;
	MSG_BUF *delay;
	int sender;

	dprintf("Starting process C\n\r");

	while (1) {
		//dprintf("Running process C\n\r");
		if (start == NULL) {
			// Queue is empty
			p = receive_message(&sender);
		} else {
			p = start;
			start = start->mp_next;
			if (!start)
				end = NULL;
		}

		if (p->mtype == COUNT_REPORT) {
			if (p->M_USER_INT % 20 == 0) {
				// Send "Process C to the CRT
				strcpy("Process C\n\r", p->mtext);
				send_message(PID_CRT, p);

				// Delay for 10 seconds
				delay = request_memory_block();
				delay->mtype = WAKEUP10;
				delayed_send(PID_C, delay, 10 * ONE_SECOND);

				while (1) {
					p = receive_message(NULL);
					if (p->mtype == WAKEUP10) {
						break;
					} else {
						// Add the message to the queue
						//dprintf("Adding message to queue: num=%d\n\r", p->M_USER_INT);
						p->mp_next = NULL;
						if (!start) {
							start = p;
							end = p;
						} else {
							end->mp_next = p;
							end = p;
						}
					}
				}
			}
		}
		release_memory_block(p);
		release_processor();
	}
}
