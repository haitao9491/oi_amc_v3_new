/*
 *
 * mpktconn.h - A brief description goes here.
 *
 */

#ifndef _HEAD_MPKTCONN_4769FE0F_6CC842A6_08953764_H
#define _HEAD_MPKTCONN_4769FE0F_6CC842A6_08953764_H

#if defined(__cplusplus)
extern "C" {
#endif

void *mpktconn_open(int (*check_running)(void), int (*pktsync)(void *buf, int size));
int   mpktconn_set_op(void *hd, int rbs, int tbs, unsigned int flag);

int   mpktconn_add_pktsvr(void *hd, char *ip, int port);
int   mpktconn_add_pktsvr_op(void *hd, char *ip, int port,
		int rbs, int tbs, unsigned int flag);

int   mpktconn_start(void *hd);
void *mpktconn_read(void *hd);
void *mpktconn_read_from(void *hd, char *ip, int *port);
int   mpktconn_write(void *hd, char *data, int len);
int   mpktconn_write_to(void *hd, char *data, int len, char *ip, int port);
void  mpktconn_stop(void *hd);
void  mpktconn_close(void *hd);
void  mpktconn_close_svr(void *hd, char *ip, int port);

void  mpktconn_set_conncb(void *hd, CSSOCK_CONN_CB conncb, void *arg);
void  mpktconn_set_disccb(void *hd, CSSOCK_DISC_CB disccb, void *arg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_MPKTCONN_4769FE0F_6CC842A6_08953764_H */
