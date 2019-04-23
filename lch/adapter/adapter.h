/*
 *
 * adapter.h - A brief description goes here.
 *
 */

#ifndef _HEAD_ADAPTER_1B49230D_52E00F49_5AF0C50F_H
#define _HEAD_ADAPTER_1B49230D_52E00F49_5AF0C50F_H

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

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

typedef void (*adap_pkt_cb)(int tid,
		void *ph, int len, char *ip, int port, void *priv);

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP int adapter_open(void *adap);
DLL_APP void *adapter_read(void *adap);
DLL_APP void *adapter_read_from(void *adap, char *ip, int *port);
DLL_APP int adapter_write(void *adap, void *data, int len);
DLL_APP int adapter_write_to(void *adap, void *data, int len, char *ip, int port);
DLL_APP void adapter_set_pkt_cb(void *adap, void *priv, adap_pkt_cb pcb);
DLL_APP int adapter_ioctl(void *adap, int code, void *arg);
DLL_APP void adapter_freebuf(void *adap, void *buf);
DLL_APP void adapter_close(void *adap);

/* For Adapter Implementation ONLY */
void *adapter_allocate(void);
void adapter_set_open(void *adap, int (*adap_open)(void *adap));
void adapter_set_read(void *adap, void *(*adap_read)(void *adap));
void adapter_set_read_from(void *adap,
		void *(*adap_read_from)(void *adap, char *ip, int *port));
void adapter_set_write(void *adap,
		int (*adap_write)(void *adap, void *data, int len));
void adapter_set_write_to(void *adap,
		int (*adap_write_to)(void *adap, void *data, int len, char *ip, int port));
void adapter_set_ioctl(void *adap,
		int (*adap_ioctl)(void *adap, int code, void *arg));
void adapter_set_freebuf(void *adap,
		void (*adap_freebuf)(void *adap, void *buf));
void adapter_set_close(void *adap, void (*adap_close)(void *adap));
void adapter_set_set_pkt_cb(void *adap,
		void (*adap_set_pkt_cb)(void *adap, void *priv, adap_pkt_cb pcb));
void adapter_set_data(void *adap, void *data);
void *adapter_get_data(void *adap);
void adapter_set_name(void *adap, char *adap_name);
char *adapter_get_name(void *adap);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_1B49230D_52E00F49_5AF0C50F_H */
