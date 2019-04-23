/*
 *
 * cssockdef.h - A brief description goes here.
 *
 */

#ifndef _HEAD_CSSOCKDEF_1A2A06A1_2805035D_14B397B6_H
#define _HEAD_CSSOCKDEF_1A2A06A1_2805035D_14B397B6_H

#define CSSOCK_RXALIGN_BUFSZ 131072
#define CSSOCK_TXTMPBUF_SIZE 131072

#if defined(CSSOCK_TX_PFIFO)
#define CSSOCK_TXBUF_SIZE    65536
#define CSSOCK_TX_ENTRYSZ    1500
#endif

#define CSSOCK_PAYLOAD_ONLY	   0X00000001
#define CSSOCK_CUSTOMIZED_PKTSYNC  0X00000002
#define CSSOCK_NO_HEARTBEAT	   0X00000010

typedef void (*CSSOCK_CONN_CB)(char *lip, int lport, char *rip, int rport, void *arg);
typedef void (*CSSOCK_DISC_CB)(char *lip, int lport, char *rip, int rport, void *arg);

#endif /* #ifndef _HEAD_CSSOCKDEF_1A2A06A1_2805035D_14B397B6_H */
