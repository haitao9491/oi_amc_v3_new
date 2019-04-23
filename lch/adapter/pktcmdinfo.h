/*
 *
 * pktcmdinfo.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_PKTCMDINFO_36F7F453_6B3690AC_63DD57A4_H
#define _HEAD_PKTCMDINFO_36F7F453_6B3690AC_63DD57A4_H

#include "pktcmd.h"

typedef int (*pktcmd_callback)(void *device, pktcmd_devcallback devcmdhdl,
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len);
typedef int (*pktcmd_argenc)(unsigned char *buf, int size, void *in);

struct pktcmd_cb_map {
	unsigned short id;
	char name[32];
	pktcmd_argenc inenc, outenc;
	pktcmd_callback  cmdcb, ackcb;
	pktcmd_devcallback devcmdcb, devackcb; /* Device specific cmd handler */
};

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTCMDINFO_36F7F453_6B3690AC_63DD57A4_H */
