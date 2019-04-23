/*
 *
 * pktcmdtype.h - A brief description goes here.
 *
 */

#ifndef _HEAD_PKTCMDTYPE_354B0EA5_05D49255_70285311_H
#define _HEAD_PKTCMDTYPE_354B0EA5_05D49255_70285311_H

typedef unsigned char         BertDir;
typedef unsigned int          BertErrors;
typedef unsigned char         BertErrRate;
typedef unsigned char         BertInverted;
typedef unsigned char         BertPattern;
typedef unsigned char         BertStatus;
typedef unsigned char         BertTSList;
typedef char *                BufferData;
typedef int                   BufferSize;
typedef unsigned int          CdrID;
typedef unsigned int          CdrType;
typedef unsigned int          CdrFlag;
typedef unsigned int          CdrStartTime;
typedef unsigned int          ChannelAlarm;
typedef unsigned int          ChannelID;
typedef unsigned char         ChannelType;
typedef unsigned short        ClkSrc;
typedef unsigned char         ClkType;
typedef unsigned char         CmdType;
typedef char                  DeviceDescription;
typedef char                  DeviceHWVersion;
typedef unsigned short        DeviceID;
typedef unsigned char         DeviceMode;
typedef unsigned char         DeviceModel;
typedef unsigned char         DeviceSequenceNumber;
typedef char                  DeviceSerialNumber;
typedef char                  DeviceSWVersion;
typedef unsigned char         EtherItfNumber;
typedef unsigned char         FrameFormat;
typedef unsigned char         Gain;
typedef unsigned char         Impedance;
typedef unsigned int          IPAddress;
typedef unsigned char         LineCode;
typedef unsigned char         MACAddress;
typedef unsigned int          MaximumCdr;
typedef unsigned char         NtpMode;
typedef unsigned char         PacketSubType;
typedef unsigned char         PerformanceTestType;
typedef unsigned char         PhyItfType;
typedef unsigned int          PortID;
typedef unsigned int          Ports;
typedef unsigned int          ReportCount;
typedef unsigned int          ReportInterval;
typedef unsigned int          ScanNList;
typedef unsigned char         ScanInterval;
typedef unsigned char         ScanType;
typedef unsigned char         StmID;
typedef unsigned char         TribID;
typedef unsigned char         TsCount;
typedef unsigned char         TsID;
typedef unsigned int          TsList;
typedef unsigned int          PortAlarm;
typedef unsigned int          SysInfoCPU;
typedef unsigned int          SysInfoMemory;
typedef unsigned int          SysInfoFilesystem;
typedef unsigned int          SysInfoProcess;
typedef unsigned int          PerformanceTestResult;
typedef unsigned int          StartTIME;
typedef int                   PktNUM;
typedef unsigned int          PktSeqNUM;
typedef unsigned int          SubChannelID;

#define PORTALARM_LOS_RED     0x00000001
#define PORTALARM_LOF         0x00000002
#define PORTALARM_AIS_BLUE    0x00000004
#define PORTALARM_RAI_YEL     0x00000008
#define PORTALARM_CASMFLOSS   0x00000010
#define PORTALARM_CRCMFLOSS   0x00000020
#define PORTALARM_CASMFRAI    0x00000040
#define PORTALARM_BERTSYNC    0x00001000
#define PORTALARM_BERT_RAI    0x00002000
#define PORTALARM_BERT_RA0    0x00004000
#define PORTALARM_OEQ         0x00010000
#define PORTALARM_UEQ         0x00020000
#define PORTALARM_JALT        0x00040000
#define PORTALARM_RESF        0x00080000
#define PORTALARM_RESEM       0x00100000
#define PORTALARM_RSLIP       0x00200000
#define PORTALARM_TOC         0x01000000
#define PORTALARM_TSC         0x02000000
#define PORTALARM_REL         0xF0000000

#define SDH_ALARM_LOS         0x00000001
#define SDH_ALARM_LOF         0x00000002
#define SDH_ALARM_RS_TIM      0x00000004
#define SDH_ALARM_SF          0x00000008
#define SDH_ALARM_SD          0x00000010
#define SDH_ALARM_MS_AIS      0x00000020
#define SDH_ALARM_AU_AIS      0x00000100
#define SDH_ALARM_AU_LOP      0x00000200
#define SDH_ALARM_HP_TIM      0x00000001
#define SDH_ALARM_HP_TIU      0x00000002
#define SDH_ALARM_HP_ERDI     0x00000004
#define SDH_ALARM_HP_RDI      0x00000008
#define SDH_ALARM_HP_PDI      0x00000010
#define SDH_ALARM_HP_UNEQ     0x00000020
#define SDH_ALARM_HP_PLM      0x00000040
#define SDH_ALARM_HP_PLU      0x00000080
#define SDH_ALARM_HP_SF       0x00000100
#define SDH_ALARM_HP_SD       0x00000200
#define SDH_ALARM_LP_TAIS     0x00000001
#define SDH_ALARM_LP_LOP      0x00000002
#define SDH_ALARM_LP_RDI      0x00000004
#define SDH_ALARM_LP_ERDI     0x00000008
#define SDH_ALARM_LP_PSLU     0x00000010
#define SDH_ALARM_LP_PSLM     0x00000020
#define SDH_ALARM_LP_UNEQ     0x00000040
#define SDH_ALARM_LP_PDI      0x00000080
#define SDH_ALARM_LP_TIU      0x00000100
#define SDH_ALARM_LP_TIM      0x00000200
#define SDH_ALARM_LP_SF       0x00000400
#define SDH_ALARM_LP_SD       0x00000800
#define SDH_ALARM_LP_LOM      0x00001000

typedef struct {
	unsigned int Line;
	unsigned int HP;
	unsigned int LP;
} SdhAlarm;

typedef struct {
	unsigned int rframes;
	unsigned int rbytes;
	unsigned int tframes;
	unsigned int tbytes;
	unsigned int rbuf_full_frames;
	unsigned int rbuf_full_bytes;
	unsigned int ll ;
	unsigned int fisu;
	unsigned int lssu;
	unsigned int msu;
	unsigned int fsn_hop;
	unsigned int fsn_dup;
	unsigned int lg;
	unsigned int no;
	unsigned int ab;
	unsigned int cr;
} ChannelStatistics;

typedef struct {
	unsigned char total;
	unsigned char num;
	unsigned char flag;
	unsigned char re;
	unsigned int crc;
} CtrlConfigFile;

typedef struct {
	unsigned int lcvc;
	unsigned int pcvc;
	unsigned int fosc;
	unsigned int ebc;
	unsigned int bert_bbc;
	unsigned int bert_bec;
} PortStatistics;

typedef struct {
	unsigned int rs_bip;
	unsigned int ms_bip;
	unsigned int ms_rei;
	unsigned int pje;
	unsigned int nje;
	unsigned int bip;
	unsigned int rei;
} SdhStatistics;

typedef struct {
	unsigned int sec;
	unsigned int usec;
} SysTime;

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTCMDTYPE_354B0EA5_05D49255_70285311_H */
