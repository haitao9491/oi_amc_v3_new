/*
 * (C) Copyright 2014
 * <www.raycores.com>
 *
 */

#ifndef _HEAD_CLOVESDBG_H
#define _HEAD_CLOVESDBG_H

#include <linux/kernel.h>

#if defined(DEBUG_CLOVES_DRIVER)
#define CLOVESDBG(fmt, args...)		printk(KERN_INFO "CLOVESDBG: " fmt "\n", ##args)
#else
#define CLOVESDBG(fmt, args...)		do { } while (0)
#endif

#define CLOVESINFO(fmt, args...)	printk(KERN_INFO "CLOVES: " fmt "\n", ##args)
#define CLOVESERR(fmt, args...)		printk(KERN_ERR "CLOVESERR: " fmt "\n", ##args)

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_CLOVESDBG_H */
