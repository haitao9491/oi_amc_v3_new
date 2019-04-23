/*
 *
 * swima_process.c - Software ima process.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "misc.h"
#include "aplog.h"
#include "pkt.h"
#include "bufmgr.h"
#include "imascan.h"
#include "swima.h"

#define CELL_TYPE_FILLER	0
#define CELL_TYPE_ICP		1
#define CELL_TYPE_ATM		2

#define STATUS_UNUSABLE		0
#define STATUS_USABLE		1
#define STATUS_ACTIVE		2

static int ima_m[4] = {32, 64, 128, 256};

struct cell {
	unsigned char type;
	unsigned char offset;
	unsigned char ifsn;
	unsigned char *data;
	struct cell *next, *prev;
};

struct ima_link {
	unsigned char status;
	unsigned char m;
	unsigned char icp;
	unsigned char ifsn;
	unsigned char offset;
	unsigned short channel;
	struct cell *head, *end;
};

struct group_stats {
	unsigned int freecount;
	unsigned int idcount;
	unsigned int icpcount;
	unsigned int atmcount;
};

struct ima_group {
	unsigned char status;
	unsigned char linknum;
	unsigned char lseq;
	unsigned char links[MAX_LNK_NUM];
};

struct ima_process {
	unsigned char gnum;
	unsigned char gseq;
	struct ima_group group[MAX_GRP_NUM];
	struct ima_link link[MAX_LNK_NUM];

	void  *dbufmgr;
	void  *cbufmgr;
	
	void (*get_ts)(unsigned int *s, unsigned int *ns, void *arg);
	void  *arg;
};

static struct cell *get_cell(struct ima_process *ima, unsigned char *pdata)
{
	struct cell *pcell = NULL;
	unsigned char *data = NULL;

	if ((ima == NULL) || (pdata == NULL))
		return pcell;

	if (ima->cbufmgr)
		pcell = (struct cell *)bufmgr_alloc(ima->cbufmgr);
	else
		pcell = (struct cell *)malloc(sizeof(struct cell));
	if (pcell == NULL)
		return pcell;
	memset(pcell, 0, sizeof(struct cell));
	pcell->data = pdata;
	pcell->ifsn = 0;
	pcell->offset = 0;
	pcell->next = NULL;
	pcell->prev = NULL;
	pcell->type = CELL_TYPE_ATM;

	/* 1-5  ATM cell header  Octet 1 = 0000 0000, Octet 2 = 0000 0000,
	 *                       Octet 3 = 0000 0000, Octet 4 = 0000 1011,
	 *                       Octet 5 = 0110 0100 (valid HEC)*/
	data = pkthdr_get_data((pkt_hdr *)pdata);
	if (data[0] == 0 &&
		data[1] == 0 &&
		data[2] == 0 &&
		data[3] == 0x0b &&
		data[4] == 0x64 &&
		(data[5] == 0x01 || data[5] == 0x03))
	{
        /* 7 A  Cell ID and Link ID
		 * Bit 7: IMA OAM Cell Type (0: FILLER cell)
		 * Bit 7: IMA OAM Cell Type (1: ICP cell)	 */
		if ((data[6] & 0x80) == 0x00)
			pcell->type = CELL_TYPE_FILLER;
		else if ((data[6] & 0x80) == 0x80)
			pcell->type = CELL_TYPE_ICP;
	}
	else if (data[0] == 0 &&
		data[1] == 0 &&
		data[2] == 0 &&
		data[3] == 0 &&
		data[4] == 0x55 &&
		data[5] == 0x6a)
	{
		pcell->type = CELL_TYPE_FILLER;
	}

	return pcell;
}

#define FREE_DATA(ima, data) { \
	if (ima->dbufmgr) \
		bufmgr_free(ima->dbufmgr, (char *)(data)); \
	else \
		free((data)); \
}

#define FREE_CELL(ima, cell) { \
	if (ima->cbufmgr) \
		bufmgr_free(ima->cbufmgr, (char *)(cell)); \
	else \
		free((cell)); \
}

static void reset_allcell(struct ima_process *ima, struct ima_link *link)
{
	struct cell *pcell = NULL;

	while (link->head) {
		pcell = link->head;
		link->head = link->head->next;
		if (link->head)
			link->head->prev = NULL;
		if (pcell->data)
			FREE_DATA(ima, pcell->data);
		FREE_CELL(ima, pcell);
	}
	link->head = NULL;
	link->end = NULL;
}

static void check_ima_link(struct ima_process *ima, struct ima_link *link, int lseq, IMA_ICP *picp)
{
	unsigned char m = ima_m[picp->grp_status_ctrl & 0x03]; // M
	unsigned char ifsn = picp->ifsn;
	unsigned char offset = picp->icp_off;
	static unsigned char status = STATUS_USABLE;
	unsigned char ostatus = link->status;

	if (link->m != m)
		link->status = STATUS_UNUSABLE;
	if (link->ifsn != ifsn)
		link->status = STATUS_UNUSABLE;
	if (link->offset != offset)
		link->status = STATUS_UNUSABLE;

	if (link->status == STATUS_UNUSABLE) {
		if (status != STATUS_UNUSABLE) {
			LOGCRITICAL("Check PhyLink[%u] ICP:"
					" M-%u[%u] ifsn-%u[%u] offset-%u[%u]",
					lseq, m, link->m, ifsn, link->ifsn, offset, link->offset);
		} else {
			LOGERROR("Check PhyLink[%u] ICP:"
					" M-%u[%u] ifsn-%u[%u] offset-%u[%u]",
					lseq, m, link->m, ifsn, link->ifsn, offset, link->offset);
		}
#if 0
		status = link->status;

		link->m = m;
		link->ifsn = ifsn;
		link->offset = offset;

		reset_allcell(ima, link);
		link->status = STATUS_USABLE;
#else
		if (link->offset == offset + 1) {
			struct cell *pcell, *qcell = NULL;

			if (link->end) {
				qcell = link->end;
				pcell = qcell->prev;

				if (qcell->type == CELL_TYPE_FILLER) {
					if (pcell) {
						pcell->next = NULL;
						link->end = pcell;
					}
					else {
						link->head = NULL;
						link->end  = NULL;
					}
			
					if (qcell->data)
						FREE_DATA(ima, qcell->data);
					FREE_CELL(ima, qcell);

					link->offset = offset;
					link->status = ostatus;

					LOGCRITICAL("Check PhyLink[%u] ICP: offset by one: previous is FILLER", lseq);

					return;
				}
				else {
					LOGCRITICAL("Check PhyLink[%u] ICP: offset by one: previous is NOT FILLER", lseq);
				}
			}
			else {
				LOGCRITICAL("Check PhyLink[%u] ICP: empty list", lseq);
			}
		}

		status = link->status;

		link->m = m;
		link->ifsn = ifsn;
		link->offset = offset;

		reset_allcell(ima, link);
		link->status = STATUS_USABLE;
#endif

		return;
	}
	LOGINFO("Check PhyLink[%u] ICP: M-%u[%u] ifsn-%u[%u] offset-%u[%u]",
		lseq, m, link->m, ifsn, link->ifsn, offset, link->offset);
}

static int sort_link_for_swima(const void *p1, const void *p2)
{
	CFG_IMA_TX_LINK *pLink1 = (CFG_IMA_TX_LINK *)p1;
	CFG_IMA_TX_LINK *pLink2 = (CFG_IMA_TX_LINK *)p2;

	return (pLink1->lid - pLink2->lid);
}

static CFG_IMA_TX_LINK *get_imalinks(CFG_IMA_GRP cfg)
{
	CFG_IMA_TX_LINK *links = cfg.txLinks;

	qsort(links, cfg.numTxLinks, sizeof(CFG_IMA_TX_LINK), sort_link_for_swima);

	return links;
}

static unsigned short get_imalink_channel(int seq)
{
	unsigned char port;
	unsigned char link;
	unsigned short channel;

	port = seq / 64;
	link = seq % 64;

	channel = ((((port & 0x03) >> 1) << 7) | ((link & 0x3f) << 1) | (port % 2));

	return channel;
}

void *swima_open(IMA_CFG *imacfg)
{
	int i, j, gseq = 0;
	int plseq, minseq;
	unsigned short channel;
	struct ima_process *ima = NULL;
	CFG_IMA_TX_LINK *links = NULL;

	ima = (struct ima_process *)malloc(sizeof(struct ima_process));
	if (ima == NULL)
		return NULL;

	memset(ima, 0, sizeof(struct ima_process));
	ima->gnum = imacfg->groupNum;
	ima->gseq = 0;

	for (i = 0; i < MAX_LNK_NUM; i++) {
		ima->link[i].status = STATUS_UNUSABLE;
		ima->link[i].m = 255;
		ima->link[i].icp = 0;
		ima->link[i].ifsn = 0;
		ima->link[i].offset = 0;
		ima->link[i].head = NULL;
		ima->link[i].end = NULL;
	}

	for (i = 0; i < imacfg->grpNumAll; i++) {
		if (imacfg->grpMode[i] != 2)
			continue;

		ima->group[gseq].status = STATUS_UNUSABLE;
		ima->group[gseq].lseq = 0;
		ima->group[gseq].linknum = (unsigned char)imacfg->grpCfg[i].numTxLinks;
		links = get_imalinks(imacfg->grpCfg[i]);
		
		minseq = 0xff;
		for (j = 0; j < ima->group[gseq].linknum; j++) {
			plseq = links[j].phyLink;
			plseq = plseq - ((plseq / 28) * 7);
			minseq = (plseq < minseq) ? plseq : minseq;
		}
		channel = get_imalink_channel(minseq);

		for (j = 0; j < ima->group[gseq].linknum; j++) {
			plseq = links[j].phyLink;
			plseq = plseq - ((plseq / 28) * 7);
			ima->group[gseq].links[j] = plseq;
			ima->link[plseq].m = ima_m[imacfg->grpCfg[i].txM];
			ima->link[plseq].channel = channel;
			LOGINFO("Group[%u] LinkID[%u] PhyLink[%u] Channel[%u]",
					gseq, links[j].lid, plseq, channel);
		}
		gseq++;
	}

	return ima;
}

void swima_set_bufmgr(void *hd, void *bufmgr)
{
	struct ima_process *ima = (struct ima_process *)hd;

	if ((ima == NULL) || (bufmgr == NULL))
		return;

	ima->dbufmgr = bufmgr;
}

int swima_set_cellbuf(void *hd, char *buf, int size)
{
	struct ima_process *ima = (struct ima_process *)hd;

	if ((ima == NULL) || (buf == NULL) || (size <= 0)) {
		LOGERROR("swima(set_cellbuf): invalid parameters");
		return 1;
	}

	ima->cbufmgr = bufmgr_open(buf, size, sizeof(struct cell));
	if (ima->cbufmgr == NULL) {
		LOGERROR("swima(set_cellbuf): failed to open cell bufmgr");
		return 1;
	}

	return 0;
}

void swima_set_timecb(void *hd, void (*get_ts)(unsigned int *, unsigned *, void *), void *arg)
{
	struct ima_process *ima = (struct ima_process *)hd;

	if ((ima == NULL) || (get_ts == NULL) || (arg == NULL))
		return;

	ima->get_ts = get_ts;
	ima->arg = arg;
}

void swima_close(void *hd)
{
	int i;
	struct cell *tmp = NULL;
	struct ima_process *ima = (struct ima_process *)hd;

	if (ima == NULL)
		return;

	if (ima->cbufmgr) {
		bufmgr_close(ima->cbufmgr);
	}

	for (i = 0; i < MAX_LNK_NUM; i++) {
		while (ima->link[i].head) {
			tmp = ima->link[i].head;
			ima->link[i].head = ima->link[i].head->next;
			if (ima->link[i].head)
				ima->link[i].head->prev = NULL;
			if (tmp->data)
				FREE_DATA(ima, tmp->data);
			FREE_CELL(ima, tmp);
		}
		ima->link[i].head = NULL;
		ima->link[i].end = NULL;
	}
	free(ima);
}

static int links_aligning(struct ima_process *ima, int gseq,
		int *ifsn, int *offset)
{
	int i;
	struct ima_link *link = NULL;
	struct ima_group *group = &(ima->group[gseq]);

	for (i = 0; i < group->linknum; i++) {
		link = &(ima->link[group->links[i]]);
		if (link->status == STATUS_UNUSABLE) {
			group->status = STATUS_UNUSABLE;
			LOGINFO("Link aligning: PhyLink[%u] unusable", group->links[i]);
			return -1;
		}
	}
	for (i = 0; i < group->linknum; i++) {
		link = &(ima->link[group->links[i]]);
		if (link->status == STATUS_USABLE) {
			group->status = STATUS_UNUSABLE;
			break;
		}
	}

	if (group->status == STATUS_ACTIVE)
		return 0;

	for (i = 0; i < group->linknum; i++) {
		link = &(ima->link[group->links[i]]);
		if ((link->head) && (*ifsn < link->head->ifsn)) {
			*ifsn = link->head->ifsn;
			*offset = link->head->offset;
		} else if ((link->head) && (*ifsn == link->head->ifsn)) {
			if (*offset < link->head->offset)
				*offset = link->head->offset;
		}
	}

	return 1;
}

static void group_aligning(struct ima_process *ima, int gseq,
		int ifsn, int offset)
{
	int i;
	struct ima_link *link = NULL;
	struct ima_group *group = &(ima->group[gseq]);
	struct cell *tmp = NULL;
	static struct group_stats grsta[MAX_GRP_NUM];

	for (i = 0; i < group->linknum; i++) {
		link = &(ima->link[group->links[i]]);
		tmp = link->head;
		while ((tmp) && (tmp->ifsn <= ifsn)) {
			if ((tmp->ifsn == ifsn) && (tmp->offset == offset)) {
				link->status = STATUS_ACTIVE;
				break;
			}
			grsta[gseq].freecount++;
			if (tmp->type == CELL_TYPE_FILLER) {
				grsta[gseq].idcount++;
			} else if (tmp->type == CELL_TYPE_ICP) {
				grsta[gseq].icpcount++;
			} else if (tmp->type == CELL_TYPE_ATM) {
				grsta[gseq].atmcount++;
			}
			link->head = link->head->next;
			if (link->head)
				link->head->prev = NULL;
			FREE_DATA(ima, tmp->data);
			FREE_CELL(ima, tmp);
			tmp = link->head;
			link->status = STATUS_USABLE;
		}
	}

	for (i = 0; i < group->linknum; i++)
		if (ima->link[group->links[i]].status != STATUS_ACTIVE)
			return;

	LOG("Group[%u] Aligning: ifsn[%u] offset[%u].", gseq, ifsn, offset);
	LOG("Group[%u] discarded cell %u(icp %u, id %u, atm %u)",
			gseq, grsta[gseq].freecount, grsta[gseq].icpcount,
			grsta[gseq].idcount, grsta[gseq].atmcount);
	memset(&(grsta[gseq]), 0, sizeof(struct group_stats));

	group->status = STATUS_ACTIVE;
	group->lseq = 0;
}

static void aligning(struct ima_process *ima)
{
	int i;
	int cellifsn = 0;
	int celloffset = 0;

	for (i = 0; i < ima->gnum; i++) {
		if (links_aligning(ima, i, &cellifsn, &celloffset) <= 0)
			continue;

		group_aligning(ima, i, cellifsn, celloffset);
	}
}

static int isfiller(unsigned char *data)
{
	int id6a = 0;
	int i;

	for (i = 5; i < 53; i++) {
		if (data[i] == 0x6a) {
			id6a++;
		}
	}

	if (id6a > 40)
		return 1;

	return 0;
}

static unsigned char *read_group_cell(struct ima_process *ima)
{
	struct ima_group *group = &(ima->group[ima->gseq]);
	struct ima_link *link = NULL;
	struct cell *pcell = NULL;
	unsigned char *data = NULL;
	unsigned char linkseq = 0;

	if (group->status != STATUS_ACTIVE)
		return NULL;

	linkseq = group->links[group->lseq];
	link = &(ima->link[linkseq]);
	pcell = link->head;
	if (pcell == NULL)
		return NULL;

	LOGINFO("Read %s: PhyLink[%u][%u] Group[%u][%u]",
			((pcell->type == CELL_TYPE_FILLER) ? "Filler" :
			((pcell->type == CELL_TYPE_ICP) ? "ICP" : "ATM Cell")),
			linkseq, pcell->offset, ima->gseq, group->lseq);

	group->lseq++;
	if (group->lseq >= group->linknum)
		group->lseq = 0;

	link->head = pcell->next;
	if (link->head)
		link->head->prev = NULL;
	else
		link->end = NULL;

	if ((pcell->type <= CELL_TYPE_ICP) ||
			(isfiller(pkthdr_get_data((pkt_hdr *)pcell->data)))) {
		FREE_DATA(ima, pcell->data);
		pcell->data = NULL;
	}

	data = pcell->data;

	pcell->data = NULL;
	pcell->next = NULL;
	pcell->prev = NULL;
	FREE_CELL(ima, pcell);

	if (data)
		pkthdr_set_channel((pkt_hdr *)data, link->channel);

	return data;
}

static int add_link_cell(struct ima_link *link, int lseq, struct cell *pcell)
{
	if (link->head == NULL) {
		link->head = pcell;
		link->end = pcell;
	} else {
		pcell->prev = link->end;
		link->end->next = pcell;
		link->end = pcell;
	}

	LOGINFO("%s -> PhyLink[%u]: offset %u",
		((pcell->type == CELL_TYPE_ICP) ? "ICP" :
		((pcell->type == CELL_TYPE_FILLER) ? "FILLER" : "CELL")), lseq, pcell->offset);

	return 0;
}

static void set_link_arg(struct ima_link *link, int lseq)
{
	link->offset++;
	if (link->offset >= link->m) {
		if (link->icp == 0) {
			LOGERROR("PhyLink[%u] ifsn[%u] offset[%u] M[%u] Missing ICP Cell."
				" Status->unusable", lseq, link->ifsn, link->offset, link->m);
			link->status = STATUS_UNUSABLE;
		}
		link->icp = 0;
		link->offset = 0;

		if (link->ifsn < 255)
			link->ifsn++;
		else
			link->ifsn = 0;
	}
}

static int write_link_cell(struct ima_process *ima, unsigned char *pdata)
{
	struct ima_link *link = NULL;
	struct cell *pcell = NULL;
	IMA_ICP *picp = NULL;
	unsigned short linknum = 0;

	if (pdata == NULL)
		return -1;

	if (pkthdr_get_dlen((pkt_hdr *)pdata) != 53) {
		LOGERROR("swima: not an atm cell, %d bytes, discarded.",
				pkthdr_get_plen((pkt_hdr *)pdata));
		FREE_DATA(ima, pdata);
		return -1;
	}

	linknum = pkthdr_get_channel((pkt_hdr *)pdata);
	if (linknum < MAX_LNK_NUM) {
		link = &(ima->link[linknum]);
		pcell = get_cell(ima, pdata);
	} else {
		LOGERROR("swima: not link num %u, discarded.", linknum);
	}
	if (pcell == NULL) {
		FREE_DATA(ima, pdata);
		return -1;
	}

	if (pcell->type == CELL_TYPE_ICP) {
		link->icp++;
		if (link->icp > 1) {
			LOGINFO("PhyLink[%u] StuffEvent: ifsn[%u] offset[%u] ICPCount[%u]",
				linknum, link->ifsn, link->offset, link->icp);
			if (pcell->data)
				FREE_DATA(ima, pcell->data);
			FREE_CELL(ima, pcell);
			return 1;
		}
		picp = (IMA_ICP *)pkthdr_get_data((pkt_hdr *)pcell->data);
		check_ima_link(ima, link, linknum, picp);
	}

	pcell->offset = link->offset;
	pcell->ifsn = link->ifsn;

	if (add_link_cell(link, linknum, pcell) < 0)
		return -1;

	set_link_arg(link, linknum);

	return 0;
}

static void set_pkt_time(struct ima_process *ima, pkt_hdr *ph)
{
	unsigned int s, us, ns;

	if (ima->get_ts) {
		(*ima->get_ts)(&s, &ns, ima->arg);
		pkthdr_set_ts_s(ph, s);
		pkthdr_set_ts_ns(ph, ns);
	}
	else {
		if (get_sys_time_s(&s, &us) > 0) {
			pkthdr_set_ts_s(ph, s);
			pkthdr_set_ts_ns(ph, (us * 1000));
		}
	}
}

unsigned char *swima_proc(void *hd, unsigned char *pdata)
{
	int gseq;
	unsigned char *data = NULL;
	struct ima_process *ima = (struct ima_process *)hd;

	if (write_link_cell(ima, pdata) < 0) {
		LOGERROR("Error: Write cell into link.");
	}

	aligning(ima);

	gseq = ima->gseq;
	do {
		data = read_group_cell(ima);
		if (data) {
#if 0
			pkthdr_set_channel((pkt_hdr *)data, ima->gseq);
#endif
			pkthdr_set_subtype((pkt_hdr *)data, PKT_SUBTYPE_ATM);
			set_pkt_time(ima, (pkt_hdr *)data);
		}

		ima->gseq++;
		if (ima->gseq >= ima->gnum)
			ima->gseq = 0;
		if (ima->gseq == gseq)
			break;
	} while (data == NULL);

	return data;
}

