/**
 * @file:   k_process.c  
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/02/28
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS. 
 *       The purpose is to show how context switch could be done under stated assumptions. 
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations. 
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB **gp_pcbs;                  /* array of pcbs */
PCB *gp_current_process = NULL; /* always point to the current RUN process */
QUEUE *gp_pqueue;								/* array of queues */
U32 g_num_blocked = 0;					/* the number of blocked processes */
U32 g_released_memory = 0;

U32 g_switch_flag = 0;          /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
				/* this value will be set by UART handler */

/* process initialization table */
PROC_INIT g_proc_table[NUM_TEST_PROCS + 1]; /* holds init info for null process and all test processes */
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

/**
 * @brief: add a PCB to the back of the queue
 */
 
void enqueue(QUEUE *q, PCB *program) {
	if (!q || !program)
		return;
	
	if (program->m_pid == 0)
		uart0_put_string("Success!");
	
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

int get_process_priority(int);

/**
 * @brief: find the highest priority blocked process
 */
PCB * find_first_blocked(void) {	
	int i;
	PCB* cur_program;
	
	for (i = 0; i < NUM_PRIORITIES; i++) {
		cur_program = gp_pqueue[i].first;
		while (cur_program) {
			if (cur_program->m_state == BLOCKED)
				return cur_program;
			cur_program = cur_program->mp_next;
		}
	}
	
	return NULL;
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
	
	priority = get_process_priority(process_id);
	if (priority == -1)
		return NULL;
	
	q = &gp_pqueue[priority];
	if (q->first->m_pid == process_id)
		return dequeue(q);
	
	prev = q->first;
	cur = prev->mp_next;
	while (cur) {
		if (cur->m_pid == process_id) {
			prev->mp_next = cur->mp_next;
			if (q->last == cur)
				q->last = prev;
			
			return cur;
		}
	}
	
	return NULL;
}

/**
 * @brief: block the current process (waiting for memory)
 */
void k_add_blocked(void) {
	if (!gp_current_process)
		return;
#ifdef DEBUG_0 
	printf("Blocking process: %d\n", gp_current_process->m_pid);
#endif /* ! DEBUG_0 */
	gp_current_process->m_state = BLOCKED;
	g_num_blocked++;
	k_release_processor();
}

/**
 * @brief: set the priority of a process
 * If a process gets a priority higher than the running process,
 * then the new process is run
 */
int k_set_process_priority(int process_id, int priority) {
	PCB *process;
	
	if (process_id == 0)
		return -1;
	
	if (priority < 0 || priority > 3)
		return -1;
	
	if (process_id == gp_current_process->m_pid) {
		// Modifying the priority of the running process
		gp_current_process->m_priority = priority;
		if (priority >= gp_current_process->m_priority)
			k_release_processor();
		
		return 0;
	}
	
	process = remove_by_PID(process_id);
	if (!process)
		return -1;
	
	process->m_priority = priority;
	enqueue(&gp_pqueue[priority], process);
	// process is set to a higher priority than running process
	if (priority < gp_current_process->m_priority) {
		k_release_processor();
	}
	
	return 0;
}

/**
 * @brief: get the priority of a process
 */
int get_process_priority(int process_id) {
	int i;
	PCB* cur_program;
	
	if (process_id == gp_current_process->m_pid)
		return gp_current_process->m_priority;
	
	for (i = 0; i < NUM_PRIORITIES; i++) {
		cur_program = gp_pqueue[i].first;
		while (cur_program) {
			if (cur_program->m_pid == process_id)
				return i;
			cur_program = cur_program->mp_next;
		}
	}
	
	return -1;
}

void k_null_process(void);

/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init() 
{
	int i;
	int priority;
	U32 *sp;
  
        /* fill out the initialization table */
	set_test_procs();	
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[i + 1].m_pid = g_test_procs[i].m_pid;
		g_proc_table[i + 1].m_priority = g_test_procs[i].m_priority;
		g_proc_table[i + 1].m_stack_size = g_test_procs[i].m_stack_size;
		g_proc_table[i + 1].mpf_start_pc = g_test_procs[i].mpf_start_pc;
	}
	
	// Set initilization values for the null process
	g_proc_table[0].m_pid = 0;
	g_proc_table[0].m_stack_size = 0x100;
	g_proc_table[0].mpf_start_pc = &k_null_process;
	
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_TEST_PROCS + 1; i++) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
		(gp_pcbs[i])->m_state = NEW;
		
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		(gp_pcbs[i])->mp_sp = sp;
	}

	// Load ready queue
	for (i = 0; i < NUM_TEST_PROCS; i++) {
		priority = g_proc_table[i + 1].m_priority;
		if (priority < 0 || priority > 3)
			priority = 3;
		
		enqueue(&gp_pqueue[priority], gp_pcbs[i + 1]);
	}
	
	printf("debug\n");
}

void add_to_priority_queue(PCB *process) {
	if (!process)
		return;
	
	if (process->m_pid == 0)
		return;
	
	//process->m_state = RDY;
	enqueue(&gp_pqueue[process->m_priority], process);
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
PCB *scheduler(void)
{
	int i;
	PCB *process;
	
	// Default to Null process if this is the first time running
	if (gp_current_process == NULL) {
		gp_current_process = gp_pcbs[0]; 
		return gp_pcbs[0];
	}
	
	if (g_released_memory) {
		// Find first blocked process
		process = find_first_blocked();
		if (process) {
			process->m_state = RDY;
			g_num_blocked--;
			process = remove_by_PID(process->m_pid);
			//enqueue(&gp_pqueue[process->m_priority], process);
			g_released_memory = 0;
			return process;
		}
		
		g_released_memory = 0;
	}
	// Find the first process with the highest priority (that isn't blocked)
	for (i = 0; i < NUM_PRIORITIES; i++) {
		PCB *first_process = dequeue(&gp_pqueue[i]);
		if (!first_process)
			continue;
		
		if (first_process->m_state != BLOCKED)
			return first_process;
		else
			enqueue(&gp_pqueue[i], first_process);
		
		process = dequeue(&gp_pqueue[i]);
		while (process != first_process) {
			if (process->m_state != BLOCKED) {
				return process;
			}
				
			enqueue(&gp_pqueue[i], process);
			process = dequeue(&gp_pqueue[i]);
		}
	}
	
	// All processes busy, run the null process
	return gp_pcbs[0];
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old) 
{
	PROC_STATE_E state;
	
	state = gp_current_process->m_state;

	if (state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	} 
	
	/* The following will only execute if the if block above is FALSE */

	if (gp_current_process != p_pcb_old) {
		if (state == RDY ) {//|| state == BLOCKED){ 		
			if (p_pcb_old->m_state == RUN)
				p_pcb_old->m_state = RDY; 
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			gp_current_process->m_state = RUN;
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack    
		} else {
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		} 
	}
	return RTX_OK;
}
/**
 * @brief release_processor(). 
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
	PCB *p_pcb_old = NULL;
	
	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if ( gp_current_process == NULL  ) {
		gp_current_process = p_pcb_old; // revert back to the old process
		return RTX_ERR;
	}
	if ( p_pcb_old == NULL ) {
		p_pcb_old = gp_current_process;
	} else {
		add_to_priority_queue(p_pcb_old);
	}
	
	process_switch(p_pcb_old);
	return RTX_OK;
}

/**
 * @brief null_process()
 */
void k_null_process(void) {
	while(1) {
		uart1_put_string("NULL_PROCESS\n");
		release_processor();
	}
}
