/*
 * (C) Copyright 2013
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * nmphy.c - A description goes here.
 *
 */


#include "nm_board_phy.h"
#include "oiclib.h"
#include "oic.h"

typedef int (*nmboard_stat) (void *hd, void *arg);
struct nmboard_oiamcv3_cfg{
	unsigned char cmd;
	int nmdlen;	//datalen
	int nmplen;//payloadlen
	int plen;	//pktlen
	nmboard_stat fptr;
};

#define NM_OISDH_PORT_STATUS_T_LEN 	(sizeof(nm_oisdh_port_status_t))
#define NM_OISDH_PORT_FLOW_STAT_LEN (sizeof(nm_oisdh_port_flow_t))
#define NM_NMPKT_HDR_LEN	(sizeof(nmpkt_hdr))
#define NM_PKT_HDR_LEN 		(sizeof(pkt_hdr))

int nmboard_get_amcoiv3_port_status(void *hd, void *arg);
int nmboard_get_amcoiv3_port_flow_stat(void *hd, void *arg);

#define CMD_NUM 3
#define NM_CMD_OIAMC_V3_END 0xfa
static struct nmboard_oiamcv3_cfg nm_cfg_list[CMD_NUM] = {
	{
		NM_CMD_GET_FPGA_STATUS, 
		NM_OISDH_PORT_STATUS_T_LEN,
		NM_OISDH_PORT_STATUS_T_LEN + NM_NMPKT_HDR_LEN,
		NM_OISDH_PORT_STATUS_T_LEN + NM_NMPKT_HDR_LEN + NM_PKT_HDR_LEN,
		nmboard_get_amcoiv3_port_status,
	},

	{
		NM_CMD_GET_FPGA_FLOW_STAT,
		NM_OISDH_PORT_FLOW_STAT_LEN,
		NM_OISDH_PORT_FLOW_STAT_LEN + NM_NMPKT_HDR_LEN,
		NM_OISDH_PORT_FLOW_STAT_LEN + NM_NMPKT_HDR_LEN + NM_PKT_HDR_LEN,
		nmboard_get_amcoiv3_port_flow_stat,
	},

	{
		NM_CMD_OIAMC_V3_END,
		0,
		0,
		0,
		NULL,
	},
};

static struct nmboard_oiamcv3_cfg *phead = &nm_cfg_list[0];

#define for_each_cmd(phead, node) \
	for ((node) = (phead); ((node) != NULL && (node)->cmd != NM_CMD_OIAMC_V3_END); (node)++)

int nmboard_get_PKT_dlen()
{
	int len = 0;
	struct nmboard_oiamcv3_cfg *node = NULL;

	for_each_cmd(phead, node)
	{
		len += node->plen;
	}

	return len;
}

int nmboard_get_PKT_plen()
{
	unsigned int len = 0;
	len = nmboard_get_PKT_dlen();
	if (len > 0) { 
		return len += NM_PKT_HDR_LEN;
	}

	return -1;
}

struct nmboard_oiamcv3_cfg *nmboard_get_node(int cmd)
{
	struct nmboard_oiamcv3_cfg *node = NULL;

	for_each_cmd(phead, node)
	{
		if (node->cmd == cmd) {
			return node;
		}
	}
	LOGERROR("Failed to get cmd:0x%x.", cmd);
	return NULL;
}

int nmboard_get_pkt_plen(int cmd)
{
	struct nmboard_oiamcv3_cfg *node = NULL;

	node = nmboard_get_node(cmd);
	if (NULL != node) {
		return node->plen;
	}

	return -1;
}

int nmboard_get_nmpkt_plen(int cmd)
{
	struct nmboard_oiamcv3_cfg *node = NULL;

	node = nmboard_get_node(cmd);
	if (NULL != node) {
		return node->nmplen;
	}

	return -1;
}

int nmboard_get_nmpkt_dlen(int cmd)
{
	struct nmboard_oiamcv3_cfg *node = NULL;

	node = nmboard_get_node(cmd);
	if (NULL != node) {
		return node->nmdlen;
	}

	return -1;
}

int nmboard_get_amcoiv3_port_status(void *hd, void *arg)
{
	struct fpga_board_runinfo rinfo;
	nmboard_handle *ctl = (nmboard_handle *)hd;
	nm_oisdh_port_status_t *oips = (nm_oisdh_port_status_t *)arg;
	int i;

	if (!hd || !arg)
		return -1;

	memset(&rinfo, 0, sizeof(rinfo));
	if (oiclib_get_fpga_bd_runinfo(ctl->hd, &rinfo) != 0) {
		LOGERROR("%s: Failed to get board port infomation.", __func__);
		return -1;
	}

	for (i = 0; i < NM_OI_PORT_COUNT; i++) {
		oips->oisdh[i].los   = rinfo.ports[i].los;
		oips->oisdh[i].lof   = rinfo.ports[i].los;
		oips->oisdh[i].stm1  = rinfo.ports[i].stm1_synced;
		oips->oisdh[i].e1cnt = rinfo.ports[i].e1_synced_num;
		oips->oisdh[i].sig64kchcnt = rinfo.ports[i].ch_64k_num;
		oips->oisdh[i].sig64kfrcnt = rinfo.ports[i].ch_64k_frames;
		oips->oisdh[i].sig2mchcnt  = rinfo.ports[i].ch_2m_num;
		oips->oisdh[i].sig2mfrcnt  = rinfo.ports[i].ch_2m_frames;
	}

	return 0;
}

int nmboard_get_amcoiv3_port_flow_stat(void *hd, void *arg)
{
	struct fpga_board_runinfo rinfo;
	nmboard_handle *ctl = (nmboard_handle *)hd;
	nm_oisdh_port_flow_t *flow = (nm_oisdh_port_flow_t *)arg;
	int i;

	if (!hd || !arg)
		return -1;

	memset(&rinfo, 0, sizeof(rinfo));
	if (oiclib_get_fpga_bd_runinfo(ctl->hd, &rinfo) != 0) {
		LOGERROR("%s: Failed to get board port flow infomation.", __func__);
		return -1;
	}

	for (i = 0; i < NM_OI_FLOW_PORT_COUNT; i++) {
		flow->flow[i] = rinfo.traffic;
	}

	return 0;
}

int nmboard_cfg_nmpkt_hdr(void *hdr, int cmd)
{
	nmpkt_hdr *p = (nmpkt_hdr *)hdr;

	if (NULL != p) {
		nmpkthdr_slow_set_magic(p);
		nmpkthdr_set_dlen(p, nmboard_get_nmpkt_dlen(cmd));
		p->module = NM_MODULE_FPGA;
		p->cmd = cmd;

		return 0;
	}

	return -1;
}

int nmboard_oiamcv3_stat_nmpkt(void *hd, void *pkt, int cmd)
{
	int ret = -1, plen = 0;
	struct nmboard_oiamcv3_cfg *node = NULL;
	struct nmpkt_hdr_impl *nmpkt = (struct nmpkt_hdr_impl *)pkt;

	plen = nmboard_get_nmpkt_plen(cmd);
	if (plen <= 0) {
		LOGERROR("Failed to get command 0x%x len %d.", cmd, plen);
		return -1;
	}

	node = nmboard_get_node(cmd);
	if (NULL != nmpkt && NULL != node && NULL != node->fptr) {
		ret = node->fptr(hd, (void *)nmpkt->data);
		if (0 == ret) {
			nmboard_cfg_nmpkt_hdr((void *)nmpkt, cmd);
			return plen;;
		}
	}

	LOGERROR("Failed to get nmpkt.");
	return -1;
}

int nmboard_oiamcv3_stat(void *hd, void *ppkt, int cmd)
{
	int dlen = 0;
	pkt_hdr *pkt = (pkt_hdr *)ppkt;

	if (NULL != pkt) {

		dlen = nmboard_oiamcv3_stat_nmpkt(hd, pkthdr_get_data(pkt), cmd);
		if (dlen > 0) {
			pkthdr_set_dlen(pkt, dlen);
			pkthdr_set_sync(pkt);

			return 0;
		}
	}

	return -1;
}

void nmboard_oiamcv3_close(void *hd)
{
	nmboard_handle *h= (nmboard_handle *)hd;

	if (NULL != h) {
		h = (nmboard_handle *)hd;
		if (h->hd) {
			oiclib_close(h->hd);
			h->hd = NULL;
		}

		free(h);
	}

	LOGDEBUG("nmboard_oiamcv3_close()...");
}

void *nmboard_oiamcv3_open()
{
	nmboard_handle *ctl = NULL;

	ctl = (nmboard_handle *)malloc(sizeof(nmboard_handle));
	if (ctl == NULL) {
		LOGERROR("Failed to malloc handle.");
		return NULL;
	}
	memset(ctl, 0, sizeof(*ctl));

	/* openning */
	ctl->hd = oiclib_open();
	if (ctl->hd == NULL) {
		LOGERROR("Failed to open amcoiv3 device");
		goto nmphy_init_failed;
	}

	return ctl;

nmphy_init_failed:
	nmboard_oiamcv3_close(ctl);

	return NULL;

}

pkt_hdr *nmboard_oiamcv3_report(void *hd)
{
	int ret = 0;
	int plen = 0, dlen = 0;

	pkt_hdr *PKT = NULL;
	struct nmboard_oiamcv3_cfg *node = NULL;
	unsigned char *data = NULL;
	unsigned char *dend = NULL;


	plen = nmboard_get_PKT_plen();
	if (plen <=  0) {
		LOGERROR("OIAMCV3, Failed to get PKT len %d.", plen);
		return NULL;
	}

	PKT = (pkt_hdr *)malloc(plen);
	if (NULL != PKT) {
		memset(PKT, 0, plen);
		dlen = nmboard_get_PKT_dlen();
		data = pkthdr_get_data(PKT);
		dend = data + dlen;

		for_each_cmd(phead, node)
		{
			ret = nmboard_oiamcv3_stat(hd, data, node->cmd);
			if (0 == ret) {
				if  (data < dend) {
					data += pkthdr_get_plen((pkt_hdr *)data);
				}
			}
		}

		pkthdr_set_dlen(PKT, dlen);
		pkthdr_set_sync(PKT);
		return PKT;
	}

	LOGERROR("OIAMCV3, Failed to report status.");
	return NULL;

}

pkt_hdr *nmboard_oiamcv3_query(void *hd, pkt_hdr *ph)
{
	int cmd = 0, len = 0, ret = 0;
	unsigned short seq = 0;
	nmpkt_hdr *nmpkt = (nmpkt_hdr *)pkthdr_get_data(ph);
	pkt_hdr *PKT = NULL, *pkt = NULL;

	if (NULL != hd && NULL != ph) {
		cmd = nmpkt->cmd;
		seq = nmpkt->seq;
		len = nmboard_get_pkt_plen(cmd);
		if (len <= 0) {
			LOGERROR("OIAMCV3 command(0x%x), Failed to get pkt len %d.", len);
			return NULL;
		}

		PKT = (pkt_hdr *)malloc(len + sizeof(pkt_hdr));
		if (NULL != PKT) {
			memset(PKT, 0, len + sizeof(pkt_hdr));

			pkt = (pkt_hdr *)pkthdr_get_data(PKT);
			ret = nmboard_oiamcv3_stat(hd, pkt, cmd);
			if (0 == ret) {
				nmpkt = (nmpkt_hdr *)pkthdr_get_data(pkt);
				nmpkthdr_set_seq(nmpkt,seq);

				pkthdr_set_sync(PKT);
				pkthdr_set_dlen(PKT, len);
				return PKT;
			}
		}
	}

	LOGERROR("OIAMCV3 command(0x%x), Failed to query.");
	return NULL;
}
