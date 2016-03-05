#include "kcd.h"

//TODO: delete
#include "printf.h"

#include "string.h"

KCD *g_kcd_list = NULL;

void add_kcd_command(char *str, int pid) {
	int i = 0;
	KCD *command = request_memory_block();
	
	command->mp_next = g_kcd_list;
	command->m_pid = pid;
	
	// Copy command into struct
	// This is unsafe- could write past the end
	// 								 of allocated memory block
	while (*str != '\0') {
		command->m_command[i] = *str;
		str++;
		i++;
	}
	
	g_kcd_list = command;
}

int get_kcd_handler(char *str) {
	KCD *curr = g_kcd_list;
	
	while (curr) {
		// compare strings
		if (is_prefix(curr->m_command, str))
			return curr->m_pid;
		
		curr = curr->mp_next;
	}
	
	return -1;
}

void kcd_process(void) {
	MSG_BUF *message;
	MSG_BUF *new_message;
	int pid;
	while (1) {
		// Block until next message
		message = receive_message(NULL);
		//TODO: Check to make sure message has correct form
		switch (message->mtype) {
			case KCD_REG:
				// Register a new command
				add_kcd_command(message->mtext, message->m_send_pid);
				break;
			case DEFAULT:
				// From UART i-process
				pid = get_kcd_handler(message->mtext);
				if (pid != -1) {
					// If we have a registered handler, send it a copy of the message
					new_message = request_memory_block();
					*new_message = *message;
					send_message(pid, (void*)new_message);
				}
				send_message(PID_CRT, message);
				break;
			default:
				release_memory_block(message);
		}
	}
}
