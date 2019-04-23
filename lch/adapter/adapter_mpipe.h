/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * adapter_mpipe.h - A description goes here.
 *
 * There must be a configuration section to run this adapter. An
 * example config is listed below.

[Adapter.mPIPE]
# port = LinkName[,Flag]
#
# LinkName: link name
# Flag: tx - This interface will be used to send packets.
#
port = xgbe0,tx
port = xgbe1
port = xgbe2,tx
port = xgbe3

# iqueue.number = N[@<dpcore>]
#
# N: the number of ingress queues, must be power-of-two
# dpcore: the start dediated core for iqueue processing
#
iqueue.number = 4

 *
 */

#ifndef _HEAD_ADAPTER_MPIPE_17230D5E_1B27046D_08664E5E_H
#define _HEAD_ADAPTER_MPIPE_17230D5E_1B27046D_08664E5E_H

#define ADAPTER_MPIPE_IOCTL_GETIQUEUESTAT 1
#define ADAPTER_MPIPE_IOCTL_GETILINKSTAT  2
#define ADAPTER_MPIPE_IOCTL_GETELINKSTAT  3
#define ADAPTER_MPIPE_IOCTL_STOPRX        4
#define ADAPTER_MPIPE_IOCTL_GETMPIPESTAT  5

#if defined(__cplusplus)
extern "C" {
#endif

void *adapter_register_mpipe(unsigned long cfghd, char *section, int sidx);
void *adapter_register_mpipe_cfgfile(char *cfgfile, char *section, int sidx);
void *adapter_register_mpipe_cfgstr(char *cfgstr, char *section, int sidx);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_MPIPE_17230D5E_1B27046D_08664E5E_H */
