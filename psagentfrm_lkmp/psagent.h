/*
 * (C) Copyright 2017
 * zhangqizhi <qizhi.zhang@raycores.com>
 *
 * psagent.h - A description goes here.
 *
 */

#ifndef _HEAD_PSAGENT_7D3E55FB_015F05C8_1B940D0B_H
#define _HEAD_PSAGENT_7D3E55FB_015F05C8_1B940D0B_H

#include "adapter.h"
#include "adapter_cs.h"
#include "psp.h"

#define PSAGENT_DEV_PORT_NUM		16

struct psp_ops {
	void *(*open)(void);
	int (*close)(void *mif);
	int (*register_msg)(void *adap, void *mif, struct psp_msg *msg);
	int (*register_ack_msg)(void *adap, void *mif, struct psp_msg *msg);
	int (*scan_clear)(void *adap, void *mif, struct psp_msg *msg);
	int (*notify_anm)(void *adap, void *mif, struct psp_msg *msg);
	int (*notify_rel)(void *adap, void *mif, struct psp_msg *msg);
	int (*get_phy_stat)(void *adap, void *mif, struct psp_msg *msg);
	int (*get_channel_stat)(void *adap, void *mif, struct psp_msg *msg);
	int (*clear_index)(void *adap, void *mif, struct psp_msg *msg);
	int (*vchannel_start)(void *adap, void *mif, struct psp_msg *msg);
	int (*vchannel_stop)(void *adap, void *mif, struct psp_msg *msg);
    int (*clear_all)(void *adap, void *mif, struct psp_msg *msg);
	int (*check_phy)(void *adap, void *mif, struct psp_msg *msg);
	int (*reset_pointer)(void *adap, void *mif, struct psp_msg *msg);
	int (*tran_enable)(void *adap, void *mif, struct psp_msg *msg);
	int (*get_dial_status)(void *adap, void *mif, struct psp_msg *msg);
};

#define GET_CHANNEL_BUF_SIZE      9216  /*9K*/
struct psagent_get_chan_stat {
    unsigned char type;
    unsigned int e1_ts[63];
};

/* psagent glb data */
struct psagent_glb_data {
		int first;
		int phy[4]; //phy status loslof
		int e1[4][64]; //e1 vaild of one port
} glb_data;

#define LM_VERSION_NORMAL	1
#define LM_VERSION_SIMPLE	2
/* psagent glb info */
struct psagent_glb {
    char probetype;
    char cardtype;
    char slot; /*ATCA: slot. oipc: devnum*/
    char subslot;
	char check_phy_status; /* '1': on  '0': off*/
	char check_interval;
	char lm_version; /* '1': linkmap version normal '2': linkmap version simple */
	int  ps_port_num;
};
struct psagent_glb psglb;

/* send/recv pkts stat */
struct st_pkt_stat {
    /*recv pkts*/
	unsigned long long rpkts;
	unsigned long long register_ack_cnt;
	unsigned long long scan_clear;
	unsigned long long notify_anm;
	unsigned long long notify_rel;
	unsigned long long get_phy_stat;
	unsigned long long get_channel_stat;
	unsigned long long miss;
	unsigned long long clear_index;
	unsigned long long vch_start;
	unsigned long long vch_stop;
	unsigned long long tran_en;
	unsigned long long clear_all_index;
	unsigned long long reset_pointer;
        
    /*send pkts*/
	unsigned long long spkts;
	unsigned long long register_cnt;
	unsigned long long scan_clear_ack;
	unsigned long long get_phy_stat_ack;
	unsigned long long get_channel_stat_ack;
	unsigned long long clear_index_ack;
	unsigned long long check_phy_cnt;
};

struct st_pkt_stat pkt_stat;

#define SEND_PKTS(adap,buf,len,spkts)        \
	if (adapter_write(adap,buf,len) <= 0) {  \
		LOGERROR("failed to adapter_write.");  \
	} else {                                   \
		spkts++;                                \
	}

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PSAGENT_7D3E55FB_015F05C8_1B940D0B_H */
