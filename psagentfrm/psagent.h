/*
 * (C) Copyright 2017
 * zhangqizhi <qizhi.zhang@raycores.com>
 *
 * psagent.h - A description goes here.
 *
 */

#ifndef _HEAD_PSAGENT_7D3E55FB_015F05C8_1B940D0B_H
#define _HEAD_PSAGENT_7D3E55FB_015F05C8_1B940D0B_H

#include "psp.h"

struct silence_link {
	unsigned char link;
	unsigned char ts;
	unsigned char link1;
	unsigned char ts1;

	void *timer;
};

struct psp_ops {
	void *(*open)(void);
	int (*close)(void *mif);
	int (*scan_init)(void *adap, void *mif, struct psp_msg *msg);
	int (*scan_start)(void *adap, void *mif, struct psp_msg *msg);
	int (*scan_stop)(void *adap, void *mif, struct psp_msg *msg);
	int (*probe_link_enable)(void *adap, void *mif, struct psp_msg *msg);
	int (*probe_link_disable)(void *adap, void *mif, struct psp_msg *msg);
	int (*probe_channel_start)(void *adap, void *mif, struct psp_msg *msg);
	int (*probe_channel_stop)(void *adap, void *mif, struct psp_msg *msg);
	int (*probe_allch_start)(void *adap, void *mif, struct psp_msg *msg);
	int (*probe_allch_stop)(void *adap, void *mif, struct psp_msg *msg);
	int (*get_stat)(void *adap, void *mif, struct psp_msg *msg);
	int (*set_silence_timeout)(void *adap, void *mif, struct psp_msg *msg);
	int (*set_silence_range)(void *adap, void *mif, struct psp_msg *msg);
	int (*get_silence_result)(void *mif, void *data);
	int (*get_phy_stat)(void *adap, void *mif, struct psp_msg *msg);
};

struct st_pkt_stat {
	/*recvice Packet stat*/
	unsigned long long scan_init;
	unsigned long long scan_start;
	unsigned long long scan_stop;
	unsigned long long link_en;
	unsigned long long link_dis;
	unsigned long long ch_start;
	unsigned long long ch_stop;
	unsigned long long allch_start;
	unsigned long long allch_stop;
	unsigned long long miss;
	unsigned long long get_phy_stat;
	/*send Packet stat*/
	unsigned long long spkts;
	unsigned long long get_phy_stat_ack;
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

int psagent_is_gsm(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PSAGENT_7D3E55FB_015F05C8_1B940D0B_H */
