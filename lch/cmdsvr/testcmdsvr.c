/*
 *
 * testcmdsvr.c - A brief description to describe this file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "cmdsvr.h"
#include "pktcmd.h"
#include "pktcmdid.h"
#include "pktcmdarg.h"

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

static int CmdGetHwModelDevCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetHwModelIn *i = (CmdGetHwModelIn *)in;)
	CmdGetHwModelOut *o = (CmdGetHwModelOut *)out;

	LOGDEBUG("CmdGetHwModelDevCB: deviceSequenceNumber %u",
			i->deviceSequenceNumber);
	o->deviceModel = 0x41;
	LOGDEBUG("CmdGetHwModelDevCB: deviceModel %u", o->deviceModel);

	return 0;
}

static int CmdGetHwDescriptionDevCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetHwDescriptionIn *i = (CmdGetHwDescriptionIn *)in;)
	CmdGetHwDescriptionOut *o = (CmdGetHwDescriptionOut *)out;

	LOGDEBUG("CmdGetHwDescriptionDevCB: deviceSequenceNumber %u",
			i->deviceSequenceNumber);
	strcpy(o->deviceDescription, "This is a testing device.");
	LOGDEBUG("CmdGetHwDescriptionDevCB: deviceDescription %s",
			o->deviceDescription);

	return 0;
}

static struct cmdsvr_callback testdevcbs[] = {
	{ CMD_GET_HW_MODEL, CmdGetHwModelDevCB, NULL },
	{ CMD_GET_HW_DESCRIPTION, CmdGetHwDescriptionDevCB, NULL },
	{ 0, NULL, NULL }
};
static struct cmdsvr_device testdev;

struct cmdsvr_device *cmdsvr_register_device(void)
{
	testdev.name = "Testdev";
	testdev.device = NULL;
	testdev.devcbs = testdevcbs;
	testdev.send = NULL;

	return &testdev;
}

void cmdsvr_release_device(struct cmdsvr_device *dev)
{
}

