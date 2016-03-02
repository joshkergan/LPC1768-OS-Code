#ifndef KCD_H_
#define KCD_H_

#include "stdint.h"
#include "rtx.h"

/* ----- RTX User API ----- */
#define __SVC_0  __svc_indirect(0)

/* Memory Management */
extern void *k_request_memory_block(void);
#define request_memory_block() _request_memory_block((U32)k_request_memory_block)
extern void *_request_memory_block(U32 p_func) __SVC_0;

typedef struct kcd_command {
	struct kcd_command *mp_next;
	uint32_t m_pid;
	char m_command[1];
} KCD;


#endif /* !KCD_H_ */
