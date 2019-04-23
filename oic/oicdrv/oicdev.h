/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oicdev.h - A description goes here.
 *
 */

#ifndef _HEAD_OICDEV_2E911A53_2A92410A_3EA4116A_H
#define _HEAD_OICDEV_2E911A53_2A92410A_3EA4116A_H

#include <linux/cdev.h>
#include "oic.h"

struct oic_device {
	int				major;
	struct cdev		cdev;
	struct class   *class;

	unsigned int	fpga_phys;
	void		   *fpga_virt;
	unsigned int	fpga_size;
	unsigned int	cpld_phys;
	void		   *cpld_virt;
	unsigned int	cpld_size;

	void			*link_map;

	struct mutex	lock;

	/* '1': LP '2': HP */
	int				sdhtype;
};

extern struct oic_device *oicdev;
extern int gfpmode;

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_OICDEV_2E911A53_2A92410A_3EA4116A_H */
