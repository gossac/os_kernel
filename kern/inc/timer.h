/**
 * @file timer.h
 * @brief The timer driver.
 * 
 * @author Tony Xi (xiaolix)
 * @author Zekun Ma (zekunm)
 */

#ifndef TIMER_H_SEEN
#define TIMER_H_SEEN

unsigned int tick_count;

void install_timer(void (*tickback)(unsigned int));

#endif
