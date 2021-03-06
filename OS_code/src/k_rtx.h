/** 
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#ifndef K_RTX_H_
#define K_RTX_H_
/*----- Definitations -----*/
#include "common.h"

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0

#define NUM_TEST_PROCS 		6
#define NUM_STRESS_PROCS  3
#define NUM_SYSTEM_PROCS 	7 // NULL, KCD, CRT, CLOCK,
														// TIMER-IPROC, UART-IPROC, SET PRIORITY
#define NUM_PROCS					(NUM_STRESS_PROCS + NUM_TEST_PROCS + NUM_SYSTEM_PROCS)
#define NUM_PRIORITIES 		4


/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states, note we only assume three states in this example */
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_MEMORY, BLOCKED_ON_RECEIVE, EXIT} PROC_STATE_E;  

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project 
*/
typedef struct pcb 
{ 
	struct pcb *mp_next;  /* next pcb, not used in this example */  
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	int m_priority;	/* process priority */
	BOOL b_i_process;
	PROC_STATE_E m_state;   /* state of the process */      
	void *mp_assigned_mem;   /* memory returned from being blocked */
} PCB;

typedef struct pcb_queue
{
	PCB *first;
	PCB *last;
} QUEUE;

#endif // ! K_RTX_H_
