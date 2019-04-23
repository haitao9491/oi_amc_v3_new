/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oic.h - A description goes here.
 *
 */

#ifndef _HEAD_OIC_12720691_154A4884_54CE5B33_H
#define _HEAD_OIC_12720691_154A4884_54CE5B33_H

#define OIC_NAME			"oicdev"

#define OIC_IOC_MAGIC		'O'
#define OIC_GET_FPGA_VERSION            _IOWR(OIC_IOC_MAGIC, 0x1, void *)
#define OIC_GET_FPGA_DDR_STATUS	     	_IOWR(OIC_IOC_MAGIC, 0x2, void *)
#define OIC_SET_FPGA_BD_STARTUP	     	_IOWR(OIC_IOC_MAGIC, 0x3, void *)
#define OIC_GET_FPGA_BD_STARTUP	     	_IOWR(OIC_IOC_MAGIC, 0x4, void *)
#define OIC_SET_FPGA_SYNCTIME	     	_IOWR(OIC_IOC_MAGIC, 0x5, void *)
#define OIC_SET_FPGA_BD_CFGINFO	     	_IOWR(OIC_IOC_MAGIC, 0x6, void *)
#define OIC_GET_FPGA_BD_CFGINFO	     	_IOWR(OIC_IOC_MAGIC, 0x7, void *)
#define OIC_SET_FPGA_BD_CFGINFO_EX  	_IOWR(OIC_IOC_MAGIC, 0x8, void *)
#define OIC_GET_FPGA_BD_CFGINFO_EX  	_IOWR(OIC_IOC_MAGIC, 0x9, void *)
#define OIC_GET_FPGA_BD_RUNINFO	        _IOWR(OIC_IOC_MAGIC, 0xa, void *)
#define OIC_SET_FPGA_64K_CH_TRAN        _IOWR(OIC_IOC_MAGIC, 0xb, void *)
#define OIC_SET_FPGA_2M_CH_TRAN	        _IOWR(OIC_IOC_MAGIC, 0xc, void *)
#define OIC_SET_FPGA_2M_CH_VALID        _IOWR(OIC_IOC_MAGIC, 0xd, void *)
#define OIC_GET_FPGA_2M_CH_VALID        _IOWR(OIC_IOC_MAGIC, 0xe, void *)
#define OIC_SET_FPGA_PL_CFGCH	        _IOWR(OIC_IOC_MAGIC, 0xf, void *)
#define OIC_SET_FPGA_PL_RSTCH	        _IOWR(OIC_IOC_MAGIC, 0x10, void *)
#define OIC_GET_FPGA_PL_MATCH	        _IOWR(OIC_IOC_MAGIC, 0x11, void *)
#define OIC_GET_FPGA_IN_STAT	        _IOWR(OIC_IOC_MAGIC, 0x12, void *)
#define OIC_SET_FPGA_PL_MODE	        _IOWR(OIC_IOC_MAGIC, 0x13, void *)
#define OIC_SET_FPGA_PL_SLOT	        _IOWR(OIC_IOC_MAGIC, 0x14, void *)
#define OIC_SET_FPGA_PL_AGE_CHANNEL     _IOWR(OIC_IOC_MAGIC, 0x15, void *)
#define OIC_GET_FPGA_IS_SIGCH	        _IOWR(OIC_IOC_MAGIC, 0x16, void *)
#define OIC_SET_FPGA_SILENCE_RANGE      _IOWR(OIC_IOC_MAGIC, 0x17, void *)
#define OIC_GET_FPGA_SILENCE_RANGE      _IOWR(OIC_IOC_MAGIC, 0x18, void *)
#define OIC_GET_FPGA_SILENCE_RESULT     _IOWR(OIC_IOC_MAGIC, 0x19, void *)
#define OIC_SET_FPGA_SILENCE_TOUT       _IOWR(OIC_IOC_MAGIC, 0x1a, void *)
#define OIC_GET_FPGA_SILENCE_TOUT       _IOWR(OIC_IOC_MAGIC, 0x1b, void *)
#define OIC_GET_FPGA_BD_RUNINFO_EX      _IOWR(OIC_IOC_MAGIC, 0x1c, void *) 
#define OIC_GET_FPGA_SLINK_INFO         _IOWR(OIC_IOC_MAGIC, 0x1d, void *) 
#define OIC_SET_FPGA_SLINK_INFO_START   _IOWR(OIC_IOC_MAGIC, 0x1e, void *) 
#define OIC_SET_FPGA_SLINK_INFO_END     _IOWR(OIC_IOC_MAGIC, 0x1f, void *) 
#define OIC_SET_FPGA_TRY_GROUP          _IOWR(OIC_IOC_MAGIC, 0x20, void *) 
#define OIC_SET_FPGA_GROUP              _IOWR(OIC_IOC_MAGIC, 0x21, void *) 
#define OIC_GET_FPGA_TRY_RESULT         _IOWR(OIC_IOC_MAGIC, 0x22, void *) 
#define OIC_GET_FPGA_GROUP              _IOWR(OIC_IOC_MAGIC, 0x23, void *) 
#define OIC_GET_FPGA_TRY_GROUP			_IOWR(OIC_IOC_MAGIC, 0x24, void *) 
#define OIC_SET_FPGA_HDLC_CFG          	_IOWR(OIC_IOC_MAGIC, 0x25, void *)
#define OIC_GET_FPGA_HDLC_CHSTAT		_IOWR(OIC_IOC_MAGIC, 0x26, void *)
#define OIC_GET_FPGA_HDLC_LOCAL_SCHAN   _IOWR(OIC_IOC_MAGIC, 0x27, void *)
#define OIC_GET_FPGA_HDLC_CONFIG_SCHAN  _IOWR(OIC_IOC_MAGIC, 0x28, void *)
#define OIC_GET_FPGA_HDLC_GE_STAT       _IOWR(OIC_IOC_MAGIC, 0x29, void *)
#define OIC_GET_FPGA_HDLC_DB_STATUS     _IOWR(OIC_IOC_MAGIC, 0x2a, void *)
#define OIC_SET_GFP_FPGA_SPU_SELNUM             _IOWR(OIC_IOC_MAGIC, 0x2b, void *)
#define OIC_GET_GFP_FPGA_SPU_SELNUM             _IOWR(OIC_IOC_MAGIC, 0x2c, void *)
#define OIC_SET_GFP_FPGA_SPU_FORWARD_ENABLE     _IOWR(OIC_IOC_MAGIC, 0x2d, void *)
#define OIC_SET_GFP_FPGA_SPU_FORWARD_DISABLE    _IOWR(OIC_IOC_MAGIC, 0x2e, void *)
#define OIC_SET_GFP_FPGA_SPU_FORWARD_RULE       _IOWR(OIC_IOC_MAGIC, 0x2f, void *)
#define OIC_SET_GFP_FPGA_SPU_CHANNEL_FORWARD    _IOWR(OIC_IOC_MAGIC, 0x30, void *)

/* sdh network-management */
#define OIC_SET_SDH_LOCAL_RULE					_IOWR(OIC_IOC_MAGIC, 0x31, void *)
#define OIC_SET_SDH_GLOBAL_RULE					_IOWR(OIC_IOC_MAGIC, 0x32, void *)
#define OIC_SET_SDH_INNER_RULE                  _IOWR(OIC_IOC_MAGIC, 0x33, void *)
#define OIC_SET_SDH_SET_SEL						_IOWR(OIC_IOC_MAGIC, 0x34, void *)
#define OIC_SET_SDH_SET_E1USER                  _IOWR(OIC_IOC_MAGIC, 0x35, void *)
#define OIC_SET_SDH_SET_E1LINKMAP               _IOWR(OIC_IOC_MAGIC, 0x36, void *)
#define OIC_SET_SDH_GET_BOARD_INFO              _IOWR(OIC_IOC_MAGIC, 0x37, void *)
#define OIC_SET_SDH_GET_PAYLOAD_INFO            _IOWR(OIC_IOC_MAGIC, 0x38, void *)
#define OIC_SET_SDH_GET_BOARD_STAT              _IOWR(OIC_IOC_MAGIC, 0x39, void *)
#define OIC_GET_FPGA_SLINK_INFO_HP				_IOWR(OIC_IOC_MAGIC, 0x3a, void *)
#define OIC_GET_SDH_SET_BOARD_STAT_TYPE			_IOWR(OIC_IOC_MAGIC, 0x3b, void *)
#define OIC_SET_SDH_SET_E164K                   _IOWR(OIC_IOC_MAGIC, 0x3c, void *)
#define OIC_SET_SDH_SET_BOARD_INFO				_IOWR(OIC_IOC_MAGIC, 0x3d, void *)
#define OIC_SET_SDH_GET_E1_INFO					_IOWR(OIC_IOC_MAGIC, 0x3e, void *)
#define OIC_GET_SDH_GET_AU4_STATUS              _IOWR(OIC_IOC_MAGIC, 0x3f, void *)
#define OIC_SET_SDH_SET_E1GFP                   _IOWR(OIC_IOC_MAGIC, 0x40, void *)
#define OIC_GET_SDH_CHANNEL2GLOBAL               _IOWR(OIC_IOC_MAGIC, 0x41, void *)
#define OIC_IOC_NUMBER							0x42

struct spu_forward_rule{
    unsigned char src_port;    
    unsigned char src_channel;    
    unsigned char dst_port;    
    unsigned char dst_selnum;  
};

struct chan_flag{
    unsigned char chan;
    unsigned int flag;
};

struct fpga_verinfo {
	unsigned char	version;
	unsigned int	date;
};

struct fpga_time_info {
	unsigned int	sec;
	unsigned int	usec;
};

#define FMASK_DSTMAC    (1 << 0)
#define FMASK_SRCMAC    (1 << 1)
#define FMASK_ETHERTYPE (1 << 2)
#define FMASK_SLOT      (1 << 3)
#define FMASK_DSTIP     (1 << 4)
#define FMASK_SRCIP     (1 << 5)
#define FMASK_DSTPORT   (1 << 6)
#define FMASK_SRCPORT   (1 << 7)
#define FMASK_DEVID     (1 << 8)
struct fpga_board_cfginfo {
	unsigned char	dstmac[6];
	unsigned char	srcmac[6];
	unsigned short	ethertype;
	unsigned char	slot;
	unsigned int 	fmask;
};

struct fpga_board_cfginfo_ex {
	unsigned char	dstmac[6];
	unsigned char	srcmac[6];
	unsigned int	dstip;
	unsigned int	srcip;
	unsigned short	dstport;
	unsigned short	srcport;
	unsigned char	slot;
	unsigned int	devid;		
	unsigned int 	fmask;
};

#define FPGA_OI_PORTNUM	4
struct fpga_board_runinfo_port {
	unsigned char	los;
	unsigned char	stm1_synced;
	unsigned char	e1_synced_num;
	unsigned int	ch_64k_num;
	unsigned int	ch_64k_frames;
	unsigned int	ch_2m_num;
	unsigned int	ch_2m_frames;
};

struct fpga_board_runinfo {
	struct fpga_board_runinfo_port	ports[FPGA_OI_PORTNUM];
	unsigned int			traffic;
};

/* vc4(155M)/c4 info */
struct fpga_board_runinfo_port_ex {
    unsigned char	los;             //'1': los '0':no los
    unsigned char	stm1_synced;    //'1': stm1 sync, no lof. '0':stm1 unsynced,have lof.
    unsigned char   phy_type; //'0': 155M '1':622M '2':2.5G '3': 10G
	unsigned short  b1_cnt;
	unsigned short  b2_cnt;
	unsigned short  b3_cnt;
	unsigned short  auptr_val;
	unsigned char   auptr_0110_rev;
	unsigned char   auptr_0110_no;
	unsigned char   auptr_1001_rev;
	unsigned char   auptr_1001_no;
	unsigned char   c2_val; // 见文档
    unsigned char   chan_rate; //高阶：输入端口速率 低阶：155M. '1':c4 '2':c12 '3':c11
	unsigned char   e1_cnt;
    unsigned char   nfm_e1_cnt; 
    unsigned int    ch_64k_num;
    unsigned int    ch_64k_frames;
    unsigned int    ch_2m_num;
	unsigned int	ch_2m_frames;

	unsigned char fiber;      /* port number */
	unsigned char channel;
	// 高阶：Reg 0x64 = 0x09 : Reg0x65-0x68 参见[1.3 6) ]； bit 28    channel_rate == 1
	unsigned char vc_valid; /*  高阶：VC4_sync; 0-invalid, 1-valid */
    //高阶：Reg 0x64 = 0x09 : Reg0x65-0x68 参见[1.3 6) ]；bit 27 : 24 channel_rate == 1
	unsigned char is_lcas;  /* Lcas协议控制字段 2/3填1; 否则填0 */
	unsigned char is_member; /* Lcas协议控制字段 2/3填1；否则填0 */
	unsigned char is_last_member; /* Lcas协议控制字段 3填1；否则填0 */
	//高阶：Reg 0x64 = 0x09 : Reg0x65-0x68 参见[1.3 6) ]；bit 19 : 8 channel_rate == 1
	unsigned short mfi;
	//高阶：Reg 0x64 = 0x09 : Reg0x65-0x68 参见[1.3 6) ]；bit 7 : 0 channel_rate == 1
	unsigned char sq;
	//高阶：Reg 0x64 = 0x0a : Reg0x65-0x68 参见[1.3 7) ]；bit 31 : 0 channel_rate == 1
	unsigned int pre_gid;
	//高阶：Reg 0x64 = 0x0b : Reg0x65-0x68 参见[1.3 7) ]；bit 31 : 0 channel_rate == 1
	unsigned int cur_gid;
	//高阶：Reg 0x64 = 0x09 : Reg0x65-0x68 参见[1.3 6) ]；bit 23 : 20 channel_rate == 1
	unsigned int svc_type; /* 1-GFP/2-LAPS/3-PPP(pos)/4-ATM/5-TUG/6-HDLC/7-DDN/0-OTH */
    //高阶：sync, sq, mfi不连续错误统计
	unsigned short  sync_cnt;
	unsigned char   sq_cnt;
	unsigned char   mfi_cnt;
    //高阶：J1 是128bit的值，j1_0是低0-31bit,依次类推，j1_3是96-127bit
    unsigned int j1_0;
    unsigned int j1_1;
    unsigned int j1_2;
    unsigned int j1_3;
};

struct fpga_board_runinfo_ex {
    unsigned int start_port;
	struct fpga_board_runinfo_port_ex	ports[FPGA_OI_PORTNUM];
    unsigned char   clear; //"0": no "1": clear
};

#define FPGA_OI_CHANNUM		63
/* c12 info */
struct slink_info {
	// 输入的port
	unsigned char fiber;      /* port number */

	// Reg 0x64 = 0x04 : Reg0x65-0x68 参见[1.3 2) ]
	unsigned char fiber_rate; /* 0-155M/1-622M/2-2.5G/3-10G ; Reg 0x64 = 0x04: [1.3 2) ]*/

	// 输入的通道号, 每次获取一个端口的一个通道的链路信息
	unsigned char channel;

	// Reg 0x64 = 0x04 : Reg0x65-0x68 参见[1.3 2) ]
	unsigned char channel_rate; /* 1-C4/2-C12/3-C11; Reg 0x64 = 0x04: [1.3 2) ] */

	//低阶：Reg 0x64 = 0x05 : Reg0x65-0x68 参见[1.3 3) ]； bit 29-28
	unsigned char vc_valid; /* 低阶：v5_sync和k4_sync 同时为1有效; 高阶：VC4_sync; 0-invalid, 1-valid */

	//低阶：Reg 0x64 = 0x05 : Reg0x65-0x68 参见[1.3 3) ]；bit 27 : 24
	unsigned char is_lcas;  /* Lcas协议控制字段 2/3填1; 否则填0 */
	unsigned char is_member; /* Lcas协议控制字段 2/3填1；否则填0 */
	unsigned char is_last_member; /* Lcas协议控制字段 3填1；否则填0 */

	//低阶：Reg 0x64 = 0x05 : Reg0x65-0x68 参见[1.3 3) ]；bit 19 : 8
	unsigned short mfi;
	//低阶：Reg 0x64 = 0x05 : Reg0x65-0x68 参见[1.3 3) ]；bit 7 : 0
	unsigned char sq;
	//低阶：Reg 0x64 = 0x06 : Reg0x65-0x68 参见[1.3 4) ]；bit 31 : 0
	unsigned int pre_gid;
	//低阶：Reg 0x64 = 0x07 : Reg0x65-0x68 参见[1.3 4) ]；bit 31 : 0
	unsigned int cur_gid;
	//低阶：Reg 0x64 = 0x05 : Reg0x65-0x68 参见[1.3 3) ]；bit 23 : 20
	unsigned int svc_type; /* 1-GFP/2-LAPS/3-PPP/4-ATM/0-OTH */

	unsigned char   tuptr_0110_rev; /*tuptr_0110_反转*/
	unsigned char   tuptr_0110_no; /*tuptr_0110_不反转*/
	unsigned char   tuptr_1001_rev; /*tuptr_1001_反转*/
    unsigned char   tuptr_1001_no; /*tuptr_1001_不反转*/
    unsigned char   tuptr_val;
    unsigned char   v5_val;
    unsigned short  v5_cnt;
    //低阶：v5,k4不连续错误统计
	unsigned char   v5_sync_cnt;
	unsigned char   k4_sync_cnt;

	unsigned char   sq_cnt;
	unsigned char   mfi_cnt;
	unsigned char   e1_sync;            //成帧e1_sync
	unsigned char   e1_sync_err;        //成帧e1_sync_err统计值
	unsigned char   nfm_e1_sync;        //非成帧e1_sync
	unsigned char   nfm_e1_sync_err;    //非成帧e1_sync_err统计值
};

#if defined(IN_X86)
#define TRY_GROUP_MAXNUM 512
#define SET_GROUP_MAXNUM 4096
#define TRY_LINK_MAXNUM	 512
#define SET_LINK_MAXNUM	 4096
#else
#define TRY_GROUP_MAXNUM 256
#define SET_GROUP_MAXNUM 256
#define TRY_LINK_MAXNUM	 256
#define SET_LINK_MAXNUM	 256
#endif

#define TRY_NO_RESULT 1
struct sgroup_info {
    int group_id;
	int flag;
	int is_valid; /* 0-invalid; 1-valid */

	int linkarrays_size;
	struct slink_info *linkarrays;

	unsigned char resokpkt;
	unsigned char reserrpkt;
};

#define GE_NUM_MAX      4
struct ge_stat {
    int ge;
    unsigned int tran;
    unsigned int tran_clr;
    unsigned int hdlc;
    unsigned int hdlc_clr;
};

struct bd_ge_stat {
	struct ge_stat gstat[GE_NUM_MAX];
    unsigned char clear; //"1": clear "0": no 
};

struct hdlc_chan_stat {
    unsigned int port;
    unsigned int type;
    unsigned int match_num;
    unsigned int e1;
    unsigned int ts;
    unsigned int ok_pkts;
    unsigned int err_pkts;
    unsigned int n_7e;
};

#define CHANNEL_NUM_MAX     8192
struct hdlc_port_stat {
    unsigned int num_chstats;
    struct hdlc_chan_stat chstat[CHANNEL_NUM_MAX];
};

#define  HDLC_CFG_LEN         (1 << 0)
#define  HDLC_CFG_SCAN_MODE   (1 << 1)
struct hdlc_cfg {
    unsigned int    mask;
    unsigned int    len_max;
    unsigned int    len_min;
    unsigned char   scan_mode; //'1': local '2': config
};

struct ram_info {
	unsigned int ram1[256];
	unsigned char ram2[256];
	unsigned int ram1_c4[256];
	unsigned int ram1_c12[256];
	unsigned char ram2_c4[256];
	unsigned char ram2_c12[256];
};

#define KMALLOC_SIZE         (30 * 1024) 
struct sgroup_all_info {
    int gsize;
	int port_group;
    struct sgroup_info *ginfo;
};

struct fpga_trans_info {
	unsigned short	channel;
	unsigned char	valid;
};
struct fpga_set_age {
	unsigned char flag;
};

struct fpga_pl_tbl_info {
	unsigned char slot;
	unsigned char subslot;
};

struct fpga_cfg {
	unsigned int index;
	unsigned char en;
	unsigned char ts;
	unsigned char mode;
	unsigned char flag;
};


struct fpga_au_stat {
	unsigned short	auptr;
	unsigned short	aundf0;	/* auptr: 0110 */
	unsigned short	aundf1;	/* auptr: 1001 */
};

struct fpga_is_sigch {
	unsigned int sig[4][63]; 
};

#define FPGA_OI_E1NUM	63
struct fpga_v5_stat {
	unsigned short	v5_ecode[FPGA_OI_E1NUM];
	unsigned short	v5ptr[FPGA_OI_E1NUM];
	unsigned short	v5ndf0[FPGA_OI_E1NUM];
	unsigned short	v5ndf1[FPGA_OI_E1NUM];
};

struct fpga_e1_stat {
	unsigned char	e1_sync[FPGA_OI_E1NUM];
	unsigned short	e1_sync_ecode[FPGA_OI_E1NUM];
	unsigned char	e1_change;
	unsigned char	e1_speed_diff[FPGA_OI_E1NUM];
};

struct fpga_in_stat {
	unsigned short	b1_ecode[FPGA_OI_PORTNUM];
	unsigned short	b2_ecode[FPGA_OI_PORTNUM];
	unsigned short	b3_ecode[FPGA_OI_PORTNUM];

	struct fpga_au_stat auptr[FPGA_OI_PORTNUM];

	struct fpga_v5_stat	v5[FPGA_OI_PORTNUM];

	struct fpga_e1_stat e1[FPGA_OI_PORTNUM];
};

struct silence_result {
    unsigned short ch;
    unsigned char stat;
};

/* sdh network-management */
#define FPGA_VERSION_HP				1
#define FPGA_VERSION_LP				2
#define LOCAL_OUTPUT_PORT_NUM		4
#define SEL_LIST_NUM         		16

#define LIST_TYPE_HP_LOCAL			0x00
#define LIST_TYPE_LP_LOCAL			0x04
#define LIST_TYPE_HP_GLOBLA			0x01
#define LIST_TYPE_LP_GLOBLA			0x05
#define LIST_TYPE_HP_TO_LP_SEL		0x02
#define LIST_TYPE_HP_TO_LP_AU4		0x03
#define LIST_TYPE_LP_SEL	  		0x06		/* list : linkmap selA */
#define LIST_TYPE_LP_E1USER	  		0x07
#define LIST_TYPE_LP_DEVMAP	  		0x08
#define LIST_TYPE_LP_64KPPP_SEL  	0x09		/* list : 64kppp selA */
#define LIST_TYPE_LP_LOCAL_E1USER	0x0a
#define LIST_TYPE_LP_LOCAL_LM		0x0b
#define LIST_TYPE_LP_LOCAL_64KPPP  	0x0c
#define LIST_TYPE_LP_GLOBLA_E1USER	0x0d
#define LIST_TYPE_LP_GLOBLA_LM		0x0e
#define LIST_TYPE_LP_GLOBLA_64KPPP  0x0f

#define LIST_TYPE_LP_E1GFP_USER	  	0x20		/* e1 gfp user list type */
#define LIST_TYPE_LP_E1GFP_DEVMAP	0x21		/* e1 gfp devmap list type */
#define LIST_TYPE_LP_LOCAL_E1GFP    0x22		/* e1 gfp local */
#define LIST_TYPE_LP_GLOBAL_E1GFP   0x23		/* e1 gfp global */

struct sdh_local_rule {
    unsigned char proto;
    unsigned char valid_num;
    unsigned char sel[SEL_LIST_NUM];        /* bit0-bit3 '1': valid '0': unvalid */
};

struct sdh_global_rule {
    unsigned char proto;
    unsigned char valid_num;
    unsigned char sel[SEL_LIST_NUM];
};

struct sdh_inner_rule {
	unsigned char src_port;
	unsigned char src_channel;
	unsigned char enable;					//'1':enable '0':disable
	unsigned char dst_sel;
	unsigned char dst_channel;
};

struct sdh_sel {
	unsigned char fpgaid;
	unsigned char sel;
};

#define E1_RULE_LOCAL		1
#define E1_RULE_GLOBAL		2
struct sdh_e1user_map {
    unsigned char au4;
    unsigned char e1;
	unsigned char rule;
	unsigned char sel;
    unsigned char enable;
	unsigned char user_chassisid;
    unsigned char user_slot;
    unsigned char user_port;
    unsigned char user_e1;
};

struct sdh_e1linkmap_map {
	unsigned char  lmp_sel;
	struct sdh_e1user_map map;
};

struct sdh_e164k_map{
	unsigned char  e164k_sel;
	struct sdh_e1user_map map;
};

struct sdh_e1gfp_map {
    unsigned char e1gfp_sel;
    struct sdh_e1user_map map;
};

#define SDH_BOARD_STAT_TYPE_CLEAR   1
#define SDH_BOARD_STAT_TYPE_PKTS    2
#define SDH_BOARD_STAT_TYPE_BYTES   3

struct sdh_fpga_stat {
	unsigned long task_p;
	unsigned long ch_err;
	unsigned long gfp_ok;
	unsigned long gfp_we_err;
	unsigned long gfp_fisu;
	unsigned long atm_ok;
	unsigned long atm_we_err;
	unsigned long atm_fisu;
	unsigned long ppp_ok;
	unsigned long ppp_we_err;
	unsigned long ppp_fisu;
	unsigned long gfp_los;
	unsigned long ppp_los;
	unsigned long atm_los;
	unsigned long gfp_cnt;
	unsigned long atm_cnt;
	unsigned long ppp_cnt;
	unsigned long e1user_cnt;
	unsigned long e164k_ppp_cnt;
	unsigned long e1lm_cnt;
	unsigned long hp2lp_cnt;

	unsigned long laps_ok;
	unsigned long laps_we_err;
	unsigned long laps_fisu;
	unsigned long laps_cnt;
	unsigned long laps_los;

	unsigned long local_eth0;
	unsigned long local_eth1;
	unsigned long local_eth2;
	unsigned long local_eth3;
	unsigned long sw_eth0;
	unsigned long sw_eth1;
	unsigned long sw_eth2;
	unsigned long sw_eth3;

	unsigned long gfp_tx_cnt;
	unsigned long atm_tx_cnt;
	unsigned long ppp_tx_cnt;
	unsigned long e3_tx_cnt;
	unsigned long lap_tx_cnt;
	unsigned long cx_rx_cnt;
	unsigned long e3_rx_cnt;
	unsigned long rx_los_cnt;
};

struct sdh_fpga_info {
	unsigned char fpgaid;
	unsigned int version;
	unsigned char type;
	unsigned char subtype;
	unsigned char status;
	unsigned char chassisid;
	unsigned char slot;
	unsigned char physlot;
	unsigned char date[4];
};

struct e1_info {
	unsigned int  e1_cnt;
	unsigned int  m64k_cnt;
	unsigned int  m2m_cnt;
	unsigned int  m2m_nfm_cnt;
};

struct gfp_local2global {
	unsigned int local_au4;
	unsigned int local_e1;
	unsigned int chassisid;
	unsigned int slot;
	unsigned int port;
	unsigned int au4;
	unsigned int e1;
};

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_OIC_12720691_154A4884_54CE5B33_H */
