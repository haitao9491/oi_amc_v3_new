/*
 *
 * pktcmdhdl.c - A brief description to describe this file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pktcmd.h"
#include "pktcmdarg.h"
#include "pktcmdid.h"
#include "pktcmdhdl.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern unsigned char *pktcmd_allocate_result(struct pktcmd_cmdinfo *cmd,
		unsigned int size);

#if defined(__cplusplus)
}
#endif

#define DEF_PKTCMDHDL_IN(cmdname) \
int Cmd##cmdname##Hdl(void *device, pktcmd_devcallback cmdcb,    \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##In in;                                         \
	                                                             \
	if (cmd->type != PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	if (pktcmd_allocate_result(cmd, 0) == NULL)                  \
		return -1;                                               \
		                                                         \
	memset(&in, 0, sizeof(in));                                  \
	if (Cmd##cmdname##InDecode(arg, len, &in) < 0)               \
		return -1;                                               \
		                                                         \
	if (!cmdcb || ((*cmdcb)(device, cmd, &in, NULL) < 0))  \
		return -1;                                               \
		                                                         \
	return 0;                                                    \
}

#define DEF_PKTCMDHDL_IN_OUT(cmdname) \
int Cmd##cmdname##Hdl(void *device, pktcmd_devcallback cmdcb,    \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##In  in;                                        \
	Cmd##cmdname##Out out;                                       \
	unsigned char *res;                                          \
	unsigned int   size = sizeof(out) + 64;                      \
	int osize;                                                   \
	                                                             \
	if (cmd->type != PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	res = pktcmd_allocate_result(cmd, size);                     \
	if (res == NULL)                                             \
		return -1;                                               \
		                                                         \
	memset(&in, 0, sizeof(in));                                  \
	if (Cmd##cmdname##InDecode(arg, len, &in) < 0)               \
		return -1;                                               \
		                                                         \
	memset(&out, 0, sizeof(out));                                \
	if (!cmdcb || ((*cmdcb)(device, cmd, &in, &out) < 0))  \
		return -1;                                               \
		                                                         \
	if ((osize = Cmd##cmdname##OutEncode(res, size, &out)) < 0)  \
		return -1;                                               \
		                                                         \
	cmd->size = sizeof(pkt_hdr) + osize;                         \
		                                                         \
	return 0;                                                    \
}

#define DEF_PKTCMDHDL_IN_OUT_VARSIZE(cmdname) \
int Cmd##cmdname##Hdl(void *device, pktcmd_devcallback cmdcb,    \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##In  in;                                        \
	Cmd##cmdname##Out out;                                       \
	unsigned char *res;                                          \
	unsigned int   size = sizeof(out) + 64;                      \
	int osize;                                                   \
		                                                         \
	if (cmd->type != PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	memset(&in, 0, sizeof(in));                                  \
	if (Cmd##cmdname##InDecode(arg, len, &in) < 0) {             \
		pktcmd_allocate_result(cmd, 0);                          \
		return -1;                                               \
	}                                                            \
		                                                         \
	memset(&out, 0, sizeof(out));                                \
	if (!cmdcb || ((*cmdcb)(device, cmd, &in, &out) < 0)) { \
		pktcmd_allocate_result(cmd, 0);                          \
		return -1;                                               \
	}                                                            \
	                                                             \
	size += out.size;                                            \
	res = pktcmd_allocate_result(cmd, size);                     \
	if (res == NULL) {                                           \
		if ((out.size > 0) && (out.data != NULL)) {              \
			free(out.data);                                      \
		}                                                        \
		return -1;                                               \
	}                                                            \
		                                                         \
	if ((osize = Cmd##cmdname##OutEncode(res, size, &out)) < 0) { \
		if ((out.size > 0) && (out.data != NULL)) {              \
			free(out.data);                                      \
		}                                                        \
		return -1;                                               \
	}                                                            \
		                                                         \
	if ((out.size > 0) && (out.data != NULL)) {                  \
		free(out.data);                                          \
	}                                                            \
		                                                         \
	cmd->size = sizeof(pkt_hdr) + osize;                         \
		                                                         \
	return 0;                                                    \
}

#define DEF_PKTCMDHDL_IN_VARSIZE(cmdname) \
int Cmd##cmdname##Hdl(void *device, pktcmd_devcallback cmdcb,    \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##In in;                                         \
	                                                             \
	if (cmd->type != PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	if (pktcmd_allocate_result(cmd, 0) == NULL)                  \
		return -1;                                               \
		                                                         \
	memset(&in, 0, sizeof(in));                                  \
	if (Cmd##cmdname##InDecode(arg, len, &in) < 0)               \
		return -1;                                               \
		                                                         \
	if (!cmdcb || ((*cmdcb)(device, cmd, &in, NULL) < 0)) { \
		if ((in.size > 0) && (in.data != NULL)) {                \
			free(in.data);                                       \
		}                                                        \
		return -1;                                               \
	}                                                            \
		                                                         \
	if ((in.size > 0) && (in.data != NULL)) {                    \
		free(in.data);                                           \
	}                                                            \
		                                                         \
	return 0;                                                    \
}

#define DEF_PKTCMDHDL_IN_VARSIZE_OUT_VARSIZE(cmdname) \
int Cmd##cmdname##Hdl(void *device, pktcmd_devcallback cmdcb,    \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##In  in;                                        \
	Cmd##cmdname##Out out;                                       \
	unsigned char *res;                                          \
	unsigned int   size = sizeof(out) + 64;                      \
	int osize;                                                   \
		                                                         \
	if (cmd->type != PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	memset(&in, 0, sizeof(in));                                  \
	if (Cmd##cmdname##InDecode(arg, len, &in) < 0) {             \
		pktcmd_allocate_result(cmd, 0);                          \
		return -1;                                               \
	}                                                            \
		                                                         \
	memset(&out, 0, sizeof(out));                                \
	if (!cmdcb || ((*cmdcb)(device, cmd, &in, &out) < 0)) { \
		pktcmd_allocate_result(cmd, 0);                          \
		if ((in.size > 0) && (in.data != NULL)) {                \
			free(in.data);                                       \
		}                                                        \
		return -1;                                               \
	}                                                            \
	                                                             \
	size += out.size;                                            \
	res = pktcmd_allocate_result(cmd, size);                     \
	if (res == NULL) {                                           \
		if ((in.size > 0) && (in.data != NULL)) {                \
			free(in.data);                                       \
		}                                                        \
		if ((out.size > 0) && (out.data != NULL)) {              \
			free(out.data);                                      \
		}                                                        \
		return -1;                                               \
	}                                                            \
		                                                         \
	if ((osize = Cmd##cmdname##OutEncode(res, size, &out)) < 0) { \
		if ((in.size > 0) && (in.data != NULL)) {                \
			free(in.data);                                       \
		}                                                        \
		if ((out.size > 0) && (out.data != NULL)) {              \
			free(out.data);                                      \
		}                                                        \
		return -1;                                               \
	}                                                            \
		                                                         \
	if ((in.size > 0) && (in.data != NULL)) {                    \
		free(in.data);                                           \
	}                                                            \
		                                                         \
	if ((out.size > 0) && (out.data != NULL)) {                  \
		free(out.data);                                          \
	}                                                            \
		                                                         \
	cmd->size = sizeof(pkt_hdr) + osize;                         \
		                                                         \
	return 0;                                                    \
}

DEF_PKTCMDHDL_IN_OUT(GetHwModel)
DEF_PKTCMDHDL_IN_OUT(GetHwVersion)
DEF_PKTCMDHDL_IN_OUT(GetSwVersion)
DEF_PKTCMDHDL_IN_OUT(GetHwSerialNum)
DEF_PKTCMDHDL_IN_OUT(GetHwDescription)
DEF_PKTCMDHDL_IN(SetHwDescription)
DEF_PKTCMDHDL_IN_OUT(GetEtherInfo)
DEF_PKTCMDHDL_IN(SetEtherInfo)
DEF_PKTCMDHDL_IN_OUT(GetMac)
DEF_PKTCMDHDL_IN(SetMac)
DEF_PKTCMDHDL_IN_OUT(GetServerIp)
DEF_PKTCMDHDL_IN(SetServerIp)
DEF_PKTCMDHDL_IN_OUT(GetGatewayIp)
DEF_PKTCMDHDL_IN(SetGatewayIp)
DEF_PKTCMDHDL_IN_OUT(GetSysTime)
DEF_PKTCMDHDL_IN(SetSysTime)
DEF_PKTCMDHDL_IN_OUT(GetNtpMode)
DEF_PKTCMDHDL_IN(SetNtpMode)
DEF_PKTCMDHDL_IN_OUT(GetClkType)
DEF_PKTCMDHDL_IN(SetClkType)
DEF_PKTCMDHDL_IN_OUT(GetPorts)
DEF_PKTCMDHDL_IN_OUT(GetGlobalPortCfg)
DEF_PKTCMDHDL_IN(SetGlobalPortCfg)
DEF_PKTCMDHDL_IN_OUT(GetPortCfg)
DEF_PKTCMDHDL_IN(SetPortCfg)
DEF_PKTCMDHDL_IN(DelPortCfg)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(DownloadPortCfgFile)
DEF_PKTCMDHDL_IN_VARSIZE(UploadPortCfgFile)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(DownloadChCfgFile)
DEF_PKTCMDHDL_IN_VARSIZE(UploadChCfgFile)
DEF_PKTCMDHDL_IN_OUT(GetDevId)
DEF_PKTCMDHDL_IN(SetDevId)
DEF_PKTCMDHDL_IN_OUT(GetPackType)
DEF_PKTCMDHDL_IN(SetPackType)
DEF_PKTCMDHDL_IN_OUT(GetDevMode)
DEF_PKTCMDHDL_IN(SetDevMode)
DEF_PKTCMDHDL_IN_OUT(GetPortSdhMatrixCfg)
DEF_PKTCMDHDL_IN(SetPortSdhMatrixCfg)
DEF_PKTCMDHDL_IN_OUT(GetPortMatrixCfg)
DEF_PKTCMDHDL_IN(SetPortMatrixCfg)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(DownloadMatrixCfgFile)
DEF_PKTCMDHDL_IN_VARSIZE(UploadMatrixCfgFile)
DEF_PKTCMDHDL_IN(GainScan)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(ChanScan)
DEF_PKTCMDHDL_IN_OUT(GetGlobalBertCfg)
DEF_PKTCMDHDL_IN(SetGlobalBertCfg)
DEF_PKTCMDHDL_IN_OUT(GetBertCfg)
DEF_PKTCMDHDL_IN(SetBertCfg)
DEF_PKTCMDHDL_IN(DelBertCfg)
DEF_PKTCMDHDL_IN(StartPrbs)
DEF_PKTCMDHDL_IN(StopPrbs)
DEF_PKTCMDHDL_IN(InsertError)
DEF_PKTCMDHDL_IN(SysReboot)
DEF_PKTCMDHDL_IN(SysStart)
DEF_PKTCMDHDL_IN(SysStop)
DEF_PKTCMDHDL_IN_VARSIZE(SysFirmwareUpgrade)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(GetSysResRep)
DEF_PKTCMDHDL_IN_OUT(GetLineStatInfo)
DEF_PKTCMDHDL_IN(ClrLineStatInfo)
DEF_PKTCMDHDL_IN_OUT(GetPortStatInfo)
DEF_PKTCMDHDL_IN(ClrPortStatInfo)
DEF_PKTCMDHDL_IN(StartPfmTest)
DEF_PKTCMDHDL_IN_OUT(GetPortPfmResult)
DEF_PKTCMDHDL_IN(StopPfmTest)
DEF_PKTCMDHDL_IN_OUT(GetChCfg)
DEF_PKTCMDHDL_IN_OUT(GetChStatInfo)
DEF_PKTCMDHDL_IN(ClrChStatInfo)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(GetRawData)
DEF_PKTCMDHDL_IN_OUT_VARSIZE(GetRtVoice)
DEF_PKTCMDHDL_IN_VARSIZE_OUT_VARSIZE(GetCallTrace)

#define DEF_PKTCMDACKHDL_OUT(cmdname) \
int Cmd##cmdname##ACKHdl(void *device, pktcmd_devcallback ackcb, \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##Out out;                                       \
	                                                             \
	if (cmd->type == PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	memset(&out, 0, sizeof(out));                                \
	if ((cmd->type == PKT_TYPE_CMDACK) &&                        \
	    (Cmd##cmdname##OutDecode(arg, len, &out) < 0))           \
		return -1;                                               \
		                                                         \
	if (!ackcb || ((*ackcb)(device, cmd, NULL, &out) < 0)) \
		return -1;                                               \
		                                                         \
	return 0;                                                    \
}

#define DEF_PKTCMDACKHDL_OUT_VARSIZE(cmdname) \
int Cmd##cmdname##ACKHdl(void *device, pktcmd_devcallback ackcb, \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	Cmd##cmdname##Out out;                                       \
	                                                             \
	if (cmd->type == PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	memset(&out, 0, sizeof(out));                                \
	if ((cmd->type == PKT_TYPE_CMDACK) &&                        \
	    (Cmd##cmdname##OutDecode(arg, len, &out) < 0))           \
		return -1;                                               \
		                                                         \
	if (!ackcb || ((*ackcb)(device, cmd, NULL, &out) < 0)) { \
		if ((out.size > 0) && (out.data != NULL)) {              \
			free(out.data);                                      \
		}                                                        \
		return -1;                                               \
	}                                                            \
	                                                             \
	if ((out.size > 0) && (out.data != NULL)) {                  \
		free(out.data);                                          \
	}                                                            \
	                                                             \
	return 0;                                                    \
}

#define DEF_PKTCMDACKHDL(cmdname) \
int Cmd##cmdname##ACKHdl(void *device, pktcmd_devcallback ackcb, \
		struct pktcmd_cmdinfo *cmd, unsigned char *arg, int len) \
{                                                                \
	if (cmd->type == PKT_TYPE_CMD)                               \
		return -1;                                               \
	                                                             \
	if (!ackcb || ((*ackcb)(device, cmd, NULL, NULL) < 0)) \
		return -1;                                               \
		                                                         \
	return 0;                                                    \
}

DEF_PKTCMDACKHDL_OUT(GetHwModel)
DEF_PKTCMDACKHDL_OUT(GetHwVersion)
DEF_PKTCMDACKHDL_OUT(GetSwVersion)
DEF_PKTCMDACKHDL_OUT(GetHwSerialNum)
DEF_PKTCMDACKHDL_OUT(GetHwDescription)
DEF_PKTCMDACKHDL(SetHwDescription)
DEF_PKTCMDACKHDL_OUT(GetEtherInfo)
DEF_PKTCMDACKHDL(SetEtherInfo)
DEF_PKTCMDACKHDL_OUT(GetMac)
DEF_PKTCMDACKHDL(SetMac)
DEF_PKTCMDACKHDL_OUT(GetServerIp)
DEF_PKTCMDACKHDL(SetServerIp)
DEF_PKTCMDACKHDL_OUT(GetGatewayIp)
DEF_PKTCMDACKHDL(SetGatewayIp)
DEF_PKTCMDACKHDL_OUT(GetSysTime)
DEF_PKTCMDACKHDL(SetSysTime)
DEF_PKTCMDACKHDL_OUT(GetNtpMode)
DEF_PKTCMDACKHDL(SetNtpMode)
DEF_PKTCMDACKHDL_OUT(GetClkType)
DEF_PKTCMDACKHDL(SetClkType)
DEF_PKTCMDACKHDL_OUT(GetPorts)
DEF_PKTCMDACKHDL_OUT(GetGlobalPortCfg)
DEF_PKTCMDACKHDL(SetGlobalPortCfg)
DEF_PKTCMDACKHDL_OUT(GetPortCfg)
DEF_PKTCMDACKHDL(SetPortCfg)
DEF_PKTCMDACKHDL(DelPortCfg)
DEF_PKTCMDACKHDL_OUT_VARSIZE(DownloadPortCfgFile)
DEF_PKTCMDACKHDL(UploadPortCfgFile)
DEF_PKTCMDACKHDL_OUT_VARSIZE(DownloadChCfgFile)
DEF_PKTCMDACKHDL(UploadChCfgFile)
DEF_PKTCMDACKHDL_OUT(GetDevId)
DEF_PKTCMDACKHDL(SetDevId)
DEF_PKTCMDACKHDL_OUT(GetPackType)
DEF_PKTCMDACKHDL(SetPackType)
DEF_PKTCMDACKHDL_OUT(GetDevMode)
DEF_PKTCMDACKHDL(SetDevMode)
DEF_PKTCMDACKHDL_OUT(GetPortSdhMatrixCfg)
DEF_PKTCMDACKHDL(SetPortSdhMatrixCfg)
DEF_PKTCMDACKHDL_OUT(GetPortMatrixCfg)
DEF_PKTCMDACKHDL(SetPortMatrixCfg)
DEF_PKTCMDACKHDL_OUT_VARSIZE(DownloadMatrixCfgFile)
DEF_PKTCMDACKHDL(UploadMatrixCfgFile)
DEF_PKTCMDACKHDL(GainScan)
DEF_PKTCMDACKHDL_OUT_VARSIZE(ChanScan)
DEF_PKTCMDACKHDL_OUT(GetGlobalBertCfg)
DEF_PKTCMDACKHDL(SetGlobalBertCfg)
DEF_PKTCMDACKHDL_OUT(GetBertCfg)
DEF_PKTCMDACKHDL(SetBertCfg)
DEF_PKTCMDACKHDL(DelBertCfg)
DEF_PKTCMDACKHDL(StartPrbs)
DEF_PKTCMDACKHDL(StopPrbs)
DEF_PKTCMDACKHDL(InsertError)
DEF_PKTCMDACKHDL(SysReboot)
DEF_PKTCMDACKHDL(SysStart)
DEF_PKTCMDACKHDL(SysStop)
DEF_PKTCMDACKHDL(SysFirmwareUpgrade)
DEF_PKTCMDACKHDL_OUT_VARSIZE(GetSysResRep)
DEF_PKTCMDACKHDL_OUT(GetLineStatInfo)
DEF_PKTCMDACKHDL(ClrLineStatInfo)
DEF_PKTCMDACKHDL_OUT(GetPortStatInfo)
DEF_PKTCMDACKHDL(ClrPortStatInfo)
DEF_PKTCMDACKHDL(StartPfmTest)
DEF_PKTCMDACKHDL_OUT(GetPortPfmResult)
DEF_PKTCMDACKHDL(StopPfmTest)
DEF_PKTCMDACKHDL_OUT(GetChCfg)
DEF_PKTCMDACKHDL_OUT(GetChStatInfo)
DEF_PKTCMDACKHDL(ClrChStatInfo)
DEF_PKTCMDACKHDL_OUT_VARSIZE(GetRawData)
DEF_PKTCMDACKHDL_OUT_VARSIZE(GetRtVoice)
DEF_PKTCMDACKHDL_OUT_VARSIZE(GetCallTrace)

struct pktcmd_cb_map pktcmd_cbs[] = {
	{ CMD_GET_HW_MODEL, "CMD_GET_HW_MODEL", CmdGetHwModelInEncode, CmdGetHwModelOutEncode, CmdGetHwModelHdl, CmdGetHwModelACKHdl, NULL, NULL },
	{ CMD_GET_HW_VERSION, "CMD_GET_HW_VERSION", CmdGetHwVersionInEncode, CmdGetHwVersionOutEncode, CmdGetHwVersionHdl, CmdGetHwVersionACKHdl, NULL, NULL },
	{ CMD_GET_SW_VERSION, "CMD_GET_SW_VERSION", CmdGetSwVersionInEncode, CmdGetSwVersionOutEncode, CmdGetSwVersionHdl, CmdGetSwVersionACKHdl, NULL, NULL },
	{ CMD_GET_HW_SERIAL_NUM, "CMD_GET_HW_SERIAL_NUM", CmdGetHwSerialNumInEncode, CmdGetHwSerialNumOutEncode, CmdGetHwSerialNumHdl, CmdGetHwSerialNumACKHdl, NULL, NULL },
	{ CMD_GET_HW_DESCRIPTION, "CMD_GET_HW_DESCRIPTION", CmdGetHwDescriptionInEncode, CmdGetHwDescriptionOutEncode, CmdGetHwDescriptionHdl, CmdGetHwDescriptionACKHdl, NULL, NULL },
	{ CMD_SET_HW_DESCRIPTION, "CMD_SET_HW_DESCRIPTION", CmdSetHwDescriptionInEncode, NULL, CmdSetHwDescriptionHdl, CmdSetHwDescriptionACKHdl, NULL, NULL },
	{ CMD_GET_ETHER_INFO, "CMD_GET_ETHER_INFO", CmdGetEtherInfoInEncode, CmdGetEtherInfoOutEncode, CmdGetEtherInfoHdl, CmdGetEtherInfoACKHdl, NULL, NULL },
	{ CMD_SET_ETHER_INFO, "CMD_SET_ETHER_INFO", CmdSetEtherInfoInEncode, NULL, CmdSetEtherInfoHdl, CmdSetEtherInfoACKHdl, NULL, NULL },
	{ CMD_GET_MAC, "CMD_GET_MAC", CmdGetMacInEncode, CmdGetMacOutEncode, CmdGetMacHdl, CmdGetMacACKHdl, NULL, NULL },
	{ CMD_SET_MAC, "CMD_SET_MAC", CmdSetMacInEncode, NULL, CmdSetMacHdl, CmdSetMacACKHdl, NULL, NULL },
	{ CMD_GET_SERVER_IP, "CMD_GET_SERVER_IP", CmdGetServerIpInEncode, CmdGetServerIpOutEncode, CmdGetServerIpHdl, CmdGetServerIpACKHdl, NULL, NULL },
	{ CMD_SET_SERVER_IP, "CMD_SET_SERVER_IP", CmdSetServerIpInEncode, NULL, CmdSetServerIpHdl, CmdSetServerIpACKHdl, NULL, NULL },
	{ CMD_GET_GATEWAY_IP, "CMD_GET_GATEWAY_IP", CmdGetGatewayIpInEncode, CmdGetGatewayIpOutEncode, CmdGetGatewayIpHdl, CmdGetGatewayIpACKHdl, NULL, NULL },
	{ CMD_SET_GATEWAY_IP, "CMD_SET_GATEWAY_IP", CmdSetGatewayIpInEncode, NULL, CmdSetGatewayIpHdl, CmdSetGatewayIpACKHdl, NULL, NULL },
	{ CMD_GET_SYS_TIME, "CMD_GET_SYS_TIME", CmdGetSysTimeInEncode, CmdGetSysTimeOutEncode, CmdGetSysTimeHdl, CmdGetSysTimeACKHdl, NULL, NULL },
	{ CMD_SET_SYS_TIME, "CMD_SET_SYS_TIME", CmdSetSysTimeInEncode, NULL, CmdSetSysTimeHdl, CmdSetSysTimeACKHdl, NULL, NULL },
	{ CMD_GET_NTP_MODE, "CMD_GET_NTP_MODE", CmdGetNtpModeInEncode, CmdGetNtpModeOutEncode, CmdGetNtpModeHdl, CmdGetNtpModeACKHdl, NULL, NULL },
	{ CMD_SET_NTP_MODE, "CMD_SET_NTP_MODE", CmdSetNtpModeInEncode, NULL, CmdSetNtpModeHdl, CmdSetNtpModeACKHdl, NULL, NULL },
	{ CMD_GET_CLK_TYPE, "CMD_GET_CLK_TYPE", CmdGetClkTypeInEncode, CmdGetClkTypeOutEncode, CmdGetClkTypeHdl, CmdGetClkTypeACKHdl, NULL, NULL },
	{ CMD_SET_CLK_TYPE, "CMD_SET_CLK_TYPE", CmdSetClkTypeInEncode, NULL, CmdSetClkTypeHdl, CmdSetClkTypeACKHdl, NULL, NULL },
	{ CMD_GET_PORTS, "CMD_GET_PORTS", CmdGetPortsInEncode, CmdGetPortsOutEncode, CmdGetPortsHdl, CmdGetPortsACKHdl, NULL, NULL },
	{ CMD_GET_GLOBAL_PORT_CFG, "CMD_GET_GLOBAL_PORT_CFG", CmdGetGlobalPortCfgInEncode, CmdGetGlobalPortCfgOutEncode, CmdGetGlobalPortCfgHdl, CmdGetGlobalPortCfgACKHdl, NULL, NULL },
	{ CMD_SET_GLOBAL_PORT_CFG, "CMD_SET_GLOBAL_PORT_CFG", CmdSetGlobalPortCfgInEncode, NULL, CmdSetGlobalPortCfgHdl, CmdSetGlobalPortCfgACKHdl, NULL, NULL },
	{ CMD_GET_PORT_CFG, "CMD_GET_PORT_CFG", CmdGetPortCfgInEncode, CmdGetPortCfgOutEncode, CmdGetPortCfgHdl, CmdGetPortCfgACKHdl, NULL, NULL },
	{ CMD_SET_PORT_CFG, "CMD_SET_PORT_CFG", CmdSetPortCfgInEncode, NULL, CmdSetPortCfgHdl, CmdSetPortCfgACKHdl, NULL, NULL },
	{ CMD_DEL_PORT_CFG, "CMD_DEL_PORT_CFG", CmdDelPortCfgInEncode, NULL, CmdDelPortCfgHdl, CmdDelPortCfgACKHdl, NULL, NULL },
	{ CMD_DOWNLOAD_PORT_CFGFILE, "CMD_DOWNLOAD_PORT_CFGFILE", CmdDownloadPortCfgFileInEncode, CmdDownloadPortCfgFileOutEncode, CmdDownloadPortCfgFileHdl, CmdDownloadPortCfgFileACKHdl, NULL, NULL },
	{ CMD_UPLOAD_PORT_CFGFILE, "CMD_UPLOAD_PORT_CFGFILE", CmdUploadPortCfgFileInEncode, NULL, CmdUploadPortCfgFileHdl, CmdUploadPortCfgFileACKHdl, NULL, NULL },
	{ CMD_DOWNLOAD_CH_CFGFILE, "CMD_DOWNLOAD_CH_CFGFILE", CmdDownloadChCfgFileInEncode, CmdDownloadChCfgFileOutEncode, CmdDownloadChCfgFileHdl, CmdDownloadChCfgFileACKHdl, NULL, NULL },
	{ CMD_UPLOAD_CH_CFGFILE, "CMD_UPLOAD_CH_CFGFILE", CmdUploadChCfgFileInEncode, NULL, CmdUploadChCfgFileHdl, CmdUploadChCfgFileACKHdl, NULL, NULL },
	{ CMD_GET_DEV_ID, "CMD_GET_DEV_ID", CmdGetDevIdInEncode, CmdGetDevIdOutEncode, CmdGetDevIdHdl, CmdGetDevIdACKHdl, NULL, NULL },
	{ CMD_SET_DEV_ID, "CMD_SET_DEV_ID", CmdSetDevIdInEncode, NULL, CmdSetDevIdHdl, CmdSetDevIdACKHdl, NULL, NULL },
	{ CMD_GET_PACK_TYPE, "CMD_GET_PACK_TYPE", CmdGetPackTypeInEncode, CmdGetPackTypeOutEncode, CmdGetPackTypeHdl, CmdGetPackTypeACKHdl, NULL, NULL },
	{ CMD_SET_PACK_TYPE, "CMD_SET_PACK_TYPE", CmdSetPackTypeInEncode, NULL, CmdSetPackTypeHdl, CmdSetPackTypeACKHdl, NULL, NULL },
	{ CMD_GET_DEV_MODE, "CMD_GET_DEV_MODE", CmdGetDevModeInEncode, CmdGetDevModeOutEncode, CmdGetDevModeHdl, CmdGetDevModeACKHdl, NULL, NULL },
	{ CMD_SET_DEV_MODE, "CMD_SET_DEV_MODE", CmdSetDevModeInEncode, NULL, CmdSetDevModeHdl, CmdSetDevModeACKHdl, NULL, NULL },
	{ CMD_GET_PORT_SDH_MATRIX_CFG, "CMD_GET_PORT_SDH_MATRIX_CFG", CmdGetPortSdhMatrixCfgInEncode, CmdGetPortSdhMatrixCfgOutEncode, CmdGetPortSdhMatrixCfgHdl, CmdGetPortSdhMatrixCfgACKHdl, NULL, NULL },
	{ CMD_SET_PORT_SDH_MATRIX_CFG, "CMD_SET_PORT_SDH_MATRIX_CFG", CmdSetPortSdhMatrixCfgInEncode, NULL, CmdSetPortSdhMatrixCfgHdl, CmdSetPortSdhMatrixCfgACKHdl, NULL, NULL },
	{ CMD_GET_PORT_MATRIX_CFG, "CMD_GET_PORT_MATRIX_CFG", CmdGetPortMatrixCfgInEncode, CmdGetPortMatrixCfgOutEncode, CmdGetPortMatrixCfgHdl, CmdGetPortMatrixCfgACKHdl, NULL, NULL },
	{ CMD_SET_PORT_MATRIX_CFG, "CMD_SET_PORT_MATRIX_CFG", CmdSetPortMatrixCfgInEncode, NULL, CmdSetPortMatrixCfgHdl, CmdSetPortMatrixCfgACKHdl, NULL, NULL },
	{ CMD_DOWNLOAD_MATRIX_CFGFILE, "CMD_DOWNLOAD_MATRIX_CFGFILE", CmdDownloadMatrixCfgFileInEncode, CmdDownloadMatrixCfgFileOutEncode, CmdDownloadMatrixCfgFileHdl, CmdDownloadMatrixCfgFileACKHdl, NULL, NULL },
	{ CMD_UPLOAD_MATRIX_CFGFILE, "CMD_UPLOAD_MATRIX_CFGFILE", CmdUploadMatrixCfgFileInEncode, NULL, CmdUploadMatrixCfgFileHdl, CmdUploadMatrixCfgFileACKHdl, NULL, NULL },
	{ CMD_GAIN_SCAN, "CMD_GAIN_SCAN", CmdGainScanInEncode, CmdGainScanOutEncode, CmdGainScanHdl, CmdGainScanACKHdl, NULL, NULL },
	{ CMD_CHAN_SCAN, "CMD_CHAN_SCAN", CmdChanScanInEncode, CmdChanScanOutEncode, CmdChanScanHdl, CmdChanScanACKHdl, NULL, NULL },
	{ CMD_GET_GLOBAL_BERT_CFG, "CMD_GET_GLOBAL_BERT_CFG", CmdGetGlobalBertCfgInEncode, CmdGetGlobalBertCfgOutEncode, CmdGetGlobalBertCfgHdl, CmdGetGlobalBertCfgACKHdl, NULL, NULL },
	{ CMD_SET_GLOBAL_BERT_CFG, "CMD_SET_GLOBAL_BERT_CFG", CmdSetGlobalBertCfgInEncode, NULL, CmdSetGlobalBertCfgHdl, CmdSetGlobalBertCfgACKHdl, NULL, NULL },
	{ CMD_GET_BERT_CFG, "CMD_GET_BERT_CFG", CmdGetBertCfgInEncode, CmdGetBertCfgOutEncode, CmdGetBertCfgHdl, CmdGetBertCfgACKHdl, NULL, NULL },
	{ CMD_SET_BERT_CFG, "CMD_SET_BERT_CFG", CmdSetBertCfgInEncode, NULL, CmdSetBertCfgHdl, CmdSetBertCfgACKHdl, NULL, NULL },
	{ CMD_DEL_BERT_CFG, "CMD_DEL_BERT_CFG", CmdDelBertCfgInEncode, NULL, CmdDelBertCfgHdl, CmdDelBertCfgACKHdl, NULL, NULL },
	{ CMD_START_PRBS, "CMD_START_PRBS", CmdStartPrbsInEncode, NULL, CmdStartPrbsHdl, CmdStartPrbsACKHdl, NULL, NULL },
	{ CMD_STOP_PRBS, "CMD_STOP_PRBS", CmdStopPrbsInEncode, NULL, CmdStopPrbsHdl, CmdStopPrbsACKHdl, NULL, NULL },
	{ CMD_INSERT_ERROR, "CMD_INSERT_ERROR", CmdInsertErrorInEncode, NULL, CmdInsertErrorHdl, CmdInsertErrorACKHdl, NULL, NULL },
	{ CMD_SYS_REBOOT, "CMD_SYS_REBOOT", CmdSysRebootInEncode, NULL, CmdSysRebootHdl, CmdSysRebootACKHdl, NULL, NULL },
	{ CMD_SYS_START, "CMD_SYS_START", CmdSysStartInEncode, NULL, CmdSysStartHdl, CmdSysStartACKHdl, NULL, NULL },
	{ CMD_SYS_STOP, "CMD_SYS_STOP", CmdSysStopInEncode, NULL, CmdSysStopHdl, CmdSysStopACKHdl, NULL, NULL },
	{ CMD_SYS_FIRMWARE_UPGRADE, "CMD_SYS_FIRMWARE_UPGRADE", CmdSysFirmwareUpgradeInEncode, NULL, CmdSysFirmwareUpgradeHdl, CmdSysFirmwareUpgradeACKHdl, NULL, NULL },
	{ CMD_GET_SYS_RES_REP, "CMD_GET_SYS_RES_REP", CmdGetSysResRepInEncode, CmdGetSysResRepOutEncode, CmdGetSysResRepHdl, CmdGetSysResRepACKHdl, NULL, NULL },
	{ CMD_GET_LINE_STAT_INFO, "CMD_GET_LINE_STAT_INFO", CmdGetLineStatInfoInEncode, CmdGetLineStatInfoOutEncode, CmdGetLineStatInfoHdl, CmdGetLineStatInfoACKHdl, NULL, NULL },
	{ CMD_CLR_LINE_STAT_INFO, "CMD_CLR_LINE_STAT_INFO", CmdClrLineStatInfoInEncode, NULL, CmdClrLineStatInfoHdl, CmdClrLineStatInfoACKHdl, NULL, NULL },
	{ CMD_GET_PORT_STAT_INFO, "CMD_GET_PORT_STAT_INFO", CmdGetPortStatInfoInEncode, CmdGetPortStatInfoOutEncode, CmdGetPortStatInfoHdl, CmdGetPortStatInfoACKHdl, NULL, NULL },
	{ CMD_CLR_PORT_STAT_INFO, "CMD_CLR_PORT_STAT_INFO", CmdClrPortStatInfoInEncode, NULL, CmdClrPortStatInfoHdl, CmdClrPortStatInfoACKHdl, NULL, NULL },
	{ CMD_START_PFM_TEST, "CMD_START_PFM_TEST", CmdStartPfmTestInEncode, NULL, CmdStartPfmTestHdl, CmdStartPfmTestACKHdl, NULL, NULL },
	{ CMD_GET_PORT_PFM_RESULT, "CMD_GET_PORT_PFM_RESULT", CmdGetPortPfmResultInEncode, CmdGetPortPfmResultOutEncode, CmdGetPortPfmResultHdl, CmdGetPortPfmResultACKHdl, NULL, NULL },
	{ CMD_STOP_PFM_TEST, "CMD_STOP_PFM_TEST", CmdStopPfmTestInEncode, NULL, CmdStopPfmTestHdl, CmdStopPfmTestACKHdl, NULL, NULL },
	{ CMD_GET_CH_CFG, "CMD_GET_CH_CFG", CmdGetChCfgInEncode, CmdGetChCfgOutEncode, CmdGetChCfgHdl, CmdGetChCfgACKHdl, NULL, NULL },
	{ CMD_GET_CH_STAT_INFO, "CMD_GET_CH_STAT_INFO", CmdGetChStatInfoInEncode, CmdGetChStatInfoOutEncode, CmdGetChStatInfoHdl, CmdGetChStatInfoACKHdl, NULL, NULL },
	{ CMD_CLR_CH_STAT_INFO, "CMD_CLR_CH_STAT_INFO", CmdClrChStatInfoInEncode, NULL, CmdClrChStatInfoHdl, CmdClrChStatInfoACKHdl, NULL, NULL },
	{ CMD_GET_RAW_DATA, "CMD_GET_RAW_DATA", CmdGetRawDataInEncode, CmdGetRawDataOutEncode, CmdGetRawDataHdl, CmdGetRawDataACKHdl, NULL, NULL },
	{ CMD_GET_RT_VOICE, "CMD_GET_RT_VOICE", CmdGetRtVoiceInEncode, CmdGetRtVoiceOutEncode, CmdGetRtVoiceHdl, CmdGetRtVoiceACKHdl, NULL, NULL },
	{ CMD_GET_CALL_TRACE, "CMD_GET_CALL_TRACE", CmdGetCallTraceInEncode, CmdGetCallTraceOutEncode, CmdGetCallTraceHdl, CmdGetCallTraceACKHdl, NULL, NULL },
	{ 0, "", NULL, NULL, NULL, NULL, NULL, NULL }
};
