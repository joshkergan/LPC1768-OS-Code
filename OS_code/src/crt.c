#include "rtx.h"
#include "printf.h"

extern int uart_put_string(int, unsigned char *);
extern void output_uart(void);

void crt_process(void) {
	MSG_BUF *message;
	while (1) {
		// Block until next message
		message = receive_message(NULL);
		//TODO: Check to make sure message has correct form

#ifdef DEBUG_0
		//printf("forwarding message to UART: %s\n\r", message->mtext);
#endif
		send_message(PID_UART_IPROC, message);

		// Enable output interrupts
		output_uart();
	}
}
