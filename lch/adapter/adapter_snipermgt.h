/*
 * (C) Copyright 2013
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * adapter_snipermgt.h - A description goes here.
 *
 * There must be a configuration section to run this adapter. An
 * example config is listed below. Name of the configuration section
 * is specified by parameter 'section'.
 *

[Adapter.SNIPERMGTServerSocket]
# FORMAT: server = Port,RxBufSize,TxBufSize[,Flag[,Flag[...]]
#
# RxBufSize,TxBfSize: in KBytes
# Flag: payload - Only payload will be sent to/received from peers.
#
# Example: server = 7121,131072,131072,payload
server = 7121,1024,4096
 *
 */

#ifndef _HEAD_ADAPTER_SNIPERMGT_46B87591_56BA1C88_4CFC1D34_H
#define _HEAD_ADAPTER_SNIPERMGT_46B87591_56BA1C88_4CFC1D34_H

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

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *adapter_register_snipermgt(unsigned long cfghd, char *section, int (*running)(void));
DLL_APP void *adapter_register_snipermgt_cfgfile(char *cfgfile, char *section, int (*running)(void));
DLL_APP void *adapter_register_snipermgt_cfgstr(char *cfgstr, char *section, int (*running)(void));

DLL_APP void  adapter_snipermgt_setconncb(void *adap, CSSOCK_CONN_CB conncb, void *arg);
DLL_APP void  adapter_snipermgt_setdisccb(void *adap, CSSOCK_DISC_CB disccb, void *arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_SNIPERMGT_46B87591_56BA1C88_4CFC1D34_H */
