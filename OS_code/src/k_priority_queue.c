#ifndef K_PRIORITY_QUEUE_H
#define K_PRIORITY_QUEUE_H

#include "k_rtx.h"
#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

QUEUE *gp_pqueue;	/* array of queues */
QUEUE g_sys_pqueue;	/* queue for system processes */

/**
 * @brief: add a PCB to the back of the queue
 */
void enqueue(QUEUE *q, PCB *program) {
	if (!q || !program)
		return;
	
	program->mp_next = NULL;
	
	if (!q->first || !q->last) {
		q->first = program;
		q->last = program;
		return;
	}
	
	q->last->mp_next = program;
	q->last = program;
}

/**
 * @brief: remove a PCB from the front of the queue
 */
PCB * dequeue(QUEUE *q) {
	PCB* to_ret;
	
	if (!q)
		return NULL;
	
	if (!q->first || !q->last)
		return NULL;
	
	to_ret = q->first;
	q->first = to_ret->mp_next;
	if (!q->first)
		q->last = NULL;
	
	return to_ret;
}


/**
 * @brief: remove process from ready queue
 * pops a PCB from the ready queue and returns a pointer to the PCB
 */
PCB * remove_by_PID(int process_id) {
	int priority;
	QUEUE *q;
	PCB *prev;
	PCB *cur;
	
	// First check system queue
	q = &g_sys_pqueue;
	if (q->first->m_pid == process_id)
		return dequeue(q);
	
	prev = q->first;
	cur = prev->mp_next;
	while (cur) {
		if (cur->m_pid == process_id) {
			// Found the process, remove it from the queue
			prev->mp_next = cur->mp_next;
			if (q->last == cur)
				q->last = prev;
			
			return cur;
		}
		
		prev = cur;
		cur = cur->mp_next;
	}
	
	// Check user queues
	for (priority = 0; priority < NUM_PRIORITIES; priority++) {
		q = &gp_pqueue[priority];
		if (q->first->m_pid == process_id)
			return dequeue(q);
		
		prev = q->first;
		cur = prev->mp_next;
		while (cur) {
			if (cur->m_pid == process_id) {
				// Found the process, remove it from the queue
				prev->mp_next = cur->mp_next;
				if (q->last == cur)
					q->last = prev;
				
				return cur;
			}
			
			prev = cur;
			cur = cur->mp_next;
		}
	}
	
	// Could not find PID
	return NULL;
}

void add_to_priority_queue(PCB *process) {
	if (!process)
		return;
	
	// Never add the null process
	if (process->m_pid == 0)
		return;
	
	switch (process->m_pid) {
		case PID_CRT:
		case PID_KCD:
			enqueue(&g_sys_pqueue, process);
			break;
		default:
			enqueue(&gp_pqueue[process->m_priority], process);
	}
}

/**
 * @brief: find the highest priority blocked process
 */
PCB *find_first_blocked(void) {	
	int i;
	PCB* cur_proc;
	
	// First check system queue
	cur_proc = g_sys_pqueue.first;
	while (cur_proc) {
		if (cur_proc->m_state == BLOCKED)
			return cur_proc;
		cur_proc = cur_proc->mp_next;
	}
	
	// Check user processes
	for (i = 0; i < NUM_PRIORITIES; i++) {
		cur_proc = gp_pqueue[i].first;
		while (cur_proc) {
			if (cur_proc->m_state == BLOCKED)
				return cur_proc;
			cur_proc = cur_proc->mp_next;
		}
	}
	
	return NULL;
}

PCB * find_first_ready(void) {
	int i;
	
	// First check system queue
	PCB *cur_proc = g_sys_pqueue.first;
	while (cur_proc) {
		if (cur_proc->m_state == RDY || cur_proc->m_state == NEW)
			return cur_proc;
		cur_proc = cur_proc->mp_next;
	}
	
	// Check user queues
	for (i = 0; i < NUM_PRIORITIES; i++) {
		PCB *cur_proc = gp_pqueue[i].first;
		while (cur_proc) {
			if (cur_proc->m_state == RDY || cur_proc->m_state == NEW)
				return cur_proc;
			cur_proc = cur_proc->mp_next;
		}
	}
	
	return NULL;
}

#ifdef _DEBUG_HOTKEYS
void print_ready(void) {
	int i;
	printf("Ready processes:\n");
	for (i = 0; i < NUM_PRIORITIES; i++) {
		PCB *cur_proc = gp_pqueue[i].first;
		printf("priority %d: ", i);
		while (cur_proc) {
			if (cur_proc->m_state == RDY || cur_proc->m_state == NEW) {
				printf("%d,", cur_proc->m_pid);
			}
			
			cur_proc = cur_proc->mp_next;
		}
		printf("\n");
	}
}

void print_mem_blocked(void) {
	int i;
	printf("Memory blocked processes:\n");
	for (i = 0; i < NUM_PRIORITIES; i++) {
		PCB *cur_proc = gp_pqueue[i].first;
		printf("priority %d: ", i);
		while (cur_proc) {
			if (cur_proc->m_state == BLOCKED) {
				printf("%d,", cur_proc->m_pid);
			}
			
			cur_proc = cur_proc->mp_next;
		}
		printf("\n");
	}
}

void print_receive_blocked(void) {
	
}
#endif /* _DEBUG_HOTKEYS */

#endif /* K_PRIORITY_QUEUE_H */
