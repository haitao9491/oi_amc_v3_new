/*
 *
 * pktcmdarg.h - A brief description goes here.
 *
 */

#ifndef _HEAD_PKTCMDARG_25F13024_537B2F97_6EC3B63B_H
#define _HEAD_PKTCMDARG_25F13024_537B2F97_6EC3B63B_H

#include "pktcmdtype.h"

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetHwModelIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetHwVersionIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetSwVersionIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetHwSerialNumIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetHwDescriptionIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	DeviceDescription deviceDescription[128];
} CmdSetHwDescriptionIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	EtherItfNumber etherItfNumber;
} CmdGetEtherInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	EtherItfNumber etherItfNumber;
	IPAddress etherItfIP;
	IPAddress etherItfNetmask;
} CmdSetEtherInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	EtherItfNumber etherItfNumber;
} CmdGetMacIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	EtherItfNumber etherItfNumber;
	MACAddress etherItfMacAddr[6];
} CmdSetMacIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetServerIpIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	IPAddress serverIPAddress;
} CmdSetServerIpIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetGatewayIpIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	IPAddress gatewayIPAddress;
} CmdSetGatewayIpIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetSysTimeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	SysTime sysTime;
} CmdSetSysTimeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetNtpModeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	NtpMode ntpMode;
	IPAddress ntpServerIPAddress;
} CmdSetNtpModeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetClkTypeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	ClkType clkType;
	ClkSrc clkSrc;
} CmdSetClkTypeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetPortsIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetGlobalPortCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PhyItfType phyItfType;
	LineCode lineCode;
	Gain gain;
	FrameFormat inputFrameFormat;
	Impedance inputImpedance;
	FrameFormat outputFrameFormat;
	Impedance outputImpedance;
} CmdSetGlobalPortCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
} CmdGetPortCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
	PhyItfType phyItfType;
	LineCode lineCode;
	Gain gain;
	FrameFormat inputFrameFormat;
	Impedance inputImpedance;
	FrameFormat outputFrameFormat;
	Impedance outputImpedance;
} CmdSetPortCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
} CmdDelPortCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdDownloadPortCfgFileIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdUploadPortCfgFileIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdDownloadChCfgFileIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdUploadChCfgFileIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetDevIdIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	DeviceID deviceID;
} CmdSetDevIdIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetPackTypeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PacketSubType packetSubType;
} CmdSetPackTypeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetDevModeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	DeviceMode deviceMode;
} CmdSetDevModeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID dstPortID;
	TsID dstTsID;
} CmdGetPortSdhMatrixCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID dstPortID;
	TsID dstTsID;
	StmID srcStmID;
	TribID srcTribID;
	TsID srcTsID;
} CmdSetPortSdhMatrixCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID dstPortID;
	TsID dstTsID;
} CmdGetPortMatrixCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID dstPortID;
	TsID dstTsID;
	PortID srcPortID;
	TsID srcTsID;
} CmdSetPortMatrixCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdDownloadMatrixCfgFileIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdUploadMatrixCfgFileIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	ScanInterval scanInterval;
} CmdGainScanIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	ScanInterval  scanInterval;
	ScanType scanType;
	ScanNList scanNList;
} CmdChanScanIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetGlobalBertCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	BertStatus bertStatus;
	BertDir bertDir;
	BertTSList bertTSList;
	BertPattern bertPattern;
	BertInverted bertInverted;
	BertErrRate bertErrorRate;
} CmdSetGlobalBertCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
} CmdGetBertCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
	BertStatus bertStatus;
	BertDir bertDir;
	BertTSList bertTSList;
	BertPattern bertPattern;
	BertInverted bertInverted;
	BertErrRate bertErrorRate;
} CmdSetBertCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
} CmdDelBertCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdStartPrbsIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdStopPrbsIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	BertErrors bertErrors;
} CmdInsertErrorIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdSysRebootIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdSysStartIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdSysStopIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdSysFirmwareUpgradeIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
} CmdGetSysResRepIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	StmID stmID;
	ReportInterval statusReportInterval;
	ReportCount statusReportCount;
} CmdGetLineStatInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	StmID stmID;
} CmdClrLineStatInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
	ReportInterval statusReportInterval;
	ReportCount statusReportCount;
} CmdGetPortStatInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PortID portID;
} CmdClrPortStatInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PerformanceTestType performanceTestType;
} CmdStartPfmTestIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PerformanceTestType performanceTestType;
	PortID portID;
	ReportInterval statusReportInterval;
	ReportCount statusReportCount;
} CmdGetPortPfmResultIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	PerformanceTestType performanceTestType;
} CmdStopPfmTestIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	ChannelID channelID;
} CmdGetChCfgIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	ChannelID channelID;
} CmdGetChStatInfoIn;

typedef struct {
	DeviceSequenceNumber deviceSequenceNumber;
	ChannelID channelID;
} CmdClrChStatInfoIn;

// Output
typedef struct {
	DeviceModel deviceModel;
} CmdGetHwModelOut;

typedef struct {
	DeviceHWVersion deviceHWVersion[8];
} CmdGetHwVersionOut;

typedef struct {
	DeviceSWVersion deviceSWVersion[64];
} CmdGetSwVersionOut;

typedef struct {
	DeviceSerialNumber deviceSerialNumber[24];
} CmdGetHwSerialNumOut;

typedef struct {
	DeviceDescription deviceDescription[128];
} CmdGetHwDescriptionOut;

typedef struct {
	EtherItfNumber etherItfNumber;
	IPAddress etherItfIP;
	IPAddress etherItfNetmask;
} CmdGetEtherInfoOut;

typedef struct {
	EtherItfNumber etherItfNumber;
	MACAddress etherItfMacAddr[6];
} CmdGetMacOut;

typedef struct {
	IPAddress serverIPAddress;
} CmdGetServerIpOut;

typedef struct {
	IPAddress gatewayIPAddress;
} CmdGetGatewayIpOut;

typedef struct {
	SysTime sysTime;
} CmdGetSysTimeOut;

typedef struct {
	NtpMode ntpMode;
	IPAddress ntpServerIPAddress;
} CmdGetNtpModeOut;

typedef struct {
	ClkType clkType;
	ClkSrc clkSrc;
} CmdGetClkTypeOut;

typedef struct {
	Ports inputPorts;
	Ports outputPorts;
} CmdGetPortsOut;

typedef struct {
	PhyItfType phyItfType;
	LineCode lineCode;
	Gain gain;
	FrameFormat inputFrameFormat;
	Impedance inputImpedance;
	FrameFormat outputFrameFormat;
	Impedance outputImpedance;
} CmdGetGlobalPortCfgOut;

typedef struct {
	PortID portID;
	PhyItfType phyItfType;
	LineCode lineCode;
	Gain gain;
	FrameFormat inputFrameFormat;
	Impedance inputImpedance;
	FrameFormat outputFrameFormat;
	Impedance outputImpedance;
} CmdGetPortCfgOut;

typedef struct {
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdDownloadPortCfgFileOut;

typedef struct {
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdDownloadChCfgFileOut;

typedef struct {
	DeviceID deviceID;
} CmdGetDevIdOut;

typedef struct {
	PacketSubType packetSubType;
} CmdGetPackTypeOut;

typedef struct {
	DeviceMode deviceMode;
} CmdGetDevModeOut;

typedef struct {
	PortID dstPortID;
	TsID dstTsID;
	StmID srcStmID;
	TribID srcTribID;
	TsID srcTsID;
} CmdGetPortSdhMatrixCfgOut;

typedef struct {
	PortID dstPortID;
	TsID dstTsID;
	PortID srcPortID;
	TsID srcTsID;
} CmdGetPortMatrixCfgOut;

typedef struct {
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdDownloadMatrixCfgFileOut;

typedef struct {
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdGainScanOut;

typedef struct {
	CtrlConfigFile ctrlConfigFile;
	BufferData data;
	BufferSize size;
} CmdChanScanOut;

typedef struct {
	BertStatus bertStatus;
	BertDir bertDir;
	BertTSList bertTSList;
	BertPattern bertPattern;
	BertInverted bertInverted;
	BertErrRate bertErrorRate;
} CmdGetGlobalBertCfgOut;

typedef struct {
	PortID portID;
	BertStatus bertStatus;
	BertDir bertDir;
	BertTSList bertTSList;
	BertPattern bertPattern;
	BertInverted bertInverted;
	BertErrRate bertErrorRate;
} CmdGetBertCfgOut;

typedef struct {
	SysInfoCPU sysInfoCPU;
	SysInfoMemory sysInfoMemory;
	SysInfoFilesystem sysInfoFilesystem;
	SysInfoProcess sysInfoProcess;
	BufferData data;
	BufferSize size;
} CmdGetSysResRepOut;

typedef struct {
	StmID stmID;
	SdhAlarm sdhAlarm;
	SdhStatistics sdhStatistics;
} CmdGetLineStatInfoOut;

typedef struct {
	PortID portID;
	PortAlarm portAlarm;
	PortStatistics portStatistics;
} CmdGetPortStatInfoOut;

typedef struct {
	PerformanceTestType performanceTestType;
	PortID portID;
	PerformanceTestResult performanceTestResult;
} CmdGetPortPfmResultOut;

typedef struct {
	ChannelID channelID;
	ChannelType channleType;
	PortID channelPortID;
	TsID channelTS;
	TsCount channelTSCount;
	TsList channelTSList;
} CmdGetChCfgOut;

typedef struct {
	ChannelID channelID;
	ChannelAlarm channelAlarm;
	ChannelStatistics channelStatistics;
} CmdGetChStatInfoOut;

typedef struct {
	CdrID cdrID;
	StartTIME starttime;
} CmdGetRawDataIn;

typedef struct {
	CdrID cdrID;
	StartTIME starttime;
	BufferData data;
	BufferSize size;
	PktNUM pktnum;
} CmdGetRawDataOut;

typedef struct {
	CmdType cmdType;
	DeviceID deviceID;
	ChannelID channelID;
	SubChannelID subchID;
} CmdGetRtVoiceIn;

typedef struct {
	CmdType cmdType;
	DeviceID deviceID;
	ChannelID channelID;
	SubChannelID subchID;
	BufferData data;
	BufferSize size;
} CmdGetRtVoiceOut;

typedef struct {
	CmdType cmdtype;
	MaximumCdr maxcdr;
	BufferData data;
	BufferSize size;
} CmdGetCallTraceIn;

typedef struct {
	CmdType cmdtype;
	CdrID cdrID;
	CdrStartTime cdrStartTime;
	CdrType cdrtype;
	CdrFlag cdrflag;
	BufferData data;
	BufferSize size;
	PktSeqNUM pktseqnum;
} CmdGetCallTraceOut;

#if defined(__cplusplus)
extern "C" {
#endif

int CmdGetHwModelInEncode(unsigned char *buf, int size, void *p);
int CmdGetHwModelInDecode(unsigned char *buf, int len, void *p);
int CmdGetHwModelOutEncode(unsigned char *buf, int size, void *p);
int CmdGetHwModelOutDecode(unsigned char *buf, int len, void *p);
int CmdGetHwVersionInEncode(unsigned char *buf, int size, void *p);
int CmdGetHwVersionInDecode(unsigned char *buf, int len, void *p);
int CmdGetHwVersionOutEncode(unsigned char *buf, int size, void *p);
int CmdGetHwVersionOutDecode(unsigned char *buf, int len, void *p);
int CmdGetSwVersionInEncode(unsigned char *buf, int size, void *p);
int CmdGetSwVersionInDecode(unsigned char *buf, int len, void *p);
int CmdGetSwVersionOutEncode(unsigned char *buf, int size, void *p);
int CmdGetSwVersionOutDecode(unsigned char *buf, int len, void *p);
int CmdGetHwSerialNumInEncode(unsigned char *buf, int size, void *p);
int CmdGetHwSerialNumInDecode(unsigned char *buf, int len, void *p);
int CmdGetHwSerialNumOutEncode(unsigned char *buf, int size, void *p);
int CmdGetHwSerialNumOutDecode(unsigned char *buf, int len, void *p);
int CmdGetHwDescriptionInEncode(unsigned char *buf, int size, void *p);
int CmdGetHwDescriptionInDecode(unsigned char *buf, int len, void *p);
int CmdGetHwDescriptionOutEncode(unsigned char *buf, int size, void *p);
int CmdGetHwDescriptionOutDecode(unsigned char *buf, int len, void *p);
int CmdSetHwDescriptionInEncode(unsigned char *buf, int size, void *p);
int CmdSetHwDescriptionInDecode(unsigned char *buf, int len, void *p);
int CmdGetEtherInfoInEncode(unsigned char *buf, int size, void *p);
int CmdGetEtherInfoInDecode(unsigned char *buf, int len, void *p);
int CmdGetEtherInfoOutEncode(unsigned char *buf, int size, void *p);
int CmdGetEtherInfoOutDecode(unsigned char *buf, int len, void *p);
int CmdSetEtherInfoInEncode(unsigned char *buf, int size, void *p);
int CmdSetEtherInfoInDecode(unsigned char *buf, int len, void *p);
int CmdSetServerIpInEncode(unsigned char *buf, int size, void *p);
int CmdSetServerIpInDecode(unsigned char *buf, int len, void *p);
int CmdGetMacInEncode(unsigned char *buf, int size, void *p);
int CmdGetMacInDecode(unsigned char *buf, int len, void *p);
int CmdGetMacOutEncode(unsigned char *buf, int size, void *p);
int CmdGetMacOutDecode(unsigned char *buf, int len, void *p);
int CmdSetMacInEncode(unsigned char *buf, int size, void *p);
int CmdSetMacInDecode(unsigned char *buf, int len, void *p);
int CmdGetServerIpInEncode(unsigned char *buf, int size, void *p);
int CmdGetServerIpInDecode(unsigned char *buf, int len, void *p);
int CmdGetServerIpOutEncode(unsigned char *buf, int size, void *p);
int CmdGetServerIpOutDecode(unsigned char *buf, int len, void *p);
int CmdSetServerIpInEncode(unsigned char *buf, int size, void *p);
int CmdSetServerIpInDecode(unsigned char *buf, int len, void *p);
int CmdGetGatewayIpInEncode(unsigned char *buf, int size, void *p);
int CmdGetGatewayIpInDecode(unsigned char *buf, int len, void *p);
int CmdGetGatewayIpOutEncode(unsigned char *buf, int size, void *p);
int CmdGetGatewayIpOutDecode(unsigned char *buf, int len, void *p);
int CmdSetGatewayIpInEncode(unsigned char *buf, int size, void *p);
int CmdSetGatewayIpInDecode(unsigned char *buf, int len, void *p);
int CmdGetSysTimeInEncode(unsigned char *buf, int size, void *p);
int CmdGetSysTimeInDecode(unsigned char *buf, int len, void *p);
int CmdGetSysTimeOutEncode(unsigned char *buf, int size, void *p);
int CmdGetSysTimeOutDecode(unsigned char *buf, int len, void *p);
int CmdSetSysTimeInEncode(unsigned char *buf, int size, void *p);
int CmdSetSysTimeInDecode(unsigned char *buf, int len, void *p);
int CmdGetNtpModeInEncode(unsigned char *buf, int size, void *p);
int CmdGetNtpModeInDecode(unsigned char *buf, int len, void *p);
int CmdGetNtpModeOutEncode(unsigned char *buf, int size, void *p);
int CmdGetNtpModeOutDecode(unsigned char *buf, int len, void *p);
int CmdSetNtpModeInEncode(unsigned char *buf, int size, void *p);
int CmdSetNtpModeInDecode(unsigned char *buf, int len, void *p);
int CmdGetClkTypeInEncode(unsigned char *buf, int size, void *p);
int CmdGetClkTypeInDecode(unsigned char *buf, int len, void *p);
int CmdGetClkTypeOutEncode(unsigned char *buf, int size, void *p);
int CmdGetClkTypeOutDecode(unsigned char *buf, int len, void *p);
int CmdSetClkTypeInEncode(unsigned char *buf, int size, void *p);
int CmdSetClkTypeInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortsInEncode(unsigned char *buf, int size, void *p);
int CmdGetPortsInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortsOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPortsOutDecode(unsigned char *buf, int len, void *p);
int CmdGetGlobalPortCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetGlobalPortCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetGlobalPortCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetGlobalPortCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdSetGlobalPortCfgInEncode(unsigned char *buf, int size, void *p);
int CmdSetGlobalPortCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetPortCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPortCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdSetPortCfgInEncode(unsigned char *buf, int size, void *p);
int CmdSetPortCfgInDecode(unsigned char *buf, int len, void *p);
int CmdDelPortCfgInEncode(unsigned char *buf, int size, void *p);
int CmdDelPortCfgInDecode(unsigned char *buf, int len, void *p);
int CmdDownloadPortCfgFileInEncode(unsigned char *buf, int size, void *p);
int CmdDownloadPortCfgFileInDecode(unsigned char *buf, int len, void *p);
int CmdDownloadPortCfgFileOutEncode(unsigned char *buf, int size, void *p);
int CmdDownloadPortCfgFileOutDecode(unsigned char *buf, int len, void *p);
int CmdUploadPortCfgFileInEncode(unsigned char *buf, int size, void *p);
int CmdUploadPortCfgFileInDecode(unsigned char *buf, int len, void *p);
int CmdDownloadChCfgFileInEncode(unsigned char *buf, int size, void *p);
int CmdDownloadChCfgFileInDecode(unsigned char *buf, int len, void *p);
int CmdDownloadChCfgFileOutEncode(unsigned char *buf, int size, void *p);
int CmdDownloadChCfgFileOutDecode(unsigned char *buf, int len, void *p);
int CmdUploadChCfgFileInEncode(unsigned char *buf, int size, void *p);
int CmdUploadChCfgFileInDecode(unsigned char *buf, int len, void *p);
int CmdGetDevIdInEncode(unsigned char *buf, int size, void *p);
int CmdGetDevIdInDecode(unsigned char *buf, int len, void *p);
int CmdGetDevIdOutEncode(unsigned char *buf, int size, void *p);
int CmdGetDevIdOutDecode(unsigned char *buf, int len, void *p);
int CmdSetDevIdInEncode(unsigned char *buf, int size, void *p);
int CmdSetDevIdInDecode(unsigned char *buf, int len, void *p);
int CmdGetPackTypeInEncode(unsigned char *buf, int size, void *p);
int CmdGetPackTypeInDecode(unsigned char *buf, int len, void *p);
int CmdGetPackTypeOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPackTypeOutDecode(unsigned char *buf, int len, void *p);
int CmdSetPackTypeInEncode(unsigned char *buf, int size, void *p);
int CmdSetPackTypeInDecode(unsigned char *buf, int len, void *p);
int CmdGetDevModeInEncode(unsigned char *buf, int size, void *p);
int CmdGetDevModeInDecode(unsigned char *buf, int len, void *p);
int CmdGetDevModeOutEncode(unsigned char *buf, int size, void *p);
int CmdGetDevModeOutDecode(unsigned char *buf, int len, void *p);
int CmdSetDevModeInEncode(unsigned char *buf, int size, void *p);
int CmdSetDevModeInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortSdhMatrixCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetPortSdhMatrixCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortSdhMatrixCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPortSdhMatrixCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdSetPortSdhMatrixCfgInEncode(unsigned char *buf, int size, void *p);
int CmdSetPortSdhMatrixCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortMatrixCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetPortMatrixCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortMatrixCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPortMatrixCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdSetPortMatrixCfgInEncode(unsigned char *buf, int size, void *p);
int CmdSetPortMatrixCfgInDecode(unsigned char *buf, int len, void *p);
int CmdDownloadMatrixCfgFileInEncode(unsigned char *buf, int size, void *p);
int CmdDownloadMatrixCfgFileInDecode(unsigned char *buf, int len, void *p);
int CmdDownloadMatrixCfgFileOutEncode(unsigned char *buf, int size, void *p);
int CmdDownloadMatrixCfgFileOutDecode(unsigned char *buf, int len, void *p);
int CmdUploadMatrixCfgFileInEncode(unsigned char *buf, int size, void *p);
int CmdUploadMatrixCfgFileInDecode(unsigned char *buf, int len, void *p);
int CmdGainScanInEncode(unsigned char *buf, int size, void *p);
int CmdGainScanInDecode(unsigned char *buf, int len, void *p);
int CmdGainScanOutEncode(unsigned char *buf, int size, void *p);
int CmdGainScanOutDecode(unsigned char *buf, int len, void *p);
int CmdChanScanInEncode(unsigned char *buf, int size, void *p);
int CmdChanScanInDecode(unsigned char *buf, int len, void *p);
int CmdChanScanOutEncode(unsigned char *buf, int size, void *p);
int CmdChanScanOutDecode(unsigned char *buf, int len, void *p);
int CmdGetGlobalBertCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetGlobalBertCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetGlobalBertCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetGlobalBertCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdSetGlobalBertCfgInEncode(unsigned char *buf, int size, void *p);
int CmdSetGlobalBertCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetBertCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetBertCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetBertCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetBertCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdSetBertCfgInEncode(unsigned char *buf, int size, void *p);
int CmdSetBertCfgInDecode(unsigned char *buf, int len, void *p);
int CmdDelBertCfgInEncode(unsigned char *buf, int size, void *p);
int CmdDelBertCfgInDecode(unsigned char *buf, int len, void *p);
int CmdStartPrbsInEncode(unsigned char *buf, int size, void *p);
int CmdStartPrbsInDecode(unsigned char *buf, int len, void *p);
int CmdStopPrbsInEncode(unsigned char *buf, int size, void *p);
int CmdStopPrbsInDecode(unsigned char *buf, int len, void *p);
int CmdInsertErrorInEncode(unsigned char *buf, int size, void *p);
int CmdInsertErrorInDecode(unsigned char *buf, int len, void *p);
int CmdSysRebootInEncode(unsigned char *buf, int size, void *p);
int CmdSysRebootInDecode(unsigned char *buf, int len, void *p);
int CmdSysStartInEncode(unsigned char *buf, int size, void *p);
int CmdSysStartInDecode(unsigned char *buf, int len, void *p);
int CmdSysStopInEncode(unsigned char *buf, int size, void *p);
int CmdSysStopInDecode(unsigned char *buf, int len, void *p);
int CmdSysFirmwareUpgradeInEncode(unsigned char *buf, int size, void *p);
int CmdSysFirmwareUpgradeInDecode(unsigned char *buf, int len, void *p);
int CmdGetLineStatInfoInEncode(unsigned char *buf, int size, void *p);
int CmdGetLineStatInfoInDecode(unsigned char *buf, int len, void *p);
int CmdGetLineStatInfoOutEncode(unsigned char *buf, int size, void *p);
int CmdGetLineStatInfoOutDecode(unsigned char *buf, int len, void *p);
int CmdClrLineStatInfoInEncode(unsigned char *buf, int size, void *p);
int CmdClrLineStatInfoInDecode(unsigned char *buf, int len, void *p);
int CmdClrPortStatInfoInEncode(unsigned char *buf, int size, void *p);
int CmdClrPortStatInfoInDecode(unsigned char *buf, int len, void *p);
int CmdStartPfmTestInEncode(unsigned char *buf, int size, void *p);
int CmdStartPfmTestInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortPfmResultInEncode(unsigned char *buf, int size, void *p);
int CmdGetPortPfmResultInDecode(unsigned char *buf, int len, void *p);
int CmdStopPfmTestInEncode(unsigned char *buf, int size, void *p);
int CmdStopPfmTestInDecode(unsigned char *buf, int len, void *p);
int CmdGetChCfgInEncode(unsigned char *buf, int size, void *p);
int CmdGetChCfgInDecode(unsigned char *buf, int len, void *p);
int CmdGetChCfgOutEncode(unsigned char *buf, int size, void *p);
int CmdGetChCfgOutDecode(unsigned char *buf, int len, void *p);
int CmdGetChStatInfoInEncode(unsigned char *buf, int size, void *p);
int CmdGetChStatInfoInDecode(unsigned char *buf, int len, void *p);
int CmdGetChStatInfoOutEncode(unsigned char *buf, int size, void *p);
int CmdGetChStatInfoOutDecode(unsigned char *buf, int len, void *p);
int CmdClrChStatInfoInEncode(unsigned char *buf, int size, void *p);
int CmdClrChStatInfoInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortStatInfoInEncode(unsigned char *buf, int size, void *p);
int CmdGetPortStatInfoInDecode(unsigned char *buf, int len, void *p);
int CmdGetPortStatInfoOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPortStatInfoOutDecode(unsigned char *buf, int len, void *p);
int CmdGetSysResRepInEncode(unsigned char *buf, int size, void *p);
int CmdGetSysResRepInDecode(unsigned char *buf, int len, void *p);
int CmdGetSysResRepOutEncode(unsigned char *buf, int size, void *p);
int CmdGetSysResRepOutDecode(unsigned char *buf, int len, void *p);
int CmdGetPortPfmResultOutEncode(unsigned char *buf, int size, void *p);
int CmdGetPortPfmResultOutDecode(unsigned char *buf, int len, void *p);
int CmdGetRawDataInEncode(unsigned char *buf, int size, void *p);
int CmdGetRawDataInDecode(unsigned char *buf, int len, void *p);
int CmdGetRawDataOutEncode(unsigned char *buf, int size, void *p);
int CmdGetRawDataOutDecode(unsigned char *buf, int len, void *p);
int CmdGetRtVoiceInEncode(unsigned char *buf, int size, void *p);
int CmdGetRtVoiceInDecode(unsigned char *buf, int len, void *p);
int CmdGetRtVoiceOutEncode(unsigned char *buf, int size, void *p);
int CmdGetRtVoiceOutDecode(unsigned char *buf, int len, void *p);
int CmdGetCallTraceInEncode(unsigned char *buf, int size, void *p);
int CmdGetCallTraceInDecode(unsigned char *buf, int len, void *p);
int CmdGetCallTraceOutEncode(unsigned char *buf, int size, void *p);
int CmdGetCallTraceOutDecode(unsigned char *buf, int len, void *p);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTCMDARG_25F13024_537B2F97_6EC3B63B_H */
