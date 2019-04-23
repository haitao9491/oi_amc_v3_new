#include <stdio.h>
#include <stdlib.h>
#include "os.h"
#include "aplog.h"
#include "coding.h"
#include "pktcmdarg.h"

#ifdef WIN32
#pragma warning( disable : 4267 ; disable : 4244)
#endif

int CmdGetHwModelInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwModelIn *io = (CmdGetHwModelIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetHwModelInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwModelIn *io = (CmdGetHwModelIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetHwModelOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwModelOut *io = (CmdGetHwModelOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceModel);

	return (buf - start);
}

int CmdGetHwModelOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwModelOut *io = (CmdGetHwModelOut *)p;

	DECODE_8(buf, io->deviceModel, len);

	return 0;
}

int CmdGetHwVersionInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwVersionIn *io = (CmdGetHwVersionIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetHwVersionInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwVersionIn *io = (CmdGetHwVersionIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetHwVersionOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwVersionOut *io = (CmdGetHwVersionOut *)p;
	unsigned char *start = buf;

	CODING_STRING(buf, io->deviceHWVersion);

	return (buf - start);
}

int CmdGetHwVersionOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwVersionOut *io = (CmdGetHwVersionOut *)p;

	DECODE_STRING(buf, io->deviceHWVersion, 8, len);

	return 0;
}

int CmdGetSwVersionInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetSwVersionIn *io = (CmdGetSwVersionIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetSwVersionInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetSwVersionIn *io = (CmdGetSwVersionIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetSwVersionOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetSwVersionOut *io = (CmdGetSwVersionOut *)p;
	unsigned char *start = buf;

	CODING_STRING(buf, io->deviceSWVersion);

	return (buf - start);
}

int CmdGetSwVersionOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetSwVersionOut *io = (CmdGetSwVersionOut *)p;

	DECODE_STRING(buf, io->deviceSWVersion, 64, len);

	return 0;
}

int CmdGetHwSerialNumInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwSerialNumIn *io = (CmdGetHwSerialNumIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetHwSerialNumInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwSerialNumIn *io = (CmdGetHwSerialNumIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetHwSerialNumOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwSerialNumOut *io = (CmdGetHwSerialNumOut *)p;
	unsigned char *start = buf;

	CODING_STRING(buf, io->deviceSerialNumber);

	return (buf - start);
}

int CmdGetHwSerialNumOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwSerialNumOut *io = (CmdGetHwSerialNumOut *)p;

	DECODE_STRING(buf, io->deviceSerialNumber, 24, len);
	return 0;
}

int CmdGetHwDescriptionInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwDescriptionIn *io = (CmdGetHwDescriptionIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetHwDescriptionInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwDescriptionIn *io = (CmdGetHwDescriptionIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetHwDescriptionOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetHwDescriptionOut *io = (CmdGetHwDescriptionOut *)p;
	unsigned char *start = buf;

	CODING_STRING(buf, io->deviceDescription);

	return (buf - start);
}

int CmdGetHwDescriptionOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetHwDescriptionOut *io = (CmdGetHwDescriptionOut *)p;

	DECODE_STRING(buf, io->deviceDescription, 128, len);

	return 0;
}

int CmdSetHwDescriptionInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetHwDescriptionIn *io = (CmdSetHwDescriptionIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_STRING(buf, io->deviceDescription);

	return (buf - start);
}

int CmdSetHwDescriptionInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetHwDescriptionIn *io = (CmdSetHwDescriptionIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_STRING(buf, io->deviceDescription, 128, len);

	return 0;
}

int CmdGetEtherInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetEtherInfoIn *io = (CmdGetEtherInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->etherItfNumber);

	return (buf - start);
}

int CmdGetEtherInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetEtherInfoIn *io = (CmdGetEtherInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->etherItfNumber, len);

	return 0;
}

int CmdGetEtherInfoOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetEtherInfoOut *io = (CmdGetEtherInfoOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->etherItfNumber);
	CODING_32(buf, io->etherItfIP);
	CODING_32(buf, io->etherItfNetmask);

	return (buf - start);
}

int CmdGetEtherInfoOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetEtherInfoOut *io = (CmdGetEtherInfoOut *)p;

	DECODE_8(buf, io->etherItfNumber, len);
	DECODE_32(buf, io->etherItfIP, len);
	DECODE_32(buf, io->etherItfNetmask, len);

	return 0;
}

int CmdSetEtherInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetEtherInfoIn *io = (CmdSetEtherInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->etherItfNumber);
	CODING_32(buf, io->etherItfIP);
	CODING_32(buf, io->etherItfNetmask);

	return (buf - start);
}

int CmdSetEtherInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetEtherInfoIn *io = (CmdSetEtherInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->etherItfNumber, len);
	DECODE_32(buf, io->etherItfIP, len);
	DECODE_32(buf, io->etherItfNetmask, len);

	return 0;
}


int CmdSetServerIpInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetServerIpIn *io = (CmdSetServerIpIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->serverIPAddress);

	return (buf - start);
}

int CmdSetServerIpInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetServerIpIn *io = (CmdSetServerIpIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->serverIPAddress, len);

	return 0;
}

int CmdGetMacInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetMacIn *io = (CmdGetMacIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->etherItfNumber);

	return (buf - start);
}

int CmdGetMacInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetMacIn *io = (CmdGetMacIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->etherItfNumber, len);

	return 0;
}

int CmdGetMacOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetMacOut *io = (CmdGetMacOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->etherItfNumber);
	*buf++ = io->etherItfMacAddr[0];
	*buf++ = io->etherItfMacAddr[1];
	*buf++ = io->etherItfMacAddr[2];
	*buf++ = io->etherItfMacAddr[3];
	*buf++ = io->etherItfMacAddr[4];
	*buf++ = io->etherItfMacAddr[5];

	return (buf - start);
}

int CmdGetMacOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetMacOut *io = (CmdGetMacOut *)p;

	DECODE_8(buf, io->etherItfNumber, len);
	io->etherItfMacAddr[0] = *buf++;
	io->etherItfMacAddr[1] = *buf++;
	io->etherItfMacAddr[2] = *buf++;
	io->etherItfMacAddr[3] = *buf++;
	io->etherItfMacAddr[4] = *buf++;
	io->etherItfMacAddr[5] = *buf++;

	return 0;
}


int CmdSetMacInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetMacIn *io = (CmdSetMacIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->etherItfNumber);
	*buf++ = io->etherItfMacAddr[0];
	*buf++ = io->etherItfMacAddr[1];
	*buf++ = io->etherItfMacAddr[2];
	*buf++ = io->etherItfMacAddr[3];
	*buf++ = io->etherItfMacAddr[4];
	*buf++ = io->etherItfMacAddr[5];

	return (buf - start);
}

int CmdSetMacInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetMacIn *io = (CmdSetMacIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->etherItfNumber, len);
	io->etherItfMacAddr[0] = *buf++;
	io->etherItfMacAddr[1] = *buf++;
	io->etherItfMacAddr[2] = *buf++;
	io->etherItfMacAddr[3] = *buf++;
	io->etherItfMacAddr[4] = *buf++;
	io->etherItfMacAddr[5] = *buf++;
	return 0;
}

int CmdGetServerIpInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetServerIpIn *io = (CmdGetServerIpIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetServerIpInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetServerIpIn *io = (CmdGetServerIpIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetServerIpOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetServerIpOut *io = (CmdGetServerIpOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->serverIPAddress);

	return (buf - start);
}

int CmdGetServerIpOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetServerIpOut *io = (CmdGetServerIpOut *)p;

	DECODE_32(buf, io->serverIPAddress, len);

	return 0;
}

int CmdGetGatewayIpInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetGatewayIpIn *io = (CmdGetGatewayIpIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetGatewayIpInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetGatewayIpIn *io = (CmdGetGatewayIpIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetGatewayIpOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetGatewayIpOut *io = (CmdGetGatewayIpOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->gatewayIPAddress);

	return (buf - start);
}

int CmdGetGatewayIpOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetGatewayIpOut *io = (CmdGetGatewayIpOut *)p;

	DECODE_32(buf, io->gatewayIPAddress, len);

	return 0;
}

int CmdSetGatewayIpInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetGatewayIpIn *io = (CmdSetGatewayIpIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->gatewayIPAddress);

	return (buf - start);
}

int CmdSetGatewayIpInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetGatewayIpIn *io = (CmdSetGatewayIpIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->gatewayIPAddress, len);

	return 0;
}

int CmdGetSysTimeInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetSysTimeIn *io = (CmdGetSysTimeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetSysTimeInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetSysTimeIn *io = (CmdGetSysTimeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetSysTimeOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetSysTimeOut *io = (CmdGetSysTimeOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->sysTime.sec);
	CODING_32(buf, io->sysTime.usec);

	return (buf - start);
}

int CmdGetSysTimeOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetSysTimeOut *io = (CmdGetSysTimeOut *)p;

	DECODE_32(buf, io->sysTime.sec, len);
	DECODE_32(buf, io->sysTime.usec, len);

	return 0;
}

int CmdSetSysTimeInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetSysTimeIn *io = (CmdSetSysTimeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->sysTime.sec);
	CODING_32(buf, io->sysTime.usec);
	return (buf - start);
}

int CmdSetSysTimeInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetSysTimeIn *io = (CmdSetSysTimeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->sysTime.sec, len);
	DECODE_32(buf, io->sysTime.usec, len);

	return 0;
}

int CmdGetNtpModeInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetNtpModeIn *io = (CmdGetNtpModeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetNtpModeInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetNtpModeIn *io = (CmdGetNtpModeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetNtpModeOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetNtpModeOut *io = (CmdGetNtpModeOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->ntpMode);
	CODING_32(buf, io->ntpServerIPAddress);

	return (buf - start);
}

int CmdGetNtpModeOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetNtpModeOut *io = (CmdGetNtpModeOut *)p;

	DECODE_8(buf, io->ntpMode, len);
	DECODE_32(buf, io->ntpServerIPAddress, len);

	return 0;
}

int CmdSetNtpModeInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetNtpModeIn *io = (CmdSetNtpModeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->ntpMode);
	CODING_32(buf, io->ntpServerIPAddress);

	return (buf - start);
}

int CmdSetNtpModeInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetNtpModeIn *io = (CmdSetNtpModeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->ntpMode, len);
	DECODE_32(buf, io->ntpServerIPAddress, len);

	return 0;
}

int CmdGetClkTypeInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetClkTypeIn *io = (CmdGetClkTypeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetClkTypeInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetClkTypeIn *io = (CmdGetClkTypeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetClkTypeOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetClkTypeOut *io = (CmdGetClkTypeOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->clkType);
	CODING_16(buf, io->clkSrc);

	return (buf - start);
}

int CmdGetClkTypeOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetClkTypeOut *io = (CmdGetClkTypeOut *)p;

	DECODE_8(buf, io->clkType, len);
	DECODE_16(buf, io->clkSrc, len);

	return 0;
}

int CmdSetClkTypeInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetClkTypeIn *io = (CmdSetClkTypeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->clkType);
	CODING_16(buf, io->clkSrc);

	return (buf - start);
}

int CmdSetClkTypeInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetClkTypeIn *io = (CmdSetClkTypeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->clkType, len);
	DECODE_16(buf, io->clkSrc, len);

	return 0;
}

int CmdGetPortsInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortsIn *io = (CmdGetPortsIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetPortsInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortsIn *io = (CmdGetPortsIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetPortsOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortsOut *io = (CmdGetPortsOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->inputPorts);
	CODING_32(buf, io->outputPorts);

	return (buf - start);
}

int CmdGetPortsOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortsOut *io = (CmdGetPortsOut *)p;

	DECODE_32(buf, io->inputPorts, len);
	DECODE_32(buf, io->outputPorts, len);

	return 0;
}

int CmdGetGlobalPortCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetGlobalPortCfgIn *io = (CmdGetGlobalPortCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetGlobalPortCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetGlobalPortCfgIn *io = (CmdGetGlobalPortCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetGlobalPortCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetGlobalPortCfgOut *io = (CmdGetGlobalPortCfgOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->phyItfType);
	CODING_8(buf, io->lineCode);
	CODING_8(buf, io->gain);
	CODING_8(buf, io->inputFrameFormat);
	CODING_8(buf, io->inputImpedance);
	CODING_8(buf, io->outputFrameFormat);
	CODING_8(buf, io->outputImpedance);

	return (buf - start);
}

int CmdGetGlobalPortCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetGlobalPortCfgOut *io = (CmdGetGlobalPortCfgOut *)p;

	DECODE_8(buf, io->phyItfType, len);
	DECODE_8(buf, io->lineCode, len);
	DECODE_8(buf, io->gain, len);
	DECODE_8(buf, io->inputFrameFormat, len);
	DECODE_8(buf, io->inputImpedance, len);
	DECODE_8(buf, io->outputFrameFormat, len);
	DECODE_8(buf, io->outputImpedance, len);

	return 0;
}

int CmdSetGlobalPortCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetGlobalPortCfgIn *io = (CmdSetGlobalPortCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->phyItfType);
	CODING_8(buf, io->lineCode);
	CODING_8(buf, io->gain);
	CODING_8(buf, io->inputFrameFormat);
	CODING_8(buf, io->inputImpedance);
	CODING_8(buf, io->outputFrameFormat);
	CODING_8(buf, io->outputImpedance);

	return (buf - start);
}

int CmdSetGlobalPortCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetGlobalPortCfgIn *io = (CmdSetGlobalPortCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->phyItfType, len);
	DECODE_8(buf, io->lineCode, len);
	DECODE_8(buf, io->gain, len);
	DECODE_8(buf, io->inputFrameFormat, len);
	DECODE_8(buf, io->inputImpedance, len);
	DECODE_8(buf, io->outputFrameFormat, len);
	DECODE_8(buf, io->outputImpedance, len);

	return 0;
}

int CmdGetPortCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortCfgIn *io = (CmdGetPortCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);

	return (buf - start);
}

int CmdGetPortCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortCfgIn *io = (CmdGetPortCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);

	return 0;
}

int CmdGetPortCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortCfgOut *io = (CmdGetPortCfgOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->portID);
	CODING_8(buf, io->phyItfType);
	CODING_8(buf, io->lineCode);
	CODING_8(buf, io->gain);
	CODING_8(buf, io->inputFrameFormat);
	CODING_8(buf, io->inputImpedance);
	CODING_8(buf, io->outputFrameFormat);
	CODING_8(buf, io->outputImpedance);

	return (buf - start);
}

int CmdGetPortCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortCfgOut *io = (CmdGetPortCfgOut *)p;

	DECODE_32(buf, io->portID, len);
	DECODE_8(buf, io->phyItfType, len);
	DECODE_8(buf, io->lineCode, len);
	DECODE_8(buf, io->gain, len);
	DECODE_8(buf, io->inputFrameFormat, len);
	DECODE_8(buf, io->inputImpedance, len);
	DECODE_8(buf, io->outputFrameFormat, len);
	DECODE_8(buf, io->outputImpedance, len);

	return 0;
}

int CmdSetPortCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetPortCfgIn *io = (CmdSetPortCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);
	CODING_8(buf, io->phyItfType);
	CODING_8(buf, io->lineCode);
	CODING_8(buf, io->gain);
	CODING_8(buf, io->inputFrameFormat);
	CODING_8(buf, io->inputImpedance);
	CODING_8(buf, io->outputFrameFormat);
	CODING_8(buf, io->outputImpedance);

	return (buf - start);
}

int CmdSetPortCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetPortCfgIn *io = (CmdSetPortCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);
	DECODE_8(buf, io->phyItfType, len);
	DECODE_8(buf, io->lineCode, len);
	DECODE_8(buf, io->gain, len);
	DECODE_8(buf, io->inputFrameFormat, len);
	DECODE_8(buf, io->inputImpedance, len);
	DECODE_8(buf, io->outputFrameFormat, len);
	DECODE_8(buf, io->outputImpedance, len);

	return 0;
}

int CmdDelPortCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdDelPortCfgIn *io = (CmdDelPortCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);

	return (buf - start);
}

int CmdDelPortCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdDelPortCfgIn *io = (CmdDelPortCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);

	return 0;
}

int CmdDownloadPortCfgFileInEncode(unsigned char *buf, int size, void *p)
{
	CmdDownloadPortCfgFileIn *io = (CmdDownloadPortCfgFileIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdDownloadPortCfgFileInDecode(unsigned char *buf, int len, void *p)
{
	CmdDownloadPortCfgFileIn *io = (CmdDownloadPortCfgFileIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdDownloadPortCfgFileOutEncode(unsigned char *buf, int size, void *p)
{
	CmdDownloadPortCfgFileOut *io = (CmdDownloadPortCfgFileOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdDownloadPortCfgFileOutDecode(unsigned char *buf, int len, void *p)
{
	CmdDownloadPortCfgFileOut *io = (CmdDownloadPortCfgFileOut *)p;

	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdUploadPortCfgFileInEncode(unsigned char *buf, int size, void *p)
{
	CmdUploadPortCfgFileIn *io = (CmdUploadPortCfgFileIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdUploadPortCfgFileInDecode(unsigned char *buf, int len, void *p)
{
	CmdUploadPortCfgFileIn *io = (CmdUploadPortCfgFileIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdDownloadChCfgFileInEncode(unsigned char *buf, int size, void *p)
{
	CmdDownloadChCfgFileIn *io = (CmdDownloadChCfgFileIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdDownloadChCfgFileInDecode(unsigned char *buf, int len, void *p)
{
	CmdDownloadChCfgFileIn *io = (CmdDownloadChCfgFileIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdDownloadChCfgFileOutEncode(unsigned char *buf, int size, void *p)
{
	CmdDownloadChCfgFileOut *io = (CmdDownloadChCfgFileOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdDownloadChCfgFileOutDecode(unsigned char *buf, int len, void *p)
{
	CmdDownloadChCfgFileOut *io = (CmdDownloadChCfgFileOut *)p;

	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdUploadChCfgFileInEncode(unsigned char *buf, int size, void *p)
{
	CmdUploadChCfgFileIn *io = (CmdUploadChCfgFileIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdUploadChCfgFileInDecode(unsigned char *buf, int len, void *p)
{
	CmdUploadChCfgFileIn *io = (CmdUploadChCfgFileIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGetDevIdInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetDevIdIn *io = (CmdGetDevIdIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetDevIdInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetDevIdIn *io = (CmdGetDevIdIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetDevIdOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetDevIdOut *io = (CmdGetDevIdOut *)p;
	unsigned char *start = buf;

	CODING_16(buf, io->deviceID);

	return (buf - start);
}

int CmdGetDevIdOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetDevIdOut *io = (CmdGetDevIdOut *)p;

	DECODE_16(buf, io->deviceID, len);

	return 0;
}

int CmdSetDevIdInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetDevIdIn *io = (CmdSetDevIdIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_16(buf, io->deviceID);

	return (buf - start);
}

int CmdSetDevIdInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetDevIdIn *io = (CmdSetDevIdIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_16(buf, io->deviceID, len);

	return 0;
}

int CmdGetPackTypeInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPackTypeIn *io = (CmdGetPackTypeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetPackTypeInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPackTypeIn *io = (CmdGetPackTypeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetPackTypeOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPackTypeOut *io = (CmdGetPackTypeOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->packetSubType);

	return (buf - start);
}

int CmdGetPackTypeOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPackTypeOut *io = (CmdGetPackTypeOut *)p;

	DECODE_8(buf, io->packetSubType, len);

	return 0;
}

int CmdSetPackTypeInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetPackTypeIn *io = (CmdSetPackTypeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->packetSubType);

	return (buf - start);
}

int CmdSetPackTypeInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetPackTypeIn *io = (CmdSetPackTypeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->packetSubType, len);

	return 0;
}

int CmdGetDevModeInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetDevModeIn *io = (CmdGetDevModeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetDevModeInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetDevModeIn *io = (CmdGetDevModeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetDevModeOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetDevModeOut *io = (CmdGetDevModeOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceMode);

	return (buf - start);
}

int CmdGetDevModeOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetDevModeOut *io = (CmdGetDevModeOut *)p;

	DECODE_8(buf, io->deviceMode, len);

	return 0;
}

int CmdSetDevModeInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetDevModeIn *io = (CmdSetDevModeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->deviceMode);

	return (buf - start);
}

int CmdSetDevModeInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetDevModeIn *io = (CmdSetDevModeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->deviceMode, len);

	return 0;
}

int CmdGetPortSdhMatrixCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortSdhMatrixCfgIn *io = (CmdGetPortSdhMatrixCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->dstPortID);
	CODING_8(buf, io->dstTsID);

	return (buf - start);
}

int CmdGetPortSdhMatrixCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortSdhMatrixCfgIn *io = (CmdGetPortSdhMatrixCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->dstPortID, len);
	DECODE_8(buf, io->dstTsID, len);

	return 0;
}

int CmdGetPortSdhMatrixCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortSdhMatrixCfgOut *io = (CmdGetPortSdhMatrixCfgOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->dstPortID);
	CODING_8(buf, io->dstTsID);
	CODING_8(buf, io->srcStmID);
	CODING_8(buf, io->srcTribID);
	CODING_8(buf, io->srcTsID);

	return (buf - start);
}

int CmdGetPortSdhMatrixCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortSdhMatrixCfgOut *io = (CmdGetPortSdhMatrixCfgOut *)p;

	DECODE_32(buf, io->dstPortID, len);
	DECODE_8(buf, io->dstTsID, len);
	DECODE_8(buf, io->srcStmID, len);
	DECODE_8(buf, io->srcTribID, len);
	DECODE_8(buf, io->srcTsID, len);

	return 0;
}

int CmdSetPortSdhMatrixCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetPortSdhMatrixCfgIn *io = (CmdSetPortSdhMatrixCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->dstPortID);
	CODING_8(buf, io->dstTsID);
	CODING_8(buf, io->srcStmID);
	CODING_8(buf, io->srcTribID);
	CODING_8(buf, io->srcTsID);

	return (buf - start);
}

int CmdSetPortSdhMatrixCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetPortSdhMatrixCfgIn *io = (CmdSetPortSdhMatrixCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->dstPortID, len);
	DECODE_8(buf, io->dstTsID, len);
	DECODE_8(buf, io->srcStmID, len);
	DECODE_8(buf, io->srcTribID, len);
	DECODE_8(buf, io->srcTsID, len);

	return 0;
}

int CmdGetPortMatrixCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortMatrixCfgIn *io = (CmdGetPortMatrixCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->dstPortID);
	CODING_8(buf, io->dstTsID);

	return (buf - start);
}

int CmdGetPortMatrixCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortMatrixCfgIn *io = (CmdGetPortMatrixCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->dstPortID, len);
	DECODE_8(buf, io->dstTsID, len);

	return 0;
}

int CmdGetPortMatrixCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortMatrixCfgOut *io = (CmdGetPortMatrixCfgOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->dstPortID);
	CODING_8(buf, io->dstTsID);
	CODING_32(buf, io->srcPortID);
	CODING_8(buf, io->srcTsID);

	return (buf - start);
}

int CmdGetPortMatrixCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortMatrixCfgOut *io = (CmdGetPortMatrixCfgOut *)p;

	DECODE_32(buf, io->dstPortID, len);
	DECODE_8(buf, io->dstTsID, len);
	DECODE_32(buf, io->srcPortID, len);
	DECODE_8(buf, io->srcTsID, len);

	return 0;
}

int CmdSetPortMatrixCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetPortMatrixCfgIn *io = (CmdSetPortMatrixCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->dstPortID);
	CODING_8(buf, io->dstTsID);
	CODING_32(buf, io->srcPortID);
	CODING_8(buf, io->srcTsID);

	return (buf - start);
}

int CmdSetPortMatrixCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetPortMatrixCfgIn *io = (CmdSetPortMatrixCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->dstPortID, len);
	DECODE_8(buf, io->dstTsID, len);
	DECODE_32(buf, io->srcPortID, len);
	DECODE_8(buf, io->srcTsID, len);

	return 0;
}

int CmdDownloadMatrixCfgFileInEncode(unsigned char *buf, int size, void *p)
{
	CmdDownloadMatrixCfgFileIn *io = (CmdDownloadMatrixCfgFileIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdDownloadMatrixCfgFileInDecode(unsigned char *buf, int len, void *p)
{
	CmdDownloadMatrixCfgFileIn *io = (CmdDownloadMatrixCfgFileIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdDownloadMatrixCfgFileOutEncode(unsigned char *buf, int size, void *p)
{
	CmdDownloadMatrixCfgFileOut *io = (CmdDownloadMatrixCfgFileOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdDownloadMatrixCfgFileOutDecode(unsigned char *buf, int len, void *p)
{
	CmdDownloadMatrixCfgFileOut *io = (CmdDownloadMatrixCfgFileOut *)p;

	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdUploadMatrixCfgFileInEncode(unsigned char *buf, int size, void *p)
{
	CmdUploadMatrixCfgFileIn *io = (CmdUploadMatrixCfgFileIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdUploadMatrixCfgFileInDecode(unsigned char *buf, int len, void *p)
{
	CmdUploadMatrixCfgFileIn *io = (CmdUploadMatrixCfgFileIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGainScanInEncode(unsigned char *buf, int size, void *p)
{
	CmdGainScanIn *io = (CmdGainScanIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->scanInterval);

	return (buf - start);
}

int CmdGainScanInDecode(unsigned char *buf, int len, void *p)
{
	CmdGainScanIn *io = (CmdGainScanIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->scanInterval, len);

	return 0;
}

int CmdGainScanOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGainScanOut *io = (CmdGainScanOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdGainScanOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGainScanOut *io = (CmdGainScanOut *)p;

	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdChanScanInEncode(unsigned char *buf, int size, void *p)
{
	CmdChanScanIn *io = (CmdChanScanIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->scanInterval);
	CODING_8(buf, io->scanType);
	CODING_32(buf, io->scanNList);

	return (buf - start);
}

int CmdChanScanInDecode(unsigned char *buf, int len, void *p)
{
	CmdChanScanIn *io = (CmdChanScanIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->scanInterval, len);
	DECODE_8(buf, io->scanType, len);
	DECODE_32(buf, io->scanNList, len);

	return 0;
}

int CmdChanScanOutEncode(unsigned char *buf, int size, void *p)
{
	CmdChanScanOut *io = (CmdChanScanOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdChanScanOutDecode(unsigned char *buf, int len, void *p)
{
	CmdChanScanOut *io = (CmdChanScanOut *)p;

	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGetGlobalBertCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetGlobalBertCfgIn *io = (CmdGetGlobalBertCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetGlobalBertCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetGlobalBertCfgIn *io = (CmdGetGlobalBertCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetGlobalBertCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetGlobalBertCfgOut *io = (CmdGetGlobalBertCfgOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->bertStatus);
	CODING_8(buf, io->bertDir);
	CODING_8(buf, io->bertTSList);
	CODING_8(buf, io->bertPattern);
	CODING_8(buf, io->bertInverted);
	CODING_8(buf, io->bertErrorRate);

	return (buf - start);
}

int CmdGetGlobalBertCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetGlobalBertCfgOut *io = (CmdGetGlobalBertCfgOut *)p;

	DECODE_8(buf, io->bertStatus, len);
	DECODE_8(buf, io->bertDir, len);
	DECODE_8(buf, io->bertTSList, len);
	DECODE_8(buf, io->bertPattern, len);
	DECODE_8(buf, io->bertInverted, len);
	DECODE_8(buf, io->bertErrorRate, len);

	return 0;
}

int CmdSetGlobalBertCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetGlobalBertCfgIn *io = (CmdSetGlobalBertCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->bertStatus);
	CODING_8(buf, io->bertDir);
	CODING_8(buf, io->bertTSList);
	CODING_8(buf, io->bertPattern);
	CODING_8(buf, io->bertInverted);
	CODING_8(buf, io->bertErrorRate);

	return (buf - start);
}

int CmdSetGlobalBertCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetGlobalBertCfgIn *io = (CmdSetGlobalBertCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->bertStatus, len);
	DECODE_8(buf, io->bertDir, len);
	DECODE_8(buf, io->bertTSList, len);
	DECODE_8(buf, io->bertPattern, len);
	DECODE_8(buf, io->bertInverted, len);
	DECODE_8(buf, io->bertErrorRate, len);

	return 0;
}

int CmdGetBertCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetBertCfgIn *io = (CmdGetBertCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);

	return (buf - start);
}

int CmdGetBertCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetBertCfgIn *io = (CmdGetBertCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);

	return 0;
}

int CmdGetBertCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetBertCfgOut *io = (CmdGetBertCfgOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->portID);
	CODING_8(buf, io->bertStatus);
	CODING_8(buf, io->bertDir);
	CODING_8(buf, io->bertTSList);
	CODING_8(buf, io->bertPattern);
	CODING_8(buf, io->bertInverted);
	CODING_8(buf, io->bertErrorRate);

	return (buf - start);
}

int CmdGetBertCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetBertCfgOut *io = (CmdGetBertCfgOut *)p;

	DECODE_32(buf, io->portID, len);
	DECODE_8(buf, io->bertStatus, len);
	DECODE_8(buf, io->bertDir, len);
	DECODE_8(buf, io->bertTSList, len);
	DECODE_8(buf, io->bertPattern, len);
	DECODE_8(buf, io->bertInverted, len);
	DECODE_8(buf, io->bertErrorRate, len);

	return 0;
}

int CmdSetBertCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdSetBertCfgIn *io = (CmdSetBertCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);
	CODING_8(buf, io->bertStatus);
	CODING_8(buf, io->bertDir);
	CODING_8(buf, io->bertTSList);
	CODING_8(buf, io->bertPattern);
	CODING_8(buf, io->bertInverted);
	CODING_8(buf, io->bertErrorRate);

	return (buf - start);
}

int CmdSetBertCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdSetBertCfgIn *io = (CmdSetBertCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);
	DECODE_8(buf, io->bertStatus, len);
	DECODE_8(buf, io->bertDir, len);
	DECODE_8(buf, io->bertTSList, len);
	DECODE_8(buf, io->bertPattern, len);
	DECODE_8(buf, io->bertInverted, len);
	DECODE_8(buf, io->bertErrorRate, len);

	return 0;
}

int CmdDelBertCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdDelBertCfgIn *io = (CmdDelBertCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);

	return (buf - start);
}

int CmdDelBertCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdDelBertCfgIn *io = (CmdDelBertCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);

	return 0;
}

int CmdStartPrbsInEncode(unsigned char *buf, int size, void *p)
{
	CmdStartPrbsIn *io = (CmdStartPrbsIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdStartPrbsInDecode(unsigned char *buf, int len, void *p)
{
	CmdStartPrbsIn *io = (CmdStartPrbsIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdStopPrbsInEncode(unsigned char *buf, int size, void *p)
{
	CmdStopPrbsIn *io = (CmdStopPrbsIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdStopPrbsInDecode(unsigned char *buf, int len, void *p)
{
	CmdStopPrbsIn *io = (CmdStopPrbsIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdInsertErrorInEncode(unsigned char *buf, int size, void *p)
{
	CmdInsertErrorIn *io = (CmdInsertErrorIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->bertErrors);

	return (buf - start);
}

int CmdInsertErrorInDecode(unsigned char *buf, int len, void *p)
{
	CmdInsertErrorIn *io = (CmdInsertErrorIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->bertErrors, len);
	return 0;
}

int CmdSysRebootInEncode(unsigned char *buf, int size, void *p)
{
	CmdSysRebootIn *io = (CmdSysRebootIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdSysRebootInDecode(unsigned char *buf, int len, void *p)
{
	CmdSysRebootIn *io = (CmdSysRebootIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdSysStartInEncode(unsigned char *buf, int size, void *p)
{
	CmdSysStartIn *io = (CmdSysStartIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdSysStartInDecode(unsigned char *buf, int len, void *p)
{
	CmdSysStartIn *io = (CmdSysStartIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdSysStopInEncode(unsigned char *buf, int size, void *p)
{
	CmdSysStopIn *io = (CmdSysStopIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdSysStopInDecode(unsigned char *buf, int len, void *p)
{
	CmdSysStopIn *io = (CmdSysStopIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdSysFirmwareUpgradeInEncode(unsigned char *buf, int size, void *p)
{
	CmdSysFirmwareUpgradeIn *io = (CmdSysFirmwareUpgradeIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->ctrlConfigFile.total);
	CODING_8(buf, io->ctrlConfigFile.num);
	CODING_8(buf, io->ctrlConfigFile.flag);
	CODING_8(buf, io->ctrlConfigFile.re);
	CODING_32(buf, io->ctrlConfigFile.crc);

	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdSysFirmwareUpgradeInDecode(unsigned char *buf, int len, void *p)
{
	CmdSysFirmwareUpgradeIn *io = (CmdSysFirmwareUpgradeIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->ctrlConfigFile.total, len);
	DECODE_8(buf, io->ctrlConfigFile.num, len);
	DECODE_8(buf, io->ctrlConfigFile.flag, len);
	DECODE_8(buf, io->ctrlConfigFile.re, len);
	DECODE_32(buf, io->ctrlConfigFile.crc, len);

	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGetLineStatInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetLineStatInfoIn *io = (CmdGetLineStatInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->stmID);
	CODING_32(buf, io->statusReportInterval);
	CODING_32(buf, io->statusReportCount);

	return (buf - start);
}

int CmdGetLineStatInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetLineStatInfoIn *io = (CmdGetLineStatInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->stmID, len);
	DECODE_32(buf, io->statusReportInterval, len);
	DECODE_32(buf, io->statusReportCount, len);

	return 0;
}

int CmdGetLineStatInfoOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetLineStatInfoOut *io = (CmdGetLineStatInfoOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->stmID);
	CODING_32(buf, io->sdhAlarm.Line);
	CODING_32(buf, io->sdhAlarm.HP);
	CODING_32(buf, io->sdhAlarm.LP);
	CODING_32(buf, io->sdhStatistics.rs_bip);
	CODING_32(buf, io->sdhStatistics.ms_bip);
	CODING_32(buf, io->sdhStatistics.ms_rei);
	CODING_32(buf, io->sdhStatistics.pje);
	CODING_32(buf, io->sdhStatistics.nje);
	CODING_32(buf, io->sdhStatistics.bip);
	CODING_32(buf, io->sdhStatistics.rei);

	return (buf - start);
}

int CmdGetLineStatInfoOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetLineStatInfoOut *io = (CmdGetLineStatInfoOut *)p;

	DECODE_8(buf, io->stmID, len);
	DECODE_32(buf, io->sdhAlarm.Line, len);
	DECODE_32(buf, io->sdhAlarm.HP, len);
	DECODE_32(buf, io->sdhAlarm.LP, len);
	DECODE_32(buf, io->sdhStatistics.rs_bip, len);
	DECODE_32(buf, io->sdhStatistics.ms_bip, len);
	DECODE_32(buf, io->sdhStatistics.ms_rei, len);
	DECODE_32(buf, io->sdhStatistics.pje, len);
	DECODE_32(buf, io->sdhStatistics.nje, len);
	DECODE_32(buf, io->sdhStatistics.bip, len);

	return 0;
}

int CmdClrLineStatInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdClrLineStatInfoIn *io = (CmdClrLineStatInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->stmID);

	return (buf - start);
}

int CmdClrLineStatInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdClrLineStatInfoIn *io = (CmdClrLineStatInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->stmID, len);

	return 0;
}

int CmdClrPortStatInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdClrPortStatInfoIn *io = (CmdClrPortStatInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);

	return (buf - start);
}

int CmdClrPortStatInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdClrPortStatInfoIn *io = (CmdClrPortStatInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);

	return 0;
}

int CmdStartPfmTestInEncode(unsigned char *buf, int size, void *p)
{
	CmdStartPfmTestIn *io = (CmdStartPfmTestIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->performanceTestType);

	return (buf - start);
}

int CmdStartPfmTestInDecode(unsigned char *buf, int len, void *p)
{
	CmdStartPfmTestIn *io = (CmdStartPfmTestIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->performanceTestType, len);

	return 0;
}

int CmdGetPortPfmResultInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortPfmResultIn *io = (CmdGetPortPfmResultIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->performanceTestType);
	CODING_32(buf, io->portID);
	CODING_32(buf, io->statusReportInterval);
	CODING_32(buf, io->statusReportCount);

	return (buf - start);
}

int CmdGetPortPfmResultInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortPfmResultIn *io = (CmdGetPortPfmResultIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->performanceTestType, len);
	DECODE_32(buf, io->portID, len);
	DECODE_32(buf, io->statusReportInterval, len);
	DECODE_32(buf, io->statusReportCount, len);

	return 0;
}

int CmdStopPfmTestInEncode(unsigned char *buf, int size, void *p)
{
	CmdStopPfmTestIn *io = (CmdStopPfmTestIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_8(buf, io->performanceTestType);

	return (buf - start);
}

int CmdStopPfmTestInDecode(unsigned char *buf, int len, void *p)
{
	CmdStopPfmTestIn *io = (CmdStopPfmTestIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_8(buf, io->performanceTestType, len);

	return 0;
}

int CmdGetChCfgInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetChCfgIn *io = (CmdGetChCfgIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->channelID);

	return (buf - start);
}

int CmdGetChCfgInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetChCfgIn *io = (CmdGetChCfgIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->channelID, len);

	return 0;
}

int CmdGetChCfgOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetChCfgOut *io = (CmdGetChCfgOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->channelID);
	CODING_8(buf, io->channleType);
	CODING_32(buf, io->channelPortID);
	CODING_8(buf, io->channelTS);
	CODING_8(buf, io->channelTSCount);
	CODING_32(buf, io->channelTSList);

	return (buf - start);
}

int CmdGetChCfgOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetChCfgOut *io = (CmdGetChCfgOut *)p;

	DECODE_32(buf, io->channelID, len);
	DECODE_8(buf, io->channleType, len);
	DECODE_32(buf, io->channelPortID, len);
	DECODE_8(buf, io->channelTS, len);
	DECODE_8(buf, io->channelTSCount, len);
	DECODE_32(buf, io->channelTSList, len);

	return 0;
}

int CmdGetChStatInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetChStatInfoIn *io = (CmdGetChStatInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->channelID);

	return (buf - start);
}

int CmdGetChStatInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetChStatInfoIn *io = (CmdGetChStatInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->channelID, len);

	return 0;
}

int CmdGetChStatInfoOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetChStatInfoOut *io = (CmdGetChStatInfoOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->channelID);
	CODING_32(buf, io->channelAlarm);
	CODING_32(buf, io->channelStatistics.rframes);
	CODING_32(buf, io->channelStatistics.rbytes);
	CODING_32(buf, io->channelStatistics.tframes);
	CODING_32(buf, io->channelStatistics.tbytes);
	CODING_32(buf, io->channelStatistics.rbuf_full_frames);
	CODING_32(buf, io->channelStatistics.rbuf_full_bytes);
	CODING_32(buf, io->channelStatistics.ll);
	CODING_32(buf, io->channelStatistics.fisu);
	CODING_32(buf, io->channelStatistics.lssu);
	CODING_32(buf, io->channelStatistics.msu);
	CODING_32(buf, io->channelStatistics.fsn_hop);
	CODING_32(buf, io->channelStatistics.fsn_dup);
	CODING_32(buf, io->channelStatistics.lg);
	CODING_32(buf, io->channelStatistics.no);
	CODING_32(buf, io->channelStatistics.ab);
	CODING_32(buf, io->channelStatistics.cr);

	return (buf - start);
}

int CmdGetChStatInfoOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetChStatInfoOut *io = (CmdGetChStatInfoOut *)p;

	DECODE_32(buf, io->channelID, len);
	DECODE_32(buf, io->channelAlarm, len);
	DECODE_32(buf, io->channelStatistics.rframes, len);
	DECODE_32(buf, io->channelStatistics.rbytes, len);
	DECODE_32(buf, io->channelStatistics.tframes, len);
	DECODE_32(buf, io->channelStatistics.tbytes, len);
	DECODE_32(buf, io->channelStatistics.rbuf_full_frames, len);
	DECODE_32(buf, io->channelStatistics.rbuf_full_bytes, len);
	DECODE_32(buf, io->channelStatistics.ll, len);
	DECODE_32(buf, io->channelStatistics.fisu, len);
	DECODE_32(buf, io->channelStatistics.lssu, len);
	DECODE_32(buf, io->channelStatistics.msu, len);
	DECODE_32(buf, io->channelStatistics.fsn_hop, len);
	DECODE_32(buf, io->channelStatistics.fsn_dup, len);
	DECODE_32(buf, io->channelStatistics.lg, len);
	DECODE_32(buf, io->channelStatistics.no, len);
	DECODE_32(buf, io->channelStatistics.ab, len);
	DECODE_32(buf, io->channelStatistics.cr, len);

	return 0;
}

int CmdClrChStatInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdClrChStatInfoIn *io = (CmdClrChStatInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->channelID);

	return (buf - start);
}

int CmdClrChStatInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdClrChStatInfoIn *io = (CmdClrChStatInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->channelID, len);

	return 0;
}

int CmdGetPortStatInfoInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortStatInfoIn *io = (CmdGetPortStatInfoIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);
	CODING_32(buf, io->portID);
	CODING_32(buf, io->statusReportInterval);
	CODING_32(buf, io->statusReportCount);

	return (buf - start);
}

int CmdGetPortStatInfoInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortStatInfoIn *io = (CmdGetPortStatInfoIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);
	DECODE_32(buf, io->portID, len);
	DECODE_32(buf, io->statusReportInterval, len);
	DECODE_32(buf, io->statusReportCount, len);

	return 0;
}

int CmdGetPortStatInfoOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortStatInfoOut *io = (CmdGetPortStatInfoOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->portID);
	CODING_32(buf, io->portAlarm);
	CODING_32(buf, io->portStatistics.lcvc);
	CODING_32(buf, io->portStatistics.pcvc);
	CODING_32(buf, io->portStatistics.fosc);
	CODING_32(buf, io->portStatistics.ebc);
	CODING_32(buf, io->portStatistics.bert_bbc);
	CODING_32(buf, io->portStatistics.bert_bec);

	return (buf - start);
}

int CmdGetPortStatInfoOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortStatInfoOut *io = (CmdGetPortStatInfoOut *)p;

	DECODE_32(buf, io->portID, len);
	DECODE_32(buf, io->portAlarm, len);
	DECODE_32(buf, io->portStatistics.lcvc, len);
	DECODE_32(buf, io->portStatistics.pcvc, len);
	DECODE_32(buf, io->portStatistics.fosc, len);
	DECODE_32(buf, io->portStatistics.ebc, len);
	DECODE_32(buf, io->portStatistics.bert_bbc, len);
	DECODE_32(buf, io->portStatistics.bert_bec, len);

	return 0;
}

int CmdGetSysResRepInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetSysResRepIn *io = (CmdGetSysResRepIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->deviceSequenceNumber);

	return (buf - start);
}

int CmdGetSysResRepInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetSysResRepIn *io = (CmdGetSysResRepIn *)p;

	DECODE_8(buf, io->deviceSequenceNumber, len);

	return 0;
}

int CmdGetSysResRepOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetSysResRepOut *io = (CmdGetSysResRepOut *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->sysInfoCPU);
	CODING_32(buf, io->sysInfoMemory);
	CODING_32(buf, io->sysInfoFilesystem);
	CODING_32(buf, io->sysInfoProcess);
	CODING_32(buf, io->size);

	return (buf - start);
}

int CmdGetSysResRepOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetSysResRepOut *io = (CmdGetSysResRepOut *)p;

	DECODE_32(buf, io->sysInfoCPU, len);
	DECODE_32(buf, io->sysInfoMemory, len);
	DECODE_32(buf, io->sysInfoFilesystem, len);
	DECODE_32(buf, io->sysInfoProcess, len);
	DECODE_32(buf, io->size, len);

	return 0;
}

int CmdGetPortPfmResultOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetPortPfmResultOut *io = (CmdGetPortPfmResultOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->performanceTestType);
	CODING_32(buf, io->portID);

	return (buf - start);
}

int CmdGetPortPfmResultOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetPortPfmResultOut *io = (CmdGetPortPfmResultOut *)p;

	DECODE_8(buf, io->performanceTestType, len);
	DECODE_32(buf, io->portID, len);

	return 0;
}

int CmdGetRawDataInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetRawDataIn *io = (CmdGetRawDataIn *)p;
	unsigned char *start = buf;

	CODING_32(buf, io->starttime);
	CODING_32(buf, io->cdrID);

	return (buf - start);
}

int CmdGetRawDataInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetRawDataIn *io = (CmdGetRawDataIn *)p;

	DECODE_32(buf, io->starttime, len);
	DECODE_32(buf, io->cdrID, len);

	return 0;
}

int CmdGetRawDataOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetRawDataOut *io = (CmdGetRawDataOut*)p;
	unsigned char *start = buf;

	CODING_32(buf, io->pktnum);
	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdGetRawDataOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetRawDataOut *io = (CmdGetRawDataOut *)p;

	DECODE_32(buf, io->pktnum, len);
	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGetRtVoiceInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetRtVoiceIn *io = (CmdGetRtVoiceIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->cmdType);
	CODING_16(buf, io->deviceID);
	CODING_32(buf, io->channelID);
	CODING_32(buf, io->subchID);

	return (buf - start);
}

int CmdGetRtVoiceInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetRtVoiceIn *io = (CmdGetRtVoiceIn *)p;

	DECODE_8(buf, io->cmdType, len);
	DECODE_16(buf, io->deviceID, len);
	DECODE_32(buf, io->channelID, len);
	DECODE_32(buf, io->subchID, len);

	return 0;
}

int CmdGetRtVoiceOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetRtVoiceOut *io = (CmdGetRtVoiceOut*)p;
	unsigned char *start = buf;

	CODING_8(buf, io->cmdType);
	CODING_16(buf, io->deviceID);
	CODING_32(buf, io->channelID);
	CODING_32(buf, io->subchID);
	CODING_32(buf, io->size);
	memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdGetRtVoiceOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetRtVoiceOut *io = (CmdGetRtVoiceOut *)p;

	DECODE_8(buf, io->cmdType, len);
	DECODE_16(buf, io->deviceID, len);
	DECODE_32(buf, io->channelID, len);
	DECODE_32(buf, io->subchID, len);
	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		io->data = malloc(io->size);
		if (io->data != NULL) {
			memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGetCallTraceInEncode(unsigned char *buf, int size, void *p)
{
	CmdGetCallTraceIn *io = (CmdGetCallTraceIn *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->cmdtype);
	CODING_32(buf, io->maxcdr);
	CODING_32(buf, io->size);
	if (io->size > 0)
		memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdGetCallTraceInDecode(unsigned char *buf, int len, void *p)
{
	CmdGetCallTraceIn *io = (CmdGetCallTraceIn *)p;

	DECODE_8(buf, io->cmdtype, len);
	DECODE_32(buf, io->maxcdr, len);
	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		if (io->size > 0) {
			io->data = malloc(io->size);
			if (io->data != NULL)
				memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}

int CmdGetCallTraceOutEncode(unsigned char *buf, int size, void *p)
{
	CmdGetCallTraceOut *io = (CmdGetCallTraceOut *)p;
	unsigned char *start = buf;

	CODING_8(buf, io->cmdtype);
	CODING_32(buf, io->cdrID);
	CODING_32(buf, io->cdrStartTime);
	CODING_32(buf, io->cdrtype);
	CODING_32(buf, io->cdrflag);
	CODING_32(buf, io->pktseqnum);
	CODING_32(buf, io->size);
	if (io->size > 0)
		memcpy(buf, io->data, io->size);
	buf += io->size;

	return (buf - start);
}

int CmdGetCallTraceOutDecode(unsigned char *buf, int len, void *p)
{
	CmdGetCallTraceOut *io = (CmdGetCallTraceOut *)p;

	DECODE_8(buf, io->cmdtype, len);
	DECODE_32(buf, io->cdrID, len);
	DECODE_32(buf, io->cdrStartTime, len);
	DECODE_32(buf, io->cdrtype, len);
	DECODE_32(buf, io->cdrflag, len);
	DECODE_32(buf, io->pktseqnum, len);
	DECODE_32(buf, io->size, len);
	if (io->data == NULL) {
		if (io->size > 0) {
			io->data = malloc(io->size);
			if (io->data != NULL)
				memcpy(io->data, buf, io->size);
		}
	}

	return 0;
}


