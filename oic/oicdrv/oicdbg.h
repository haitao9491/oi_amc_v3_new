/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oicdbg.h - A description goes here.
 *
 */

#ifndef _HEAD_OICDBG_00EB17B7_74A41D31_4EFB7986_H
#define _HEAD_OICDBG_00EB17B7_74A41D31_4EFB7986_H

#include <linux/kernel.h>

#define OICDBGF(fmt, args...)	do { } while (0)

#if defined(DEBUG_OIC_DRIVER)
#define OICDBG(fmt, args...)	printk(KERN_ERR "OIC: " fmt "\n", ##args)
#else
#define OICDBG(fmt, args...)	do { } while (0)
#endif
#define OICINFO(fmt, args...)	printk(KERN_INFO "OICINFO: " fmt "\n", ##args)
#define OICERR(fmt, args...)	printk(KERN_ALERT "OICERR: " fmt "\n", ##args)

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_OICDBG_00EB17B7_74A41D31_4EFB7986_H */
