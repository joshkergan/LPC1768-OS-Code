#include "common.h"
#include "k_rtx.h"

extern volatile uint32_t g_timer_count;
extern PCB *gp_current_process;
extern int k_release_processor(void);

MSG_BUF* p_msg_boxes_start[NUM_PROCS] = {NULL};
MSG_BUF* p_msg_boxes_end[NUM_PROCS] = {NULL};

MSG_BUF* p_dely_msg_box = NULL;

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

MSG_BUF* find_message_from(int* sender_id){
	int m_recv_id = gp_current_process->m_pid;
	MSG_BUF* message = p_msg_boxes_start[m_recv_id];
	MSG_BUF* last = NULL;
	
	if(sender_id != NULL) {
		while (message != NULL && message->m_send_pid != (int)sender_id) {
			last = message;
			message = message->mp_next;
		}
	}
	
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

int k_send_message(int process_id, void *message_envelope){
	MSG_BUF* message;
	PCB* receiving_process = gp_current_process;
	__disable_irq();
	message = (MSG_BUF*) message_envelope;
	
	if (gp_current_process == NULL) {
		__enable_irq();
		return RTX_ERR;
	}
	message->m_send_pid = receiving_process->m_pid;
	message->m_recv_pid = process_id;
	
	enqueue_message(process_id, message);
	if (receiving_process->m_state == BLOCKED_ON_RECEIVE) {
		receiving_process->m_state = RDY;
	}	
	__enable_irq();
	return RTX_OK;
}

void* k_receive_message(int *sender_id){
	MSG_BUF* message;
	__disable_irq();
	while ((message = find_message_from(sender_id)) == NULL) {
		gp_current_process->m_state = BLOCKED_ON_RECEIVE;
		k_release_processor();
	}
	__enable_irq();

	return message;
}

void k_delayed_enqueue(void *p_msg) {
	MSG_BUF* message = (MSG_BUF*) p_msg;
	__disable_irq();	
	
	enqueue_message(process_id, message);
	if (receiving_process->m_state == BLOCKED_ON_RECEIVE) {
		receiving_process->m_state = RDY;
	}	
	__enable_irq();
}

int k_delayed_send(int pid, void *p_msg, int delay) {
	MSG_BUF* message = (MSG_BUF*)p_msg;
	MSG_BUF* queue = p_dely_msg_box;
	MSG_BUF* last = NULL;
	PCB* receiving_process = gp_current_process;
	
	message->m_send_pid = receiving_process->m_pid;
	message->m_recv_pid = process_id;
	message->data[0] = g_timer_count + delay;
	__disable_irq();
	
	while (queue != NULL && queue->data[0] <= message->data[0]) {
		last = queue;
		queue = queue->next;
	}
	
	message->p_next = queue;
	if (last == NULL) {
		p_dely_msg_box = message;
	} else {
		last->next = message;
	}
	
	__enable_irq();
}
