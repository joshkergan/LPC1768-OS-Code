/**
 * @brief timer.h - Timer header file
 * @author Y. Huang
 * @date 2013/02/12
 */
#ifndef _TIMER_H_
#define _TIMER_H_
#include "common.h"
#include "k_rtx.h"

uint32_t timer_init(uint8_t n_timer);  /* initialize timer n_timer */
extern MSG_BUF* p_dely_msg_box;
extern PCB* gp_current_process;

extern void k_delayed_enqueue(void *p_msg);
extern int is_message(int receiver);
extern PCB* k_get_process(int pid);

#endif /* ! _TIMER_H_ */
