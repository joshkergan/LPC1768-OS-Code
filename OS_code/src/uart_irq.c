/**
 * @brief: uart_irq.c 
 * @author: NXP Semiconductors
 * @author: Y. Huang
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "uart_polling.h"
#include "k_rtx.h"
#include "string.h"
#ifdef DEBUG_0
#include "printf.h"
#endif

#define OUTPUT_BUFF_SIZE 40

uint8_t g_in_buffer[40];
uint32_t g_in_index = 0;

uint8_t g_out_buffer[OUTPUT_BUFF_SIZE];
uint32_t g_out_start = 0;
uint32_t g_out_end = 1;

uint8_t g_is_reading = 0;

uint8_t g_char_in;
uint8_t g_char_out;

extern int k_release_processor(void);
extern void * k_request_memory_block(void);
extern int k_release_memory_block(void *p_mem_blk);
extern int k_send_message(int pid, void *p_msg);
extern void* k_receive_message(int *sender_id);
extern int is_message(int receiver);
extern PCB* k_get_process(int pid);
extern BOOL is_memory_available(void);

extern PCB *gp_current_process;

/**
 * @brief: initialize the n_uart
 * NOTES: It only supports UART0. It can be easily extended to support UART1 IRQ.
 * The step number in the comments matches the item number in Section 14.1 on pg 298
 * of LPC17xx_UM
 */
int uart_irq_init(int n_uart) {

	LPC_UART_TypeDef *pUart;

	if ( n_uart ==0 ) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		 
		-----------------------------------------------------
		Step 1: Power control configuration. 
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(3);
	
		-----------------------------------------------------
		Step2: Select the clock source. 
		       Default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZin system_LPC17xx.c file.
		PCLK = CCLK/4, default setting after reset.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(7)|BIT(6));	
			
		-----------------------------------------------------
		Step 5: Pin Ctrl Block configuration for TXD and RXD
		        See Table 79 on pg108 in LPC17xx_UM.
		-----------------------------------------------------
		Note this is done before Steps3-4 for coding purpose.
		*/
		
		/* Pin P0.2 used as TXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 4);  
		
		/* Pin P0.3 used as RXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 6);  

		pUart = (LPC_UART_TypeDef *) LPC_UART0;	 
		
	} else if ( n_uart == 1) {
	    
		/* see Table 79 on pg108 in LPC17xx_UM */ 
		/* Pin P2.0 used as TXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 0);

		/* Pin P2.1 used as RXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 2);	      

		pUart = (LPC_UART_TypeDef *) LPC_UART1;
		
	} else {
		return 1; /* not supported yet */
	} 
	
	/*
	-----------------------------------------------------
	Step 3: Transmission Configuration.
	        See section 14.4.12.1 pg313-315 in LPC17xx_UM 
	        for baud rate calculation.
	-----------------------------------------------------
        */
	
	/* Step 3a: DLAB=1, 8N1 */
	pUart->LCR = UART_8N1; /* see uart.h file */ 

	/* Step 3b: 115200 baud rate @ 25.0 MHZ PCLK */
	pUart->DLM = 0; /* see table 274, pg302 in LPC17xx_UM */
	pUart->DLL = 9;	/* see table 273, pg302 in LPC17xx_UM */
	
	/* FR = 1.507 ~ 1/2, DivAddVal = 1, MulVal = 2
	   FR = 1.507 = 25MHZ/(16*9*115200)
	   see table 285 on pg312 in LPC_17xxUM
	*/
	pUart->FDR = 0x21;       
	
 

	/*
	----------------------------------------------------- 
	Step 4: FIFO setup.
	       see table 278 on pg305 in LPC17xx_UM
	-----------------------------------------------------
        enable Rx and Tx FIFOs, clear Rx and Tx FIFOs
	Trigger level 0 (1 char per interrupt)
	*/
	
	pUart->FCR = 0x07;

	/* Step 5 was done between step 2 and step 4 a few lines above */

	/*
	----------------------------------------------------- 
	Step 6 Interrupt setting and enabling
	-----------------------------------------------------
	*/
	/* Step 6a: 
	   Enable interrupt bit(s) wihtin the specific peripheral register.
           Interrupt Sources Setting: RBR, THRE or RX Line Stats
	   See Table 50 on pg73 in LPC17xx_UM for all possible UART0 interrupt sources
	   See Table 275 on pg 302 in LPC17xx_UM for IER setting 
	*/
	/* disable the Divisior Latch Access Bit DLAB=0 */
	pUart->LCR &= ~(BIT(7)); 
	
	//pUart->IER = IER_RBR | IER_THRE | IER_RLS; 
	pUart->IER = IER_RBR | IER_RLS;

	/* Step 6b: enable the UART interrupt from the system level */
	
	if ( n_uart == 0 ) {
		NVIC_EnableIRQ(UART0_IRQn); /* CMSIS function */
	} else if ( n_uart == 1 ) {
		NVIC_EnableIRQ(UART1_IRQn); /* CMSIS function */
	} else {
		return 1; /* not supported yet */
	}
	pUart->THR = '\0';
	return 0;
}

// Force the uart to output buffer
void output_uart(void) {
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	
	// Return if it is already enabled
	if (pUart->IER & IER_THRE)
		return;
	
	pUart->IER |= IER_THRE; // enable the IER_THRE bit
	// Force a THRE interrupt by writing a character to UART
	pUart->THR = '\0';
}

/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	PRESERVE8
	IMPORT uart_iprocess
	PUSH{r4-r11, lr}
	BL uart_iprocess
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void uart_iprocess(void)
{
	uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	__disable_irq();
#ifdef DEBUG_0
	//uart1_put_string("Entering c_UART0_IRQHandler\n\r");
#endif // DEBUG_0

	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
		/* read UART. Read RBR will clear the interrupt */

		// Perform a 'context switch'
		PCB *old_proc = gp_current_process;
		gp_current_process = k_get_process(PID_UART_IPROC);
		g_char_in = pUart->RBR;

#ifdef DEBUG_0
		uart1_put_string("Reading a char = ");
		uart1_put_char(g_char_in);
		uart1_put_string("\n\r");
#endif // DEBUG_0

		// process the character. If it is a hotkey, call the corresponding function
		// If we are reading, fall through to default and add to the command buffer
		switch (g_char_in) {
			case '\r':
			case '\n':
				if (g_is_reading) {
					// We've finished reading a command, send it to the KCD process
					MSG_BUF *message;
					g_is_reading = 0;
					strcpy("\n\r", (char *)(g_in_buffer + g_in_index));
					if (is_memory_available()) {
						message = k_request_memory_block();
						message->mtype = DEFAULT;
						strcpy((char *)g_in_buffer, message->mtext);
						k_send_message(PID_KCD, message);
					}
					g_in_index = 0;
				}
				break;
			case '%':
				if (!g_is_reading) {
					// Start reading a command
					g_is_reading = 1;
					g_in_index = 1;
					g_in_buffer[0] = '%';
					break;
				}
#if defined(DEBUG_0) && defined(_DEBUG_HOTKEYS)
			case READY_Q_COMMAND:
				if (!g_is_reading) {
					print_ready();
					break;
				}
			case MEMORY_Q_COMMAND:
				if (!g_is_reading) {
					print_mem_blocked();
					break;
				}
			case RECEIVE_Q_COMMAND:
				if (!g_is_reading) {
					print_receive_blocked();
					break;
				}
			case MESSAGE_COMMAND:
				if (!g_is_reading) {
					print_messages();
					break;
				}
#endif /* DEBUG HOTKEYS */
			default:
				if (g_is_reading) {
					// TODO: check bounds
					g_in_buffer[g_in_index++] = g_char_in;
				}
		}

		gp_current_process = old_proc;
	} else if (IIR_IntId & IIR_THRE) {
	/* THRE Interrupt, transmit holding register becomes empty */
		// Check for messages and load the buffer

		// Perform a 'context switch' to the i-process
		MSG_BUF *message;
		PCB *old_proc = gp_current_process;
		gp_current_process = k_get_process(PID_UART_IPROC);

		// Don't block waiting for a message
		while (is_message(PID_UART_IPROC)) {
			int sender = PID_CRT;
			char *c;

			// Receive message and copy it to the buffer
			message = k_receive_message(&sender);
			c = message->mtext;

			//dprintf("Copying message to buffer: %s\n\r", message->mtext);

			if (*c != '\0') {
				do {
					g_out_buffer[g_out_end] = *c;
					g_out_end = (g_out_end + 1) % OUTPUT_BUFF_SIZE;
					c++;
				} while (g_out_end != g_out_start && *c != '\0');
			}
			k_release_memory_block(message);
		}

		// Check if there is something in the circular buffer
		if (g_out_start != g_out_end) {
			g_char_out = g_out_buffer[g_out_start];
			pUart->THR = g_char_out;
			g_out_start = (g_out_start + 1) % OUTPUT_BUFF_SIZE;
		} else {
			// nothing to print, disable the THRE interrupt
			pUart->IER ^= IER_THRE; // toggle (disable) the IER_THRE bit
		}

		gp_current_process = old_proc;
	      
	} else {  /* not implemented yet */
#ifdef DEBUG_0
			//uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
		__enable_irq();
		return;
	}	
	__enable_irq();
}
