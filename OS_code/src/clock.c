#include "clock.h"
#include "string.h"

#include "printf.h"

static void print_2digit(int val, char *buff) {
	int tens = val / 10 % 10;
	int ones = val % 10;
	
	buff[0] = tens + '0';
	buff[1] = ones + '0';
}

static int read_2digit(char *buff) {
	int tens = buff[0] - '0';
	int ones = buff[1] - '0';
	
	return 10 * tens + ones;
}

static void print_time(uint32_t time) {
	MSG_BUF *message;
	
	int hours = time / 60 / 60 % 24;
	int minutes = time / 60 % 60;
	int seconds = time % 60;
	
	message = request_memory_block();
	message->mtype = DEFAULT;
	strcpy("00:00:00\r\n", message->mtext);

	print_2digit(hours, message->mtext);
	print_2digit(minutes, message->mtext + 3);
	print_2digit(seconds, message->mtext + 6);

	send_message(PID_CRT, message);
}

static int is_valid_time(char *buff) {
	dprintf("Validating time: %s\n\r", buff);
	if (!(is_digit(buff[0]) &&
				is_digit(buff[1]) &&
				':' == buff[2] &&
				is_digit(buff[3]) &&
				is_digit(buff[4]) &&
				':' == buff[5] &&
				is_digit(buff[6]) &&
				is_digit(buff[7])))
		return 0;
	
	if (read_2digit(buff) >= 24)
		return 0;
	
	if (read_2digit(buff + 3) >= 60)
		return 0;
	
	if (read_2digit(buff + 6) >= 60)
		return 0;
	
#ifdef DEBUG_0
	printf("Valid time!\n\r");
#endif
	return 1;
}

void clock_process(void) {
	MSG_BUF *message;
	int is_running = 1;
	uint32_t time = 0;
	// variables used to set the time
	int hours, minutes, seconds;
	
	// Register command with the KCD process
	message = request_memory_block();
	message->mtype = KCD_REG;
	strcpy("%W", message->mtext);
	send_message(PID_KCD, message);
	
	message = request_memory_block();
	delayed_send(PID_CLOCK, message, ONE_SECOND);
	
	while (1) {
		message = receive_message(NULL);
		if (message->m_send_pid == PID_CLOCK) {
			// only update the clock if it is running
			if (is_running) {
				time++;
				print_time(time);
				delayed_send(PID_CLOCK, message, ONE_SECOND);
			}
			else {
				release_memory_block(message);
				message = NULL;
			}
		} else if (message->mtext[0] == '%' && message->mtext[1] == 'W') {
			switch(message->mtext[2]) {
				case 'R':
					// Reset
					time = 0;
					release_memory_block(message);

					if (!is_running) {
						// Restart the clock
						is_running = 1;
						message = request_memory_block();
						delayed_send(PID_CLOCK, message, ONE_SECOND);
					}
					break;
				case 'S':
					// Set
					if (!is_valid_time(message->mtext + 4)) {
						release_memory_block(message);
						break;
					}
					
					hours = read_2digit(message->mtext + 4);
					minutes = read_2digit(message->mtext + 7);
					seconds = read_2digit(message->mtext + 10);
					
					release_memory_block(message);

					printf("Updating time: hours=%d, minutes=%d, seconds=%d\n\r",
						hours, minutes, seconds);
					
					time = hours * 60 * 60 + minutes * 60 + seconds;
					if (!is_running) {
						// Restart the clock
						is_running = 1;
						message = request_memory_block();
						delayed_send(PID_CLOCK, message, ONE_SECOND);
					}
					break;
				case 'T':
					// Terminate
					is_running = 0;
					release_memory_block(message);
					break;
				default:
					release_memory_block(message);
					break;
			}
			print_time(time);
		}
	}
	
	//if (message)
	//	release_memory_block(message);
	
	//terminate_sys_proc();
}
