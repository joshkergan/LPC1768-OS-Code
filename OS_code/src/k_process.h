/**
 * @file:   k_process.h
 * @brief:  process management hearder file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef K_PROCESS_H_
#define K_PROCESS_H_

#include "k_rtx.h"

/* ----- Definitions ----- */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

struct free_heap_block {
	struct free_heap_block* next;
};

/* ----- Functions ----- */

void process_init(void);               /* initialize all procs in the system */
PCB *scheduler(void);                  /* pick the pid of the next to run process */
int k_release_processor(void);           /* kernel release_process function */

extern PCB *remove_by_PID(int);				 /* Remove a process from the queue by it's id */
extern void add_to_priority_queue(PCB *); /* Add a process to the priority queue */
extern PCB *find_first_blocked(void);  /* Retrieve the highest priority blocked process */
extern PCB *find_first_ready(void);    /* Retrieve the highest priority ready process */

extern struct free_heap_block* gp_free_space;
extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */
extern void k_null_process(void);			 /* the null process */
extern void kcd_process(void);
extern void crt_process(void);

#endif /* ! K_PROCESS_H_ */
