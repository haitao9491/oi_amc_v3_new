/*
 *
 * cmdsvr.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_CMDSVR_460502DE_4844AA8A_7C7CEA3E_H
#define _HEAD_CMDSVR_460502DE_4844AA8A_7C7CEA3E_H

#include "pktcmd.h"

struct cmdsvr_callback {
	unsigned short      id;
	pktcmd_devcallback  cmdcb;
	pktcmd_devcallback  ackcb;
};

struct cmdsvr_device {
	char *name;
	void *device;
	struct cmdsvr_callback *devcbs;
	int (*send)(void *pc);
};

#if defined(CMDSVR_APFRM)

#if defined(__cplusplus)
extern "C" {
#endif

struct cmdsvr_device *cmdsvr_register_device(void);
void cmdsvr_release_device(struct cmdsvr_device *dev);

#if defined(__cplusplus)
}
#endif

#endif

#endif /* #ifndef _HEAD_CMDSVR_460502DE_4844AA8A_7C7CEA3E_H */
