/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * fpgactl.h - A description goes here.
 *
 */

#ifndef _HEAD_FPGACTL_56F70F59_109615F7_37D96443_H
#define _HEAD_FPGACTL_56F70F59_109615F7_37D96443_H

#if defined(__cplusplus)
extern "C" {
#endif

int  fpgactl_init(void);
int  fpgactl_cfg_start(unsigned long arg);
int  fpgactl_cfg_load(unsigned long arg);
int  fpgactl_cfg_done(unsigned long arg);
int  fpgactl_cfg_stop(unsigned long arg);
int  fpgactl_get_register(unsigned long arg);
int  fpgactl_set_register(unsigned long arg);
int  fpgactl_get_number(unsigned long arg);
int  fpgactl_get_info(unsigned long arg);
void fpgactl_exit(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FPGACTL_56F70F59_109615F7_37D96443_H */
