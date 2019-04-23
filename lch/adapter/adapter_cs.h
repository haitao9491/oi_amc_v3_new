/*
 *
 * adapter_cs.h - A brief description goes here.
 *
 * There must be a configuration section to run this adapter. An
 * example config is listed below. Name of the configuration section
 * is specified by parameter 'section'.
 *

[Adapter.ClientSocket]
# FORMAT: server = IP,Port,RxBufSize,TxBufSize[,Flag[,Flag[...]]
#
# RxBufSize,TxBfSize: in KBytes
# Flag: payload - Only payload will be sent to/received from peers.
# Flag: noheartbeat - No heartbeat for pkt.
#
# Example: server = 192.168.7.100,8888,256,262144,payload
server = 192.168.1.20,7121,1024,4096
server = 192.168.1.21,7121,1024,4096

 */

#ifndef _HEAD_ADAPTER_CS_1B9D7B61_10298EC9_610D34B6_H
#define _HEAD_ADAPTER_CS_1B9D7B61_10298EC9_610D34B6_H

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

#define ADAPTER_CS_IOCTL_CLOSESOCKET 0X00000001

struct adapter_cs_addr {
	char	ip[64];
	int	port;
};

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *adapter_register_cs(unsigned long cfghd, char *section, int (*running)(void));
DLL_APP void *adapter_register_cs_cfgfile(char *cfgfile, char *section, int (*running)(void));
DLL_APP void *adapter_register_cs_cfgstr(char *cfgstr, char *section, int (*running)(void));

DLL_APP void *adapter_register_cs_c(unsigned long cfghd, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size));
DLL_APP void *adapter_register_cs_cfgfile_c(char *cfgfile, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size));
DLL_APP void *adapter_register_cs_cfgstr_c(char *cfgstr, char *section,
		int (*running)(void), int (*pktsync)(void *buf, int size));

DLL_APP void  adapter_cs_setconncb(void *adap, CSSOCK_CONN_CB conncb, void *arg);
DLL_APP void  adapter_cs_setdisccb(void *adap, CSSOCK_DISC_CB disccb, void *arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_CS_1B9D7B61_10298EC9_610D34B6_H */
