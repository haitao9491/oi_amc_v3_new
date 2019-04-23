/*
 * (C) Copyright 2014
 * <www.raycores.com>
 *
 */

#ifndef _HEAD_LED_TIMER_H
#define _HEAD_LED_TIMER_H

#define LEDTIMER_DEFAULT_FREQ	1000

#if defined(__cplusplus)
extern "C" {
#endif


void led_timer_init(void);
void led_timer_exit(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_LED_TIMER_H */
