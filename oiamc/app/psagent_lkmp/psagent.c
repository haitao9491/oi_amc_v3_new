/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * psagent.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "os.h"
#include "aplog.h"
#include "coding.h"
#include "list.h"
#include "adapter.h"
#include "adapter_cs.h"
#include "mutex.h"
#include "psp.h"
#include "fpgamif.h"
#include "psagent.h"

static int psagent_register_msg(void *adap, void *mif, struct psp_msg *msg)
{
    unsigned char buf[64];
    unsigned char *p = NULL;
    unsigned short device = 0;
    int len = 0;
    unsigned char mask = 0;

    mask = 0x02;
	if (fpgamif_set_fpga_scan_clear(mif, &mask) != 0) {
		printf("set fpga scan clear 0x%x failed.\n", mask);
		return -1;
    }

	len = sizeof(pkt_hdr) + 4;
    memset(buf, 0, sizeof(buf));
    pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_REGISTER >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_REGISTER >> 0) & 0xff);
    device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
    pkthdr_set_device((pkt_hdr *)buf, device);
    pkthdr_set_channel((pkt_hdr *)buf, 0);
	p = buf + sizeof(pkt_hdr);
    CODING_8(p, psglb.probetype);
    CODING_8(p, psglb.cardtype);
    CODING_8(p, psglb.slot);
    CODING_8(p, psglb.subslot);

    SEND_PKTS(adap, buf, len, pkt_stat.spkts);

    return 0;
}

/* set fpga clear all indexs */
static int psagent_clear_all(void *adap, void *mif, struct psp_msg *msg)
{
    unsigned char mask = 0;

    mask = 0x02;
	if (fpgamif_set_fpga_scan_clear(mif, &mask) != 0) {
		printf("set fpga clear all 0x%x failed.\n", mask);
		return -1;
    }

    return 0;
}

#define SILENCETV 0x01
#define SILENCETL 0x02
#define SILENCEVALUE 0x54

static int psagent_register_ack_msg(void *adap, void *mif, struct psp_msg *msg)
{
    struct fpga_silence sn;
	unsigned char silencetype = 0;
	unsigned int indexlimit = 0;
    struct tran_en tran;

    memset(&tran, 0, sizeof(struct tran_en));
    tran.revert = msg->rg.revert;
    tran.en = msg->rg.tranen;
    LOG("register_ack_msg : tran revert %d enable %d", tran.revert, tran.en);
	if (fpgamif_set_fpga_tran_en(mif, &tran) != 0) {
        LOGERROR("%d: fpgamif_get_fpga_tran_en Failed.", __LINE__);
		return -1;
    }

	silencetype = msg->rg.silencetype;
	indexlimit = msg->rg.indexlimit;
    if ((indexlimit > 2048) || (indexlimit == 0)) {
        LOGNOTICE("indexlimit %d is err, set default limit value 2047.", indexlimit);
		indexlimit = 2047;
    } else {
        indexlimit = (msg->rg.indexlimit - 1);
        LOGDEBUG("psagent_register_ack_msg-->index: %u silencetype: %u ", indexlimit, silencetype);
    }

    memset(&sn, 0, sizeof(&sn));
	/* To do set fpag */
    if (silencetype == SILENCETV) {
        sn.smin0 = SILENCEVALUE;
        sn.smax0 = SILENCEVALUE;
        sn.smin1 = SILENCEVALUE;
        sn.smax1 = SILENCEVALUE;
    } else if (silencetype == SILENCETL) {
        sn.smin0 = 0x50;
        sn.smax0 = 0x57;
        sn.smin1 = 0xD0;
        sn.smax1 = 0xD7;
    } else {
        LOGERROR("register_ack_msg silencetype %d err.", silencetype);
    }

	if (fpgamif_set_fpga_indexlimit(mif, &indexlimit) != 0) {
        LOGERROR("%d: fpgamif_get_fpga_indexlimit Failed.", __LINE__);
		return -1;
    }

	if (fpgamif_set_fpga_silence(mif, &sn) != 0) {
        LOGERROR("%d: fpgamif_get_fpga_silence Failed.", __LINE__);
		return -1;
    }
    return 0;
}

static int psagent_scan_clear(void *adap, void *mif, struct psp_msg *msg)
{
    unsigned short device = 0;
    unsigned char mask = 0;
    unsigned char buf[64];
    int len = 0;

    mask = msg->scan.clear_mask;
    if (mask < 0x01 || mask > 0x03) {
        LOGERROR("scan(start event): failed to set [0x%x] clear. help-->[1]:clear sig [2]: clear voice [3]: clear sig,voice", msg->scan.clear_mask);
		return -1;
    }

    if (fpgamif_set_fpga_scan_clear(mif, &mask) != 0) {
        LOGERROR("fpgamif_set_fpga_scan_clear is failed.");
        return -1;
    }

    /*do send PSP_MSG_SCAN_CLEAR_ACK*/
	len = sizeof(pkt_hdr);
    memset(buf, 0, sizeof(buf));
    pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_SCAN_CLEAR_ACK >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_SCAN_CLEAR_ACK >> 0) & 0xff);
    device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
    pkthdr_set_device((pkt_hdr *)buf, device);
    pkthdr_set_channel((pkt_hdr *)buf, 0);

    SEND_PKTS(adap, buf, len, pkt_stat.spkts);
    pkt_stat.scan_clear_ack++;

	return 0;
}

static int psagent_notify_anm(void *adap, void *mif, struct psp_msg *msg)
{
    unsigned int index = 0;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

    index = msg->scan.index;
	if (fpgamif_set_fpga_anm(mif, index) != 0) {
		LOGERROR("notify(anm event): failed to cfg fpga anm: index %u", index);
	}

	return 0;
}

static int psagent_notify_rel(void *adap, void *mif, struct psp_msg *msg)
{
    unsigned int index = 0;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}
	
    index = msg->scan.index;
	if (fpgamif_set_fpga_rel(mif, index) != 0) {
		LOGERROR("notify(rel event): failed to cfg fpga rel: index %u", index);
        return -1;
	}

	return 0;
}

static int psagent_get_phy_stat(void *adap, void *mif, struct psp_msg *msg)
{
	struct sdh_phy_stat phy_stat[FPGADRV_OIAMC_PORT_MAX];
	struct e1_status e1_s;
    unsigned int e1vh = 0;
    unsigned int e1vl = 0;
    unsigned char buf[2048];
    unsigned char *p = NULL;
    unsigned short device = 0;
    unsigned int len = 0;
    int i = 0;
    int j = 0;

    memset(phy_stat, 0, sizeof(struct sdh_phy_stat) * FPGADRV_OIAMC_PORT_MAX);
	if (fpgamif_get_phy_status(mif, phy_stat) != 0) {
        LOGERROR("%d: fpgamif_get_phy_status-->Failed.", __LINE__);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
    len = sizeof(pkt_hdr) + 1 + (23 *  psglb.ps_port_num);
    pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_PHY_STAT_ACK >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_PHY_STAT_ACK >> 0) & 0xff);
    device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
    pkthdr_set_device((pkt_hdr *)buf, device);
    pkthdr_set_channel((pkt_hdr *)buf, 0);
    p = buf + sizeof(pkt_hdr);

    CODING_8(p, psglb.ps_port_num);
    for (i = 0; i < psglb.ps_port_num; i++)
    {
        memset(&e1_s, 0, sizeof(struct e1_status));
        e1_s.port = i;
        e1vh = 0;
        e1vl = 0;
		if (fpgamif_get_e1_vaild(mif, &e1_s) == 0)
        {
            for (j = 0; j < FPGADRV_OIAMC_E1_MAX; j++)
            {
                if (j < 32)
                {
                    if (e1_s.vaild[j] != 0)
                        e1vl |= (1 << j);
                }
                else
                {
                    if (e1_s.vaild[j] != 0)
                        e1vh |= (1 << (j - 32));
                }
            }
        }
        else
        {
            LOGERROR("%d: fpgamif_get_e1_vaild-->Failed.", __LINE__);
        }

        if (phy_stat[i].status & 0x02) {
            phy_stat[i].status = (phy_stat[i].status & 0xfd); 
        }
		CODING_8(p, phy_stat[i].status);
		CODING_8(p, phy_stat[i].phy_type);
		CODING_8(p, phy_stat[i].e1_cnt);
		CODING_16(p, phy_stat[i].chcnt_64k);
		CODING_16(p, phy_stat[i].chcnt_2m);
		CODING_32(p, phy_stat[i].fmcnt_64k);
		CODING_32(p, phy_stat[i].fmcnt_2m);
		CODING_32(p, e1vl);
		CODING_32(p, e1vh);
    }

    SEND_PKTS(adap, buf, len, pkt_stat.spkts);
	LGWRDEBUG(buf, len, "Send_PHY_STAT:");
    pkt_stat.get_phy_stat_ack++;
    return 0;
}

static int psagent_get_channel_stat(void *adap, void *mif, struct psp_msg *msg)
{
	struct get_hdlc_stat_to_file *stat = NULL;
    struct psagent_get_chan_stat chan[FPGADRV_OIAMC_PORT_MAX];
	char *buf = NULL;
    unsigned char *p = NULL;
    unsigned char port = 0;
    unsigned char type = 0;
    unsigned char e1 = 0;
    unsigned char ts = 0;
    unsigned short device = 0;
    int len = 0;
    int i = 0;
    int j = 0;

	stat = malloc(sizeof(struct get_hdlc_stat_to_file));
	if (stat == NULL) {
        goto err;
	}
	memset(stat, 0, sizeof(struct get_hdlc_stat_to_file));

    buf = malloc(GET_CHANNEL_BUF_SIZE);
	if (buf == NULL) {
        goto err;
	}
	memset(buf, 0, GET_CHANNEL_BUF_SIZE);

	memset(chan, 0, sizeof(struct psagent_get_chan_stat) * FPGADRV_OIAMC_PORT_MAX);

	if (fpgamif_get_all_hdlc_channel_stat(mif, stat) != 0) {
        LOGERROR("psagent get channel->fpgamif_get_all_hdlc_channel_stat failed.");
        goto err;
    }

	for (i = 0; i < stat->ch_vaild; i++) {
        port = stat->chstat[i].port;
        type = stat->chstat[i].type;
        e1 = stat->chstat[i].e1;
        ts = stat->chstat[i].ts;
        chan[port].type = type;  /*'0': 64k '1':2M*/
        chan[port]. e1_ts[e1] |= (1 << ts);
		LOGDEBUG("ch_vaild[%d]->port: %d type: 0x%x e1: 0x%x ts: 0x%x", i, stat->chstat[i].port, chan[port].type, e1, chan[port]. e1_ts[e1]);
    }

	len = sizeof(pkt_hdr) + 1 + (253 * psglb.ps_port_num);
    p = (unsigned char *)buf + sizeof(pkt_hdr);

	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_CHANNEL_STAT_ACK >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_CHANNEL_STAT_ACK >> 0) & 0xff);
    device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
	pkthdr_set_device((pkt_hdr *)buf, device);
	pkthdr_set_channel((pkt_hdr *)buf, 0);

    CODING_8(p, psglb.ps_port_num);
    for (i = 0; i < psglb.ps_port_num; i++)
    {
        CODING_8(p, chan[i].type);
        for (j = 0; j < FPGADRV_OIAMC_E1_MAX; j++)
            CODING_32(p, chan[i].e1_ts[j]);
    }

    SEND_PKTS(adap, buf, len, pkt_stat.spkts);
	LGWRDEBUG(buf, len, "Send_CHANNEL:");
    pkt_stat.get_channel_stat_ack++;

    free(stat);
    free(buf);

	return 0;
err:
    if (stat)
        free(stat);
    if (buf)
        free(buf);

    return -1;
}

int psagent_clear_index(void *adap, void *mif, struct psp_msg *msg)
{
	unsigned int index = 0;
    unsigned char buf[64];
    int len = 0;
    unsigned short device = 0;
    unsigned char *p = NULL;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}
	
    index = msg->clear.index;

    len = sizeof(pkt_hdr) + 2;
    memset(buf, 0, sizeof(buf));
    pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);

    device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
    pkthdr_set_device((pkt_hdr *)buf, device);
    pkthdr_set_channel((pkt_hdr *)buf, 0);

    LOGDEBUG("clear_index: %d", index);
	if (fpgamif_set_fpga_clear_index(mif, &index) != 0) {
		LOGERROR("fpgamif_set_fpga_clear_index: clear index[%d] Failed.", index);

        pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_CLEAR_INDEX_NACK >> 8) & 0xff);
        pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_CLEAR_INDEX_NACK >> 0) & 0xff);
	} else {
        pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_CLEAR_INDEX_ACK >> 8) & 0xff);
        pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_CLEAR_INDEX_ACK >> 0) & 0xff);
    }

    p = buf + sizeof(pkt_hdr);
    CODING_16(p, index);
    SEND_PKTS(adap, buf, len, pkt_stat.spkts);
    pkt_stat.clear_index_ack++;

	return 0;
}

static int psagent_vchannel_start(void *adap, void *mif, struct psp_msg *msg)
{
    struct chan_tran_cfg tran;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

    memset(&tran, 0, sizeof(struct chan_tran_cfg));
    tran.chan = msg->tran.chan;
    tran.port = msg->tran.port;
    tran.e1 = msg->tran.e1;
    tran.ts = msg->tran.ts;
    tran.en = 1;
    LOG("vchannel_start : chan %d port %d e1 %d ts %d ", tran.chan,tran.port,tran.e1,tran.ts);

	if (fpgamif_set_fpga_chan_tran(&tran) != 0) {
		LOGERROR("fpgamif_set_fpga_chan_tran:  Failed.");
        return -1;
    }

	return 0;
}

static int psagent_vchannel_stop(void *adap, void *mif, struct psp_msg *msg)
{
    struct chan_tran_cfg tran;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}
    memset(&tran, 0, sizeof(struct chan_tran_cfg));
    tran.chan = msg->tran.chan;
    tran.port = msg->tran.port;
    tran.e1 = msg->tran.e1;
    tran.ts = msg->tran.ts;
    tran.en = 0;
    LOG("vchannel_stop : chan %d port %d e1 %d ts %d ", tran.chan,tran.port,tran.e1,tran.ts);

	if (fpgamif_set_fpga_chan_tran(&tran) != 0) {
		LOGERROR("fpgamif_set_fpga_chan_tran:  Failed.");
        return -1;
    }

	return 0;
}

struct port_change_status {
    int change;
    char result[8];
};
struct all_port_change_status {
    int change_cnt;
    struct port_change_status p_ch[FPGADRV_OIAMC_PORT_MAX];
};

static int psagent_check_phy(void *adap, void *mif, struct psp_msg *msg)
{
	struct sdh_phy_stat phy_stat[FPGADRV_OIAMC_PORT_MAX];
    struct all_port_change_status all_ch;
    int i, changed, len;
    char buf[1024];
    char device = 0;
    char *p = NULL;


    memset(phy_stat, 0, sizeof(struct sdh_phy_stat) * FPGADRV_OIAMC_PORT_MAX);
    memset(&all_ch, 0, sizeof(struct all_port_change_status));
    if (fpgamif_get_phy_status(mif, phy_stat) != 0) {
        LOGERROR("%d: fpgamif_get_phy_status Failed.", __LINE__);
		return -1;
	}

    if (glb_data.first == 0) {
        for (i = 0; i < FPGADRV_OIAMC_PORT_MAX; i++) {
            glb_data.phy[i] = phy_stat[i].status;
        }

        glb_data.first = 1;
        return 0;
    }
    //compere
    changed = 0;
    memset(&all_ch, 0, sizeof(struct all_port_change_status));
    for (i = 0; i < FPGADRV_OIAMC_PORT_MAX; i++) {
        if (phy_stat[i].status != glb_data.phy[i]) {
            all_ch.p_ch[i].change = 1;
            all_ch.p_ch[i].result[0] = 0xff;
            all_ch.p_ch[i].result[1] = 0xff;
            all_ch.p_ch[i].result[2] = 0xff;
            all_ch.p_ch[i].result[3] = 0xff;
            all_ch.p_ch[i].result[4] = 0xff;
            all_ch.p_ch[i].result[5] = 0xff;
            all_ch.p_ch[i].result[6] = 0xff;
            all_ch.p_ch[i].result[7] = 0x7f;
            glb_data.phy[i] = phy_stat[i].status;
            all_ch.change_cnt++;
            changed = 1;
            continue;
        } else {
            /*check port have loslof*/
            if(phy_stat[i].status == 0x03)
            continue;
        }
    }

    if (changed == 0)
        return 0;

    len = sizeof(pkt_hdr) + 4 + (all_ch.change_cnt * 10);
    memset(buf, 0, sizeof(buf));
    pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_TRAP_LINK_CHANGE  >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_TRAP_LINK_CHANGE >> 0) & 0xff);
    device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
    pkthdr_set_device((pkt_hdr *)buf, device);
    pkthdr_set_channel((pkt_hdr *)buf, 0);

    p = buf + sizeof(pkt_hdr);
    CODING_32(p, all_ch.change_cnt);
    for (i = 0; i < FPGADRV_OIAMC_PORT_MAX; i++) {
        if (all_ch.p_ch[i].change != 1)
            continue;

        CODING_8(p, psglb.slot);
        CODING_8(p, (i+1));
        CODING_8(p, all_ch.p_ch[i].result[7]);
        CODING_8(p, all_ch.p_ch[i].result[6]);
        CODING_8(p, all_ch.p_ch[i].result[5]);
        CODING_8(p, all_ch.p_ch[i].result[4]);
        CODING_8(p, all_ch.p_ch[i].result[3]);
        CODING_8(p, all_ch.p_ch[i].result[2]);
        CODING_8(p, all_ch.p_ch[i].result[1]);
        CODING_8(p, all_ch.p_ch[i].result[0]);
    }

    SEND_PKTS(adap, buf, len, pkt_stat.spkts);
    LGWRDEBUG(buf, len, "Send change msg:");
    pkt_stat.check_phy_cnt++;
    return 0;
}

static int psagent_reset_pointer(void *adap, void *mif, struct psp_msg *msg)
{
	if (fpgamif_set_fpga_reset_pointer(mif) != 0) {
        LOGERROR("fpgamif_set_fpga_reset_pointer failed."); 
        return -1; 
    }
    return 0;
}

static int psagent_tran_enable(void *adap, void *mif, struct psp_msg *msg)
{
    struct tran_en tran;

    memset(&tran, 0, sizeof(struct tran_en));
    tran.en = msg->tran_en.enable;
    tran.revert = msg->tran_en.revert;

    LOG("psagent_tran_enable : en %d revert %d", tran.en, tran.revert);
	if (fpgamif_set_fpga_tran_en(mif, &tran) != 0) {
        LOGERROR("fpgamif_set_fpga tran_enable ."); 
        return -1; 
    }
    return 0;
}

static struct psp_ops ops = {
	.open = fpgamif_open,
	.close = fpgamif_close,
    .register_msg = psagent_register_msg,
    .register_ack_msg = psagent_register_ack_msg,
	.scan_clear = psagent_scan_clear,
	.notify_anm = psagent_notify_anm,
	.notify_rel = psagent_notify_rel,
	.get_phy_stat = psagent_get_phy_stat,
	.get_channel_stat = psagent_get_channel_stat,
	.clear_index = psagent_clear_index,
	.vchannel_start = psagent_vchannel_start,
	.vchannel_stop = psagent_vchannel_stop,
    .clear_all = psagent_clear_all,
    .check_phy = psagent_check_phy,
    .reset_pointer = psagent_reset_pointer,
    .tran_enable = psagent_tran_enable,
};

#if defined(__cplusplus)
extern "C" {
#endif

struct psp_ops *psagent_ops_register(void)
{
    /* init psagent glb data */
    glb_data.first = 0;
    memset(glb_data.phy, 0, sizeof(glb_data.phy));
    memset(glb_data.e1, 0, sizeof(glb_data.e1));

	return &ops;
}

#if defined(__cplusplus)
}
#endif

