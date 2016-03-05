#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"

/**
 * @brief null_process()
 */
void k_null_process(void) {
	while (1) {
		uart1_put_string("NULL_PROCESS\n");
		release_processor();
	}
}
