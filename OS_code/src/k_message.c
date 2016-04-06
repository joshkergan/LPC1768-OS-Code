#include "common.h"
#include "k_rtx.h"
#include <stdint.h>

#ifdef DEBUG_0
#include "printf.h"
#endif

#define MSG_BUF_COUNT 10

extern volatile uint32_t g_timer_count;
extern PCB *gp_current_process;
extern PCB ** gp_pcbs;
extern int k_release_processor(void);
extern PCB* k_get_process(int m_pid);

typedef enum {SEND = 0, RECV, DELY_SEND} FUNC_CALL;  
typedef struct message_info {
	int m_sender_pid;
	int m_recver_pid;
	FUNC_CALL e_call_type;
	int m_timestamp;
	char m_text[16];
} MSG_INFO;

MSG_BUF* p_msg_boxes_start[NUM_PROCS] = {NULL};
MSG_BUF* p_msg_boxes_end[NUM_PROCS] = {NULL};

MSG_BUF* p_dely_msg_box = NULL;
MSG_INFO msg_buf[MSG_BUF_COUNT];
int msg_buf_index = 0;


BOOL unblock_receiver(int m_pid) {
	int i;
	BOOL proc_unblocked = FALSE;
	PCB* cur_program;
	for (i = 0; i < NUM_PROCS && m_pid != gp_pcbs[i]->m_pid; i++) {}
	cur_program = gp_pcbs[i];
	
	if (cur_program->m_state == BLOCKED_ON_RECEIVE) {
		cur_program->m_state = RDY;
		proc_unblocked = TRUE;
	}
	return proc_unblocked;
}

void enqueue_message(int m_recv_id, MSG_BUF* p_msg){
	if (p_msg_boxes_start[m_recv_id] == NULL) {
		p_msg_boxes_start[m_recv_id] = p_msg;
		p_msg_boxes_end[m_recv_id] = p_msg;
	} else {
		p_msg_boxes_end[m_recv_id]->mp_next = p_msg;
		p_msg_boxes_end[m_recv_id] = p_msg;
	}
	p_msg->mp_next = NULL;
}

int is_message(int receiver) {
	return p_msg_boxes_start[receiver] != NULL;
}

MSG_BUF* find_message_from(){
	int m_recv_id = gp_current_process->m_pid;
	MSG_BUF* message = p_msg_boxes_start[m_recv_id];
	MSG_BUF* last = NULL;
	
	if (message != NULL) {
		if (last == NULL) {
			p_msg_boxes_start[m_recv_id] = message->mp_next;
		} else {
			last->mp_next = message->mp_next;
		}
		if(message->mp_next == NULL) {
			p_msg_boxes_end[m_recv_id] = last;
		}
	}
	return message;
}

void increment_message_buffer(FUNC_CALL type, MSG_BUF* message) {
	int j;
	MSG_INFO info = msg_buf[msg_buf_index];
	info.m_sender_pid = message->m_send_pid;
	info.m_recver_pid = message->m_recv_pid;
	info.e_call_type = type;
	info.m_timestamp = g_timer_count;
	for (j = 0; j < 16; j++) {
		info.m_text[j] = message->mtext[j];
	}
	msg_buf[msg_buf_index] = info;
	msg_buf_index = (msg_buf_index + 1) % MSG_BUF_COUNT;
}

int k_send_message(int process_id, void *message_envelope){
	MSG_BUF* message;
	PCB* sending_process = gp_current_process;
	PCB* receiving_process;
	BOOL may_preempt;
	__disable_irq();
	message = (MSG_BUF*) message_envelope;
	
	if (gp_current_process == NULL) {
		__enable_irq();
		return RTX_ERR;
	}
	message->m_send_pid = sending_process->m_pid;
	message->m_recv_pid = process_id;
	
	enqueue_message(process_id, message);
	may_preempt = unblock_receiver(process_id);
	
	increment_message_buffer(SEND, message);

	// Preempt if necessary
	receiving_process = k_get_process(process_id);
	if(may_preempt && sending_process->m_priority > receiving_process->m_priority) {
		k_release_processor();
	}
	
	__enable_irq();
	return RTX_OK;
}

void* k_receive_message(int *sender_id){
	MSG_BUF* message;
	__disable_irq();
	while ((message = find_message_from()) == NULL) {
		gp_current_process->m_state = BLOCKED_ON_RECEIVE;
		k_release_processor();
	}
	if(sender_id != NULL) {
		*sender_id = message->m_send_pid;
	}
	increment_message_buffer(RECV, message);
	__enable_irq();

	return message;
}

void k_delayed_enqueue(void *p_msg) {
	MSG_BUF* message = (MSG_BUF*)p_msg;
	int process_id = message->m_recv_pid;	
	__disable_irq();
	
	enqueue_message(process_id, message);
	unblock_receiver(process_id);
	
	__enable_irq();
}

int k_delayed_send(int pid, void *p_msg, int delay) {
	MSG_BUF* message = (MSG_BUF*)p_msg;
	PCB* sending_process = gp_current_process;

	__disable_irq();
	message->m_send_pid = sending_process->m_pid;
	message->M_TIMESTAMP = g_timer_count + delay;
	message->M_FORWARD_PID = pid;
	
	// Register the message with the Timer i-process
	k_send_message(PID_TIMER_IPROC, message);
	
	increment_message_buffer(DELY_SEND, message);
	__enable_irq();
	return RTX_OK;
}

#if defined(DEBUG_0) &&  defined(_DEBUG_HOTKEYS)
void print_messages(void) {
	int i, j;
	for (i = 0; i < MSG_BUF_COUNT; i++) {
		MSG_INFO info = msg_buf[(i + msg_buf_index) % MSG_BUF_COUNT];
		switch(info.e_call_type) {
			case SEND:
				printf("Sent message:\n\r");
				break;
			case RECV:
				printf("Received message:\n\r");
				break;
			case DELY_SEND:
				printf("Delayed message:\n\r");
				break;
		}
		printf("\tSender: %d\n\r", info.m_sender_pid);
		printf("\tDestination: %d\n\r", info.m_recver_pid);
		printf("\tText: ");
		for (j = 0; j < 16; j++) {
			printf("%c", info.m_text[j]);
		}
		printf("\n\r");
		printf("\tTimestamp: %d\n\r", info.m_timestamp);
	}
}
#endif
