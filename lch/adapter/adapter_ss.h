/*
 *
 * adapter_ss.h - A brief description goes here.
 *
 * There must be a configuration section to run this adapter. An
 * example config is listed below. Name of the configuration section
 * is specified by parameter 'section'.
 *

[Adapter.ServerSocket]
# FORMAT: server = Port,RxBufSize,TxBufSize[,Flag[,Flag[...]]
#
# RxBufSize,TxBfSize: in KBytes
# Flag: payload - Only payload will be sent to/received from peers.
# Flag: noheartbeat - No heartbeat for pkt.
#
# Example: server = 7121,131072,131072,payload
server = 7121,1024,4096

 */

#ifndef _HEAD_ADAPTER_SS_57ED0E2E_48F726D0_56C4D66C_H
#define _HEAD_ADAPTER_SS_57ED0E2E_48F726D0_56C4D66C_H

#include "cssockdef.h"

#ifndef DLL_APP
#ifdef WIN32
#ifdef _USRDLL
#define DLL_APP _declspec(dllexport)
#else
#define DLL_APP _declspec(dllimport)
#endif
#else
#define DLL_APP
#endif
#endif

#define ADAPTER_SS_IOCTL_GETADDR 1
#define ADAPTER_SS_IOCTL_DISCADDR 2

struct adapter_ss_addr {
	unsigned int ip;
	int          port;
};

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *adapter_register_ss(unsigned long cfghd, char *section, int (*running)(void));
DLL_APP void *adapter_register_ss_cfgfile(char *cfgfile, char *section, int (*running)(void));
DLL_APP void *adapter_register_ss_cfgstr(char *cfgstr, char *section, int (*running)(void));

DLL_APP void *adapter_register_ss_c(unsigned long cfghd, char *section, 
		int (*running)(void), int (*pktsync)(void *buf, int size));
DLL_APP void *adapter_register_ss_cfgfile_c(char *cfgfile, char *section, 
		int (*running)(void), int (*pktsync)(void *buf, int size));
DLL_APP void *adapter_register_ss_cfgstr_c(char *cfgstr, char *section, 
		int (*running)(void), int (*pktsync)(void *buf, int size));

DLL_APP void  adapter_ss_setconncb(void *adap, CSSOCK_CONN_CB conncb, void *arg);
DLL_APP void  adapter_ss_setdisccb(void *adap, CSSOCK_DISC_CB disccb, void *arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_SS_57ED0E2E_48F726D0_56C4D66C_H */
