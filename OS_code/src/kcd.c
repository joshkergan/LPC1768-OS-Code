#include "k_rtx.h"
#include "kcd.h"

KCD *g_kcd_list = NULL;

int strcmp(char *s1, char *s2) {
	while (*s1 == *s2 && *s1 != '\0') {
		s1++;
		s2++;
	}
	
	if (*s1 == '\0' && *s2 == '\0')
		return 0;
	
	return (*s1 < *s2) ? -1 : 1;
}

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
		if (0 == strcmp(str, curr->m_command))
			return curr->m_pid;
		
		curr = curr->mp_next;
	}
	
	return -1;
}

void kcd_proc(void) {
	MSG_BUF *message;
	while (1) {
		// Block until next message
		//message = receive_message(null);
		//TODO: Check to make sure message has correct form
		add_kcd_command(message->mtext, message->m_send_pid);
	}
}
