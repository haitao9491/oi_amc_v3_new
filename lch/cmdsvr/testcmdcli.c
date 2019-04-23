/*
 *
 * testcmdcli.c - A brief description to describe this file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#endif
#include "misc.h"
#include <time.h>
#include "os.h"
#include "aplog.h"
#include "pkttype.h"
#include "cmdsvr.h"
#include "pktcmdid.h"
#include "pktcmdarg.h"

static int CmdGetHwModelDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetHwModelOut *o = (CmdGetHwModelOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetHwModelDevACKCB: deviceModel [%u]", o->deviceModel);
	}
	else {
		LOGERROR("CmdGetHwModelDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetSwVersionDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetSwVersionOut *o = (CmdGetSwVersionOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetSwVersionDevACKCB: deviceSWVersion [%s]",
				o->deviceSWVersion);
	}
	else {
		LOGERROR("CmdGetSwVersionDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetHwDescriptionDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetHwDescriptionOut *o = (CmdGetHwDescriptionOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetHwDescriptionDevACKCB: deviceDescription [%s]",
				o->deviceDescription);
	}
	else {
		LOGERROR("CmdGetHwDescriptionDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetSysTimeDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetSysTimeOut *o = (CmdGetSysTimeOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetSysTimeDevACKCB: sysTime [%u %u]",
				o->sysTime.sec, o->sysTime.usec);
	}
	else {
		LOGERROR("CmdGetSysTimeDevACKCB: failed.");
	}

	return 0;
}

static int CmdSetSysTimeDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdSetSysTimeDevACKCB: sysTime updated.");
	}
	else {
		LOGERROR("CmdSetSysTimeDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetClkTypeDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetClkTypeOut *o = (CmdGetClkTypeOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetClkTypeDevACKCB: clock type %u, src %u.",
				o->clkType, o->clkSrc);
	}
	else {
		LOGERROR("CmdGetClkTypeDevACKCB: failed.");
	}

	return 0;
}

static int CmdSetClkTypeDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdSetClkTypeDevACKCB: clock updated.");
	}
	else {
		LOGERROR("CmdSetClkTypeDevACKCB: failed.");
	}

	return 0;
}

static int CmdDownloadChCfgFileDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	CmdDownloadChCfgFileOut *o = (CmdDownloadChCfgFileOut *)out;

	if (cmd->type == PKT_TYPE_CMDACK) {
		o->data[o->size - 1] = 0;
		LOGDEBUG("CmdDownloadChCfgFileDevACKCB: ChCfgFile %d bytes.\n%s",
				o->size, o->data);
	}
	else {
		LOGERROR("CmdDownloadChCfgFileDevACKCB: failed.");
	}

	return 0;
}

static int CmdUploadChCfgFileDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdUploadChCfgFileDevACKCB: channel configuration uploaded.");
	}
	else {
		LOGERROR("CmdUploadChCfgFileDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetDevIdDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetDevIdOut *o = (CmdGetDevIdOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetDevIdDevACKCB: deviceID %u", o->deviceID);
	}
	else {
		LOGERROR("CmdGetDevIdDevACKCB: failed.");
	}

	return 0;
}

static int CmdSetDevIdDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdSetDevIdDevACKCB: deviceId updated.");
	}
	else {
		LOGERROR("CmdSetDevIdDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetPortStatInfoDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetPortStatInfoOut *o = (CmdGetPortStatInfoOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetPortStatInfoDevACKCB: port %u, alarm %08x, "
				"lcvc %u, pcvc %u, fosc %u, ebc %u",
				o->portID,
				o->portAlarm,
				o->portStatistics.lcvc,
				o->portStatistics.pcvc,
				o->portStatistics.fosc,
				o->portStatistics.ebc);
	}
	else {
		LOGERROR("CmdGetPortStatInfoDevACKCB: failed.");
	}

	return 0;
}

static int CmdClrPortStatInfoDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdClrPortStatInfoDevACKCB: cleared.");
	}
	else {
		LOGERROR("CmdClrPortStatInfoDevACKCB: failed.");
	}

	return 0;
}

static int CmdGetChStatInfoDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	LOGDEBUGDECL(CmdGetChStatInfoOut *o = (CmdGetChStatInfoOut *)out;)

	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdGetChStatInfoDevACKCB: channel %u: "
				"rx %u %u, tx %u %u, rbuf %u %u, "
				"ll %u, fisu %u, lssu %u, msu %u, fsn %u %u, "
				"lg %u, no %u, ab %u, cr %u",
				o->channelID,
				o->channelStatistics.rframes,
				o->channelStatistics.rbytes,
				o->channelStatistics.tframes,
				o->channelStatistics.tbytes,
				o->channelStatistics.rbuf_full_frames,
				o->channelStatistics.rbuf_full_bytes,
				o->channelStatistics.ll ,
				o->channelStatistics.fisu,
				o->channelStatistics.lssu,
				o->channelStatistics.msu,
				o->channelStatistics.fsn_hop,
				o->channelStatistics.fsn_dup,
				o->channelStatistics.lg,
				o->channelStatistics.no,
				o->channelStatistics.ab,
				o->channelStatistics.cr);
	}
	else {
		LOGERROR("CmdGetChStatInfoDevACKCB: failed.");
	}

	return 0;
}

static int CmdClrChStatInfoDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	if (cmd->type == PKT_TYPE_CMDACK) {
		LOGDEBUG("CmdClrChStatInfoDevACKCB: cleared.");
	}
	else {
		LOGERROR("CmdClrChStatInfoDevACKCB: failed.");
	}

	return 0;
}

static int CmdChanScanDevACKCB(void *device,
		struct pktcmd_cmdinfo *cmd, void *in, void *out)
{
	CmdChanScanOut *o = (CmdChanScanOut *)out;

	if (cmd->type == PKT_TYPE_CMDACK) {
		o->data[o->size - 1] = 0;
		LOGDEBUG("CmdChanScanDevACKCB: ChCfgFile %d bytes.\n%s",
				o->size, o->data);
	}
	else {
		LOGERROR("CmdChanScanDevACKCB: failed.");
	}

	return 0;
}

static struct cmdsvr_callback testdevcbs[] = {
	{ CMD_GET_HW_MODEL, NULL, CmdGetHwModelDevACKCB },
	{ CMD_GET_SW_VERSION, NULL, CmdGetSwVersionDevACKCB },
	{ CMD_GET_HW_DESCRIPTION, NULL, CmdGetHwDescriptionDevACKCB },
	{ CMD_GET_SYS_TIME, NULL, CmdGetSysTimeDevACKCB },
	{ CMD_SET_SYS_TIME, NULL, CmdSetSysTimeDevACKCB },
	{ CMD_GET_CLK_TYPE, NULL, CmdGetClkTypeDevACKCB },
	{ CMD_SET_CLK_TYPE, NULL, CmdSetClkTypeDevACKCB },
	{ CMD_DOWNLOAD_CH_CFGFILE, NULL, CmdDownloadChCfgFileDevACKCB },
	{ CMD_UPLOAD_CH_CFGFILE, NULL, CmdUploadChCfgFileDevACKCB },
	{ CMD_GET_DEV_ID, NULL, CmdGetDevIdDevACKCB },
	{ CMD_SET_DEV_ID, NULL, CmdSetDevIdDevACKCB },
	{ CMD_GET_PORT_STAT_INFO, NULL, CmdGetPortStatInfoDevACKCB },
	{ CMD_CLR_PORT_STAT_INFO, NULL, CmdClrPortStatInfoDevACKCB },
	{ CMD_GET_CH_STAT_INFO, NULL, CmdGetChStatInfoDevACKCB },
	{ CMD_CLR_CH_STAT_INFO, NULL, CmdClrChStatInfoDevACKCB },
	{ CMD_CHAN_SCAN, NULL, CmdChanScanDevACKCB },
	{ 0, NULL, NULL }
};
static struct cmdsvr_device testdev;

int testdev_CMDGetHwModel(void *pc)
{
	CmdGetHwModelIn in;

	in.deviceSequenceNumber = 0;
	if (pktcmd_send(pc, CMD_GET_HW_MODEL, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetSwVersion(void *pc)
{
	CmdGetSwVersionIn in;

	in.deviceSequenceNumber = 0;
	if (pktcmd_send(pc, CMD_GET_SW_VERSION, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetHwDescription(void *pc)
{
	CmdGetHwDescriptionIn in;

	in.deviceSequenceNumber = 1;
	if (pktcmd_send(pc, CMD_GET_HW_DESCRIPTION, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetSysTime(void *pc)
{
	CmdGetSysTimeIn in;

	in.deviceSequenceNumber = 1;
	if (pktcmd_send(pc, CMD_GET_SYS_TIME, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDSetSysTime(void *pc)
{
	CmdSetSysTimeIn in;
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == -1)
		return -1;
	in.deviceSequenceNumber = 1;
	in.sysTime.sec = tv.tv_sec;
	in.sysTime.usec = tv.tv_usec;
	if (pktcmd_send(pc, CMD_SET_SYS_TIME, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetClkType(void *pc)
{
	CmdGetClkTypeIn in;

	in.deviceSequenceNumber = 1;
	if (pktcmd_send(pc, CMD_GET_CLK_TYPE, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDSetClkType(void *pc)
{
	CmdSetClkTypeIn in;

	in.deviceSequenceNumber = 1;
	in.clkType = 1;
	in.clkSrc = 2;
	if (pktcmd_send(pc, CMD_SET_CLK_TYPE, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDDownloadChCfgFile(void *pc)
{
	CmdDownloadChCfgFileIn in;

	in.deviceSequenceNumber = 1;
	if (pktcmd_send(pc, CMD_DOWNLOAD_CH_CFGFILE, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDUploadChCfgFile(void *pc)
{
	CmdUploadChCfgFileIn in;

	in.deviceSequenceNumber = 1;
	in.data = (char *)"[Global Setting]\ndevice id = 1\n\npacket type     = DATA\npacket subtype  = MTP\npacket protocol = AUTO\n\nmode = MCCTDM\n\n[Port]\n1 = E1 HDB3 CCS 20dB 120ohm\n2 = E1 HDB3 CCS 20dB 120ohm\n3 = E1 HDB3 CCS 20dB 120ohm\n4 = E1 HDB3 CCS 20dB 120ohm\n5 = E1 HDB3 CCS 20dB 120ohm\n6 = E1 HDB3 CCS 20dB 120ohm\n7 = E1 HDB3 CCS 20dB 120ohm\n8 = E1 HDB3 CCS 20dB 120ohm\n\n[MCCTDM]\nTesting = disabled\n\n[HDLC Channel Configuration]\nCH 1    1  31    1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16\n17 18 19 20 21 22 23 24 25 26 27 28 29 30 31\nCH 2    16 1     16\nCH 3    16 1     16\nCH 4    16 1     16\nCH 5    16 1     16\nCH 6    16 1     16\nCH 7    16 1     16\nCH 8    16 1     16\n";
	in.size = (BufferSize)strlen(in.data);
	if (pktcmd_send(pc, CMD_UPLOAD_CH_CFGFILE, &in, sizeof(in) + in.size) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetDevId(void *pc)
{
	CmdGetDevIdIn in;

	in.deviceSequenceNumber = 1;
	if (pktcmd_send(pc, CMD_GET_DEV_ID, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDSetDevId(void *pc)
{
	CmdSetDevIdIn in;

	in.deviceSequenceNumber = 1;
	in.deviceID = 100;
	if (pktcmd_send(pc, CMD_SET_DEV_ID, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetPortStatInfo(void *pc)
{
	CmdGetPortStatInfoIn in;

	in.deviceSequenceNumber = 1;
	in.portID = 2;
	if (pktcmd_send(pc, CMD_GET_PORT_STAT_INFO, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDClrPortStatInfo(void *pc)
{
	CmdClrPortStatInfoIn in;

	in.deviceSequenceNumber = 1;
	in.portID = 2;
	if (pktcmd_send(pc, CMD_CLR_PORT_STAT_INFO, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDGetChStatInfo(void *pc)
{
	CmdGetChStatInfoIn in;

	in.deviceSequenceNumber = 1;
	in.channelID = 2;
	if (pktcmd_send(pc, CMD_GET_CH_STAT_INFO, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDClrChStatInfo(void *pc)
{
	CmdClrChStatInfoIn in;

	in.deviceSequenceNumber = 1;
	in.channelID = 2;
	if (pktcmd_send(pc, CMD_CLR_CH_STAT_INFO, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

int testdev_CMDChanScan(void *pc)
{
	CmdChanScanIn in;

	in.deviceSequenceNumber = 1;
	in.scanInterval = 3;
	in.scanType = 2;
	in.scanNList = 0x40000003;
	if (pktcmd_send(pc, CMD_CHAN_SCAN, &in, sizeof(in)) == 0)
		return 1;

	return 0;
}

typedef int (*testdev_cmd)(void *pc);

testdev_cmd testdev_cmds[] = {
	testdev_CMDGetHwModel,
	testdev_CMDGetSwVersion,
	testdev_CMDGetHwDescription,
	testdev_CMDGetSysTime,
	testdev_CMDSetSysTime,
	testdev_CMDGetSysTime,
	testdev_CMDGetClkType,
	testdev_CMDSetClkType,
	testdev_CMDGetClkType,
	testdev_CMDGetDevId,
	testdev_CMDSetDevId,
	testdev_CMDGetDevId,
	testdev_CMDGetPortStatInfo,
	testdev_CMDClrPortStatInfo,
	testdev_CMDGetPortStatInfo,
	testdev_CMDGetChStatInfo,
	testdev_CMDClrChStatInfo,
	testdev_CMDGetChStatInfo,
	testdev_CMDDownloadChCfgFile,
	testdev_CMDUploadChCfgFile,
	testdev_CMDChanScan,
	NULL
};

int testdev_send(void *pc)
{
	static int seq = 0;

	if (testdev_cmds[seq] == NULL)
		return 0;

	return testdev_cmds[seq++](pc);
}

struct cmdsvr_device *cmdsvr_register_device(void)
{
	testdev.name = "Testdev";
	testdev.device = NULL;
	testdev.devcbs = testdevcbs;
	testdev.send = testdev_send;

	return &testdev;
}

void cmdsvr_release_device(struct cmdsvr_device *dev)
{
}

