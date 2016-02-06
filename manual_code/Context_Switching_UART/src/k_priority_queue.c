#include "k_rtx.h"

QUEUE *gp_pqueue;	/* array of queues */

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

	enqueue(&gp_pqueue[process->m_priority], process);
}

/**
 * @brief: find the highest priority blocked process
 */
PCB *find_first_blocked(void) {	
	int i;
	PCB* cur_proc;
	
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
