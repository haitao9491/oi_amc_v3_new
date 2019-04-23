/*
 * (C) Copyright 2015
 *
 * dts_query.h - A description goes here.
 *
 */

#ifndef __DTS_FPGA__
#define __DTS_FPGA__
int dts_dev_nums(char *devname);
int dts_dev_chind_nums(unsigned int devid, char *devname);
int dts_dev_phys_addr(char *devname, unsigned int devid,unsigned int childid, unsigned long *addr);
int dts_dev_size(char *devname, unsigned int devid, unsigned int childid, unsigned long *size);

#define INFO(fmt, args...) printk(KERN_INFO "INFO(%s, %d):" fmt, __FILE__,__LINE__, ##args)
#define ERROR(fmt, args...) printk(KERN_ERR "ERR(%s, %d):" fmt, __FILE__,__LINE__, ##args)

#ifdef _SELF_DEBUG_
#define DEBUG(fmt, args...) printk("DEBUG(%d):" fmt, __LINE__, ##args)
#else
#define DEBUG(...)
#endif

#endif
