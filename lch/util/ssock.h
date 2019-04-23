/*
 *
 * ssock.h - Define server socket wrapper
 *
 */

#ifndef _HEAD_SSOCK_2470D88E_34B62725_4834479F_H
#define _HEAD_SSOCK_2470D88E_34B62725_4834479F_H

#include "cssockdef.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void *ssock_open(int port, int backlog, int (*running)(void), int (*pktsync)(void *buf, int size));
extern void *ssock_open_f(unsigned int ip, int port, int backlog, int (*running)(void),
		int (*pktsync)(void *buf, int size));
extern void  ssock_close(void *sh);

extern void  ssock_setflag(void *sh, unsigned int flag);
extern void  ssock_set_bufsize(void *sh, int rbs, int tbs);
extern int   ssock_get_port(void *sh);
extern int   ssock_peers(void *sh);

extern void *ssock_read_from(void *sh, char *ip, int *port);
extern void *ssock_read(void *sh);
extern int   ssock_write_to(void *sh, char *data, int len, char *ip, int port);
extern int   ssock_broadcast(void *sh, char *data, int len);
extern void  ssock_disconnect_peer(void *sh, char *ip, int port);
extern void *ssock_get_conn(void *sh, char *ip, int port); 

extern void  ssock_set_conncb(void *sh, CSSOCK_CONN_CB conncb, void *arg);
extern void  ssock_set_disccb(void *sh, CSSOCK_DISC_CB disccb, void *arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SSOCK_2470D88E_34B62725_4834479F_H */
