/*
 * (C) Copyright 2011
 * <www.sycomm.cn>
 *
 */

#ifndef _HEAD_FPGADRVDBG_H
#define _HEAD_FPGADRVDBG_H

#include <linux/kernel.h>

#if defined(DEBUG_FPGADRV_DRIVER)
#define FPGADRVDBG(fmt, args...)	printk(KERN_INFO "FPGADRVDBG: " fmt "\n", ##args)
#else
#define FPGADRVDBG(fmt, args...)	do { } while (0)
#endif

#define FPGADRVINFO(fmt, args...)	printk(KERN_INFO "FPGADRVINFO: " fmt "\n", ##args)
#define FPGADRVERR(fmt, args...)	printk(KERN_ERR "FPGADRVERR: " fmt "\n", ##args)

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FPGADRVDBG_H */
