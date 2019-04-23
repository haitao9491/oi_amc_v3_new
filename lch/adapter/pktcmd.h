/*
 *
 * pktcmd.h - A brief description goes here.
 *
 */

#ifndef _HEAD_PKTCMD_5E68192D_5DDE23BE_16E430E1_H
#define _HEAD_PKTCMD_5E68192D_5DDE23BE_16E430E1_H

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

#include "pkt.h"

struct pktcmd_cmdinfo {
	/* The original command packet we received from the peer. Command
	 * arguments can be derived from it.
	 */
	pkt_hdr        *ph;

	/* Where this command packet is from. */
	char ip[32];
	int  port;

	unsigned short  id;
	unsigned char   type; /* PKT_TYPE_CMD, PKT_TYPE_CMDACK, PKT_TYPE_CMDNACK */

	/* PROGRAMMERS WHO IMPLEMENT COMMAND PROCESSING HANDLERS SHOULD
	 * NOT TOUCH THE VARIABLES BELOW.
	 */

	/* If we'd like to generate an ack, we must allocate a buffer
	 * to hold the ack packet. The size of the buffer is saved in
	 * 'size'. Be careful that the first sizeof(pkt_hdr) bytes are
	 * for the header of this ack packet.
	 */
	unsigned char *result;
	int            size;
	unsigned char  ack_type; /* PKT_TYPE_CMDACK, PKT_TYPE_CMDNACK */

	void *adap;
};

typedef int (*pktcmd_devcallback)(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out);

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *pktcmd_open(void *adap, void *device);
DLL_APP int   pktcmd_register_hdl_ex(void *pc, void *cbmap);
DLL_APP int   pktcmd_register_devhdl(void *pc, unsigned short id,
		pktcmd_devcallback cmdcb, pktcmd_devcallback ackcb);
DLL_APP int   pktcmd_process(void *pc);
DLL_APP int   pktcmd_sendto(void *pc, unsigned short id, void *in, int len,
		char *ip, int port);
DLL_APP int pktcmd_send(void *pc, unsigned short id, void *in, int len);
DLL_APP int pktcmd_sendackto(void *pc, unsigned short id, void *out, int len,
		char *ip, int port);
DLL_APP int pktcmd_sendack(void *pc, unsigned short id, void *out, int len);
DLL_APP int pktcmd_sendnackto(void *pc, unsigned short id, void *out, int len,
		char *ip, int port);
DLL_APP int pktcmd_sendnack(void *pc, unsigned short id, void *out, int len);
DLL_APP void  pktcmd_close(void *pc);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTCMD_5E68192D_5DDE23BE_16E430E1_H */
