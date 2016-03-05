/**
 * @brief timer.h - Timer header file
 * @author Y. Huang
 * @date 2013/02/12
 */
#ifndef _TIMER_H_
#define _TIMER_H_
#include "common.h"


uint32_t timer_init(uint8_t n_timer);  /* initialize timer n_timer */
extern MSG_BUF* p_dely_msg_box;
extern void k_delayed_enqueue(void *p_msg);

#endif /* ! _TIMER_H_ */
