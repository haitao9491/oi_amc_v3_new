/*
 * (C) Copyright 2014
 * <www.raycores.com>
 *
 */

#ifndef _HEAD_CLOVES_MISC_CTL_H
#define _HEAD_CLOVES_MISC_CTL_H

#if defined(__cplusplus)
extern "C" {
#endif

int cloves_memmap(void);
int cloves_unmemmap(void);
int cloves_gpio_init(void);
int cloves_ctl_watchdog(unsigned long arg);
int cloves_ctl_green(unsigned long arg);
int cloves_get_dialcode(unsigned long arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_CLOVES_MISC_CTL_H */
