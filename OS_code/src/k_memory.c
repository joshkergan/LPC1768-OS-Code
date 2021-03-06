/**
 * @file:   k_memory.c
 * @brief:  kernel memory managment routines
 * @author: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* ! DEBUG_0 */

#define HEAP_BLOCK_SIZE 128 / sizeof(U32) // 128 bytes

/* ----- Global Variables ----- */
U32 *gp_stack; /* The last allocated stack low address. 8 bytes aligned */
               /* The first stack starts at the RAM high address */
							 /* stack grows down. Fully decremental stack */

// Struct that points to free memory blocks
struct free_heap_block {
	struct free_heap_block* next;
};

struct free_heap_block* gp_free_space;

/**
 * @brief: Initialize RAM as follows:

0x10008000+---------------------------+ High Address
          |    Proc 1 STACK           |
          |---------------------------|
          |    Proc 2 STACK           |
          |---------------------------|<--- gp_stack
          |                           |
          |        HEAP               |
          |                           |
          |---------------------------|<--- p_end (during initialization)
          |        PCB 2              |
          |---------------------------|
          |        PCB 1              |
          |---------------------------|
          |        PCB pointers       |
          |---------------------------|<--- gp_pcbs
          |        Padding            |
          |---------------------------|  
          |Image$$RW_IRAM1$$ZI$$Limit |
          |...........................|          
          |       RTX  Image          |
          |                           |
0x10000000+---------------------------+ Low Address

*/

void memory_init(void)
{
	U8 *p_end = (U8 *)&Image$$RW_IRAM1$$ZI$$Limit;
	int i;
	U32* p_heap;
  
	/* 4 bytes padding */
	p_end += 4;

	/* allocate memory for pcb pointers   */
	gp_pcbs = (PCB **)p_end;
	p_end += NUM_PROCS * sizeof(PCB *);
  
	/* allocate memory for priority array */
	gp_pqueue = (QUEUE *)p_end;
	p_end += NUM_PRIORITIES * sizeof(QUEUE);
	for(i = 0; i < NUM_PRIORITIES; i++) {
		gp_pqueue[i].first = NULL;
		gp_pqueue[i].last = NULL;
	}
	
	/* allocate memory for PCBs */
	for ( i = 0; i < NUM_PROCS; i++ ) {
		gp_pcbs[i] = (PCB *)p_end;
		p_end += sizeof(PCB); 
	}
#ifdef DEBUG_0  
	printf("gp_pcbs[0] = 0x%x \n", gp_pcbs[0]);
	printf("gp_pcbs[1] = 0x%x \n", gp_pcbs[1]);
	printf("gp_pcbs[2] = 0x%x \n", gp_pcbs[2]);
#endif
	
	/* prepare for alloc_stack() to allocate memory for stacks */
	gp_stack = (U32 *)RAM_END_ADDR;
	if ((U32)gp_stack & 0x04) { /* 8 bytes alignment */
		--gp_stack; 
	}
  
	/* allocate memory for heap*/
	gp_free_space = NULL;
	for(p_heap = (U32 *)p_end; p_heap < gp_stack && p_heap < (U32*)p_end + HEAP_SIZE*HEAP_BLOCK_SIZE; p_heap += HEAP_BLOCK_SIZE) {
		struct free_heap_block* next_block = (struct free_heap_block *)p_heap;
		next_block->next = gp_free_space;
		gp_free_space = next_block;
	}
  
}

/**
 * @brief: allocate stack for a process, align to 8 bytes boundary
 * @param: size, stack size in bytes
 * @return: The top of the stack (i.e. high address)
 * POST:  gp_stack is updated.
 */

U32 *alloc_stack(U32 size_b) 
{
	U32 *sp;
	sp = gp_stack; /* gp_stack is always 8 bytes aligned */
	
	/* update gp_stack */
	gp_stack = (U32 *)((U8 *)sp - size_b);
	
	/* 8 bytes alignement adjustment to exception stack frame */
	if ((U32)gp_stack & 0x04) {
		--gp_stack;
	}
	return sp;
}

BOOL is_memory_available(void) {
	return gp_free_space != NULL;
}

/**
 * @brief: allocate a heap block to a process
 * @return: A pointer to the memory block in the heap
 * POST:  gp_free_space is updated.
 */
void *k_request_memory_block(void) {
	void* mem_alloced;
	
	__disable_irq();
#ifdef DEBUG_0 
	//printf("k_request_memory_block: entering...\n");
#endif /* ! DEBUG_0 */
	
	// No free heap memory -> block thread
	if (gp_free_space == NULL) {
		dprintf("Blocking process: %d (0x%x) on memory\n", gp_current_process->m_pid, gp_current_process->m_pid);
		// suspends the process
		gp_current_process->m_state = BLOCKED_ON_MEMORY;
		g_num_mem_blocked++;
		k_release_processor();
		mem_alloced = gp_current_process->mp_assigned_mem;
	} else {
		mem_alloced = (void *)gp_free_space;
		// If there is a free heap block, approve the request (pop it off the stack of free_heap_block_s)
		gp_free_space = gp_free_space->next;
	}
	
#ifdef DEBUG_0
	//printf("Assigning memory block: 0x%x\n\r", mem_alloced);
#endif
	
	__enable_irq();
	return mem_alloced;
}

/**
 * @brief: return a block of memory to the heap
 * @param: a valid pointer to memory in the heap
 * POST:  gp_free_space is updated. The memory block may
 * 				have been assigned to a blocked process.
 * 
 *     		A blocked process can preemept the current process 
 *      	if it has a higher prioity
 */
int k_release_memory_block(void *p_mem_blk) {
	struct free_heap_block* next_block;
	if(p_mem_blk < (void*)0x10000000) {
		dprintf("Releasing pointer invalid 0x%X\n\r", p_mem_blk);
	}
	next_block = (struct free_heap_block *)p_mem_blk;
	
	__disable_irq();
#ifdef DEBUG_0 
	//printf("k_release_memory_block: releasing block @ 0x%x\n", p_mem_blk);
#endif /* ! DEBUG_0 */
	
	// !!! Assuming p_mem_blk is allocated, and can be freed by current process

	// Push p_mem_blk onto stack of free_heap_block_s
	next_block->next = gp_free_space;
	gp_free_space = next_block;
	
	if (g_num_mem_blocked > 0) {
		g_released_memory = 1;
		k_release_processor();
	}
	
	__enable_irq();
	return RTX_OK;
}
