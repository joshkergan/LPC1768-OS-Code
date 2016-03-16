#include "set_priority.h"
#include "printf.h"

static int read_number(char *start) {
	int value = 0;
	while (is_digit(*start)) {
		int digit = *start - '0';
		value *= 10;
		value += digit;
		
		start++;
	}
	
	return value;
}

static BOOL valid_message(char *text, int *pid, int *priority) {
	char *cur;
	char *pid_start;
	char *priority_start;
	
	if (text[0] != '%' ||
			text[1] != 'C' ||
			text[2] != ' ')
		return FALSE;
	
	cur = text + 3;
	pid_start = cur;
	while (*cur != '\0' && is_digit(*cur))
		cur++;
	
	// make sure our number isn't of zero length
	// and is followed by a space
	if (*cur != ' ' || cur == pid_start)
		return FALSE;
	
	cur++;
	priority_start = cur;
	if (!is_digit(*priority_start))
		return FALSE;
	
	cur++;
	// line is terminated after the priority
	if (*cur != '\0' &&
			*cur != '\n')
		return FALSE;
	
	*pid = read_number(pid_start);
	*priority = read_number(priority_start);
	return TRUE;
}

void set_prio_process(void) {
	MSG_BUF *message;
	
	// Register command with the KCD process
	message = request_memory_block();
	message->mtype = KCD_REG;
	strcpy("%C", message->mtext);
	send_message(PID_KCD, message);
	
	while (1) {
		int sender;
		message = receive_message(&sender);
		if (sender == PID_KCD) {
			int pid, priority;
			BOOL success = FALSE;
			if (valid_message(message->mtext, &pid, &priority)) {
				success = (RTX_OK == set_process_priority(pid, priority));
			}
			
			if (!success) {
				// Output error
				MSG_BUF *error_message = request_memory_block();
				strcpy("Invalid params: ", error_message->mtext);
				strcpy(message->mtext, error_message->mtext + 16);
				send_message(PID_CRT, error_message);
			} else {
				dprintf("Set priority command: PID=%d, PRIORITY=%d\n\r", pid, priority);
			}
		}
		
		release_memory_block(message);
	}
}
