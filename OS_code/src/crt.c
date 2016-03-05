#include "rtx.h"
#include "printf.h"

extern int uart_put_string(int, unsigned char *);

void crt_process(void) {
	MSG_BUF *message;
	while (1) {
		// Block until next message
		message = receive_message(NULL);
		//TODO: Check to make sure message has correct form

		// Output message
		uart_put_string(1, (unsigned char *)message->mtext);
		release_memory_block(message);
	}
}
