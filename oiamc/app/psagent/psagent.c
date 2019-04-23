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
#include "os.h"
#include "aplog.h"
#include "coding.h"
#include "list.h"
#include "adapter.h"
#include "adapter_cs.h"
#include "mutex.h"
#include "psp.h"
#include "oic.h"
#include "oiclib.h"
#include "psagent.h"

struct init_par {
	unsigned char reset;
	unsigned char mode;
	unsigned char get;
	unsigned char age;
	unsigned char scan;
}; 

static void psagent_upload_linkmap_data(void *adap)
{
#define DATA_SIZE 1024
	unsigned char buf[64 + DATA_SIZE], *p;
	unsigned char data[DATA_SIZE];
	FILE *fp;
	int i, exist;
	int len, size;
	
	exist = 0;
	for (i = 0; i < 5; i++) {
		if ((access("/tmp/stat.result", 0) == 0) && (access("/tmp/stat.result.done", 0) == 0)) {
			exist = 1;
			break;
		}
		else {
			sleep(1);
		}
	}
	if (exist == 0) {
		return ;
	}

	fp = fopen("/tmp/stat.result", "rb");
	if (fp == NULL) {
		rename("/tmp/stat.result", "/tmp/stat.result.bak");
		remove("/tmp/stat.result.done");
		return ;
	}

	LOG("upload linkmap data to server.");

	size = fread(data, 1, DATA_SIZE, fp);
	len = sizeof(pkt_hdr) + 8 + size;
	memset(buf, 0, sizeof(buf));
	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_BLOCK_DATA >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_BLOCK_DATA >> 0) & 0xff);

	p = buf + sizeof(pkt_hdr);

	CODING_16(p, 1);
	CODING_16(p, 1);
	CODING_32(p, size);
	memcpy(p, data, size);

	if (adapter_write(adap, buf, len) <= 0) {
		LOGERROR("Failed to send SCAN_RESULT message.");
	}
	else {
		LOGINFO("linkmap data upload 1 size:%d", len);
	}

	while ((size = fread(data, 1, DATA_SIZE, fp)) == DATA_SIZE) {
		len = sizeof(pkt_hdr) + 8 + size;
		memset(buf, 0, sizeof(buf));
		pkthdr_set_sync((pkt_hdr *)buf);
		pkthdr_set_plen((pkt_hdr *)buf, len);
		pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_BLOCK_DATA >> 8) & 0xff);
		pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_BLOCK_DATA >> 0) & 0xff);

		p = buf + sizeof(pkt_hdr);

		CODING_16(p, 2);
		CODING_16(p, 1);
		CODING_32(p, size);
		memcpy(p, data, size);

		if (adapter_write(adap, buf, len) <= 0) {
			LOGERROR("Failed to send SCAN_RESULT message.");
		}
		else {
			LOGINFO("linkmap data upload 2 size:%d", len);
		}
		/*delay 20ms*/
		SLEEP_MS(20);
	}

	len = sizeof(pkt_hdr) + 8 + size;
	memset(buf, 0, sizeof(buf));
	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_BLOCK_DATA >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_BLOCK_DATA >> 0) & 0xff);

	p = buf + sizeof(pkt_hdr);

	CODING_16(p, 3);
	CODING_16(p, 1);
	if (size > 0) {
		CODING_32(p, size);
		memcpy(p, data, size);
	}
	else {
		CODING_32(p, 0);
	}
	if (adapter_write(adap, buf, len) <= 0) {
		LOGERROR("Failed to send SCAN_RESULT message.");
	}
	else {
		LOGINFO("linkmap data upload 3 size:%d", len);
	}

	fclose(fp);

	LOG("upload linkmap data to server success.");

	rename("/tmp/stat.result", "/tmp/stat.result.bak");
	remove("/tmp/stat.result.done");
}

static void psagent_notify_scan_end(void *adap)
{
	unsigned char buf[64], *p;
	int len;

	len = sizeof(pkt_hdr) + 13;
	memset(buf, 0, sizeof(buf));
	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_SCAN_RESULT >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_SCAN_RESULT >> 0) & 0xff);

	p = buf + sizeof(pkt_hdr);

	CODING_8(p, 1);//scan finsh

	if (adapter_write(adap, buf, len) <= 0) {
		LOGERROR("Failed to send SCAN_RESULT message.");
	}

}

static void psagent_allch_start(void *mif, unsigned char channel)
{
	int port, i, rc;
	unsigned char phylink;	

	for (port = 1; port <= 4; port++) {
		for (i = 1; i <= 63; i++) {
			phylink = (port - 1) * 64 + (i - 1);
			rc = oiclib_set_fpga_64k_ch_tran_start(mif, phylink, channel);
			LOGDEBUG("allch(start): [%u.%u]: rc %d", phylink, channel, rc);
		}
	}
}

static void psagent_allch_stop(void *mif, int channel)
{
	int port, i, rc;
	unsigned char phylink;	

	for (port = 1; port <= 4; port++) {
		for (i = 1; i <= 63; i++) {
			phylink = (port - 1) * 64 + (i - 1);
			rc = oiclib_set_fpga_64k_ch_tran_stop(mif, phylink, channel);
			LOGDEBUG("allch(stop): [%u.%u]: rc %d", phylink, channel, rc);
		}
	}
}

static int psagent_scan_init(void *adap, void *mif, struct psp_msg *msg)
{
	struct init_par in_par;
	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}
	memset(&in_par,0x00,sizeof(in_par));
	
	if (psagent_is_gsm()) {
		LOG("Initmask:=%d",msg->scan.initmask);	
		in_par.reset = msg->scan.initmask & 0x01;
		in_par.mode = msg->scan.initmask & 0x02;
		in_par.get  = msg->scan.initmask & 0x04;
		in_par.age  = msg->scan.initmask & 0x08;

		if(in_par.reset) {
			if (oiclib_set_fpga_pl_reset(mif) != 0) {
				LOGERROR("scan(reset): failed to reset scan.");
			}	
      else {
				LOG("scan(reset):clear voice learned.");
			}
		}
		if(in_par.mode)
		{
			in_par.scan = msg->scan.scanmode;
			if(oiclib_set_fpga_pl_mode(mif,in_par.scan) != 0) {
				LOGERROR("scan(mode): failed to set.");
			}	
      else {
				LOG("scan(mode): Success to set.");
			}
		}
		if(in_par.get) {
			if (oiclib_get_fpga_pl_stat(mif) != 0) {
				LOGERROR("scan(get): failed to get stat");
			}
			else {
				LOG("scan(get):Running get stat done.");
			}
			psagent_upload_linkmap_data(adap);	
			psagent_notify_scan_end(adap);
		}
		if(in_par.age) {
			if(oiclib_set_fpga_pl_age_channel(mif,1) != 0) {
				LOGERROR("scan(age): failed to age channel");
			}
			else{
				LOG("scan(age): Success to age channel");
			}
		}
	}

	return 0;
}
static int psagent_scan_start(void *adap, void *mif, struct psp_msg *msg)
{
	struct fpga_cfg cfg;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	cfg.index = msg->scan.index;
	cfg.en = 1;
	cfg.ts = msg->scan.channel;
	cfg.flag = 1;
	
	if (oiclib_set_fpga_pl_cfgch_start(mif, cfg.index, cfg.ts, cfg.en) != 0) {
		LOGERROR("scan(start event): failed to start scan: index %u, ts %d channel-stat %d, en %d", 
			cfg.index, cfg.ts, cfg.en, cfg.flag);
	}

	return 0;
}

static int psagent_scan_stop(void *adap, void *mif, struct psp_msg *msg)
{
	struct fpga_cfg cfg;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}
	
	cfg.index=msg->scan.index;
	cfg.en=0;
	cfg.ts=msg->scan.channel;
	cfg.flag=0;

	if (oiclib_set_fpga_pl_cfgch_stop(mif, cfg.index, cfg.ts, cfg.en) != 0) {
		LOGERROR("scan(start event): failed to stop scan: index %u, ts %d channel-stat %d, en %d", 
		cfg.index, cfg.ts, cfg.en, cfg.flag);
	}

	return 0;
}

static int psagent_probe_link_enable(void *adap, void *mif, struct psp_msg *msg)
{
	int rc = 0;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	rc = oiclib_set_fpga_2m_ch_tran_start(mif, msg->probe.phylink);
	LOGDEBUG("link(enable): [%u]: rc %d", msg->probe.phylink, rc);

	return 0;
}

static int psagent_probe_link_disable(void *adap, void *mif, struct psp_msg *msg)
{
	int rc = 0;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	rc = oiclib_set_fpga_2m_ch_tran_stop(mif, msg->probe.phylink);
	LOGDEBUG("link(disable): [%u]: rc %d", msg->probe.phylink, rc);

	return 0;
}

static int psagent_probe_channel_start(void *adap, void *mif, struct psp_msg *msg)
{
	int rc = 0;
	int rc1 = 0;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	if (psagent_is_gsm()) {
		rc = oiclib_set_fpga_64k_ch_tran_start(mif, msg->probe.phylink, msg->probe.channel);
		rc1 = oiclib_set_fpga_64k_ch_tran_start(mif, msg->probe.phylink1, msg->probe.channel1);
		LOGDEBUG("channel(start): [%u.%u]:[%u.%u] rc %d rc1 %d", msg->probe.phylink, msg->probe.channel, 
				msg->probe.phylink1, msg->probe.channel1, rc, rc1);
	}

	return 0;
}

static int psagent_probe_channel_stop(void *adap, void *mif, struct psp_msg *msg)
{
	int rc = 0;
	int rc1 = 0;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	if (psagent_is_gsm()) {
		rc = oiclib_set_fpga_64k_ch_tran_stop(mif, msg->probe.phylink, msg->probe.channel);
		rc1 = oiclib_set_fpga_64k_ch_tran_stop(mif, msg->probe.phylink1, msg->probe.channel1);
		LOGDEBUG("channel(stop): [%u.%u]:[%u.%u] rc %d rc1 %d", msg->probe.phylink, msg->probe.channel,
				msg->probe.phylink1, msg->probe.channel1, rc, rc1);
	}

	return 0;
}

static int psagent_probe_allch_start(void *adap, void *mif, struct psp_msg *msg)
{
	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	if (psagent_is_gsm()) {
		psagent_allch_start(mif, msg->probe.channel);
		LOGDEBUG("psagent_allch_start: channel:%d", msg->probe.channel); 
	}

	return 0;
}

static int psagent_probe_allch_stop(void *adap, void *mif, struct psp_msg *msg)
{
	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	if (psagent_is_gsm()) {
		psagent_allch_stop(mif, msg->probe.channel);
		LOGDEBUG("psagent_allch_stop: channel:%d", msg->probe.channel); 
	}

	return 0;
}

static int psagent_set_silence_timeout(void *adap, void *mif, struct psp_msg *msg)
{
	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	if (psagent_is_gsm()) {
		oiclib_set_fpga_silence_timeout(mif, msg->silence.timeout);
		LOG("psagent_silence_timeout: %d", msg->silence.timeout); 
	}

	return 0;
}

static int psagent_set_silence_range(void *adap, void *mif, struct psp_msg *msg)
{
	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

	if (psagent_is_gsm()) {
		oiclib_set_fpga_silence_range(mif, msg->silence.range);
		LOG("psagent_silence_range: %d", msg->silence.range); 
	}

	return 0;
}

static int psagent_get_silence_result(void *mif, void *data)
{
	struct silence_link *slink = (struct silence_link *)data;
	unsigned char stat = 0, stat1 = 0;
	int rc = 0, rc1 = 0;

	if (mif == NULL || data == NULL) {
		return -1;
	}

	if (!psagent_is_gsm()) {
		return -1;
	}

	rc = oiclib_get_fpga_silence_result(mif, slink->link, slink->ts, &stat);
	if (rc != 0) {
		LOGERROR("Silence(timeout): [%u:%u] get result failed.", slink->link, slink->ts);
	}
	rc1 = oiclib_get_fpga_silence_result(mif, slink->link1, slink->ts1, &stat1);
	if (rc1 != 0) {
		LOGERROR("Silence(timeout): [%u:%u] get result failed.", slink->link1, slink->ts1);
	}

	if ((stat == 0) && (stat1 == 0))
		return 1;

	rc = oiclib_set_fpga_64k_ch_tran_stop(mif, slink->link, slink->ts);
	if (rc != 0) {
		LOGERROR("Silence(timeout): [%u:%u] channel stop failed.", slink->link, slink->ts);
	}
	rc1 = oiclib_set_fpga_64k_ch_tran_stop(mif, slink->link1, slink->ts1);
	if (rc1 != 0) {
		LOGERROR("Silence(timeout): [%u:%u] channel stop failed.", slink->link1, slink->ts1);
	}
	LOG("Silence(timeout): [%u:%u][%u:%u]", slink->link, slink->ts, slink->link1, slink->ts1);

	return 0;
}

static int psagent_get_phy_stat(void *adap, void *mif, struct psp_msg *msg)
{
    struct fpga_board_runinfo rinfo;
    unsigned char buf[2048];
    unsigned char *p = NULL;
    int len = 0;
    int device = 0;
    int i;

	if (adap == NULL || mif == NULL || msg == NULL) {
		return -1;
	}

    if (oiclib_get_fpga_bd_runinfo(mif, &rinfo) != 0) {
        LOGERROR("%d--%s : get board run info failed.");
        return -1;
    }

	memset(buf, 0, sizeof(buf));
    len = sizeof(pkt_hdr) + 1 + (23 *  4);
    pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_PHY_STAT_ACK >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_PHY_STAT_ACK >> 0) & 0xff);
    device = 99;
    pkthdr_set_device((pkt_hdr *)buf, device);
    pkthdr_set_channel((pkt_hdr *)buf, 0);
    p = buf + sizeof(pkt_hdr);
    CODING_8(p, 4);
    for (i = 0; i < 4; i++) {
        CODING_8(p, rinfo.ports[i].los);
		CODING_8(p, 0);
		CODING_8(p, 0);
		CODING_16(p, 0);
		CODING_16(p, 0);
		CODING_32(p, 0);
		CODING_32(p, 0);
		CODING_32(p, 0);
		CODING_32(p, 0);
    }
    SEND_PKTS(adap, buf, len, pkt_stat.spkts);
    pkt_stat.get_phy_stat_ack++;

    return 0;
}

static struct psp_ops ops = {
	.open = oiclib_open,
	.close = oiclib_close,
	.scan_init = psagent_scan_init,
	.scan_start = psagent_scan_start,
	.scan_stop = psagent_scan_stop,
	.probe_link_enable = psagent_probe_link_enable,
	.probe_link_disable = psagent_probe_link_disable,
	.probe_channel_start = psagent_probe_channel_start,
	.probe_channel_stop = psagent_probe_channel_stop,
	.probe_allch_start = psagent_probe_allch_start,
	.probe_allch_stop = psagent_probe_allch_stop,
	.set_silence_timeout = psagent_set_silence_timeout,
	.set_silence_range = psagent_set_silence_range,
	.get_silence_result = psagent_get_silence_result,
    .get_phy_stat = psagent_get_phy_stat,
};

#if defined(__cplusplus)
extern "C" {
#endif

struct psp_ops *psagent_ops_register(void)
{
	return &ops;
}

#if defined(__cplusplus)
}
#endif

