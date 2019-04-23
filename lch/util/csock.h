/*
 *
 * csock.h - Define client socket wrapper
 *
 */

#ifndef _HEAD_CSOCK_0A1A02B5_64CA7238_7A756FE1_H
#define _HEAD_CSOCK_0A1A02B5_64CA7238_7A756FE1_H

#include "cssockdef.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void *csock_open(const char *ip, int port, int rbs, int tbs, 
		int (*pktsync)(void *buf, int size));
extern int   csock_setflag(void *sh, unsigned int flag);
extern void  csock_close(void *sh);
extern void  csock_close_socket_lock(void *sh);

extern char *csock_get_localipstr(void *sh);
extern unsigned int csock_get_localip(void *sh);
extern int csock_get_localport(void *sh);

extern int   csock_is_connected(void *sh);
extern int   csock_check_connection(void *sh);

extern int   csock_flush_rx(void *sh);
extern void *csock_read(void *sh);
extern int   csock_write(void *sh, char *data, int len);
extern int   csock_flush_tx(void *sh);

extern void  csock_stat(void *sh);

extern void  csock_setconncb(void *sh, CSSOCK_CONN_CB conncb, void *arg);
extern void  csock_setdisccb(void *sh, CSSOCK_DISC_CB disccb, void *arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_CSOCK_0A1A02B5_64CA7238_7A756FE1_H */
