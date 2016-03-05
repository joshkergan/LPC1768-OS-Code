#ifndef KCD_H_
#define KCD_H_

#include "stdint.h"
#include "rtx.h"

extern int k_send_message(int pid, void *p_msg);
extern void *k_receive_message(int *p_pid);

typedef struct kcd_command {
	struct kcd_command *mp_next;
	uint32_t m_pid;
	char m_command[1];
} KCD;


#endif /* !KCD_H_ */
