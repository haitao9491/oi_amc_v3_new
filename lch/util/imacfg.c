/*
 *
 * imascan.c - A brief description goes here.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "cconfig.h"
#include "imacfg.h"

#ifdef WIN32
#pragma warning ( disable : 4996 )
#endif

static int ima_m[4] = {32, 64, 128, 256};

static int GetMphylink(int intrPhylink)
{
	intrPhylink = intrPhylink - ((intrPhylink / 28) * 7);
	return intrPhylink;
}

static int GetNphylink(int myPhylink)
{
	myPhylink = myPhylink;
	return myPhylink + ((myPhylink / 21) * 7);
}

static unsigned int GetValue(unsigned long hd, const char *sname,
		const char *id,	int sectionindex, unsigned int defaultValue)
{
	int value = 0;

	if ((value = CfgGetValueInt(hd, sname, id, 1, sectionindex + 1)) == -1) {
		LOGWARN("load_ima_cfg: Can't find [%s] %s", sname, id);
		return defaultValue;
	}

	LOGDEBUG("load_ima_cfg: [%s%d] %s = %d", sname, sectionindex, id, value);

	return value;
}

static unsigned int GetClockValue(unsigned long hd, const char *sname,
		int sectindex)
{
	char value[128];
	unsigned int defaultValue = 2;

	if (CfgGetValue(hd, sname, "Clock Mode", value, 1, sectindex + 1) == -1)
	{
		LOGWARN("load_ima_cfg: Can't find [%s] Clock Mode", sname);
		return defaultValue;
	}

	LOGDEBUG("load_ima_cfg: [%s%d] Clock Mode = %s", sname, sectindex, value);

	if (strcmp(value, "ITC") == 0)
		return 1;
	else if (strcmp(value, "CTC") == 0)
		return 2;
	else {
		LOGWARN("load_ima_cfg: [%s%d] Clock Mode failed.", sname, sectindex);
		return defaultValue;
	}
}

static unsigned int GetVersionValue(unsigned long hd, const char *sname,
		const char *id, int sectindex)
{
	char value[128];

	if (CfgGetValue(hd, sname, "Version", value, 1, sectindex + 1) == -1)
	{
		LOGWARN("load_ima_cfg: Can't find [%s] Version", sname);
		if (strcmp(id, "Version") == 0)
			return 0;
		else if (strcmp(id, "Label") == 0)
			return 3;
	}

	LOGDEBUG("load_ima_cfg: [%s%d] %s = %s", sname, sectindex, id, value);

	if (strcmp(value, "1.0") == 0) {
		return 1;
	} else {
		if (strcmp(value, "1.1") != 0)
			LOGWARN("load_ima_cfg: [%s%d] %s failed.", sname, sectindex, id);
		if (strcmp(id, "Version") == 0)
			return 0;
		else if (strcmp(id, "Label") == 0)
			return 3;
		else {
			return 1;
		}
	}
}

static unsigned int GetTxMValue(unsigned long hd, const char *sname,
		int sectindex)
{
	int value = 0;
	unsigned int defaultValue = 2;

	if ((value = CfgGetValueInt(hd, sname, "Tx M value", 1, sectindex+1)) == -1)
	{
		LOGWARN("load_ima_cfg: Can't find [%s] Tx M value", sname);
		return defaultValue;
	}

	LOGDEBUG("load_ima_cfg: [%s%d] TxM = %d", sname, sectindex, value);

	if (value == 32)
		return 0;
	else if (value == 64)
		return 1;
	else if (value == 128)
		return 2;
	else if (value == 256)
		return 3;
	else {
		LOGWARN("load_ima_cfg: [%s%d] TxM failed.", sname, sectindex);
		return defaultValue;
	}
}

static unsigned int GetEn(unsigned long hd, const char *sname,
		const char *id, int sectindex)
{
	char value[128];
	unsigned int defaultValue = 0;

	if (CfgGetValue(hd, sname, id, value, 1, sectindex + 1) == -1)
	{
		LOGWARN("load_ima_cfg: Can't find [%s] %s", sname, id);
		return defaultValue;
	}

	LOGDEBUG("load_ima_cfg: [%s%d] %s = %s", sname, sectindex, id, value);

	if ((strcmp(value, "no") == 0) || (strcmp(value, "n") == 0))
		return 0;
	else if ((strcmp(value, "yes") == 0) || (strcmp(value, "y") == 0))
		return 1;
	else {
		LOGWARN("load_ima_cfg: [%s%d] %s failed.", sname, sectindex, id);
		return defaultValue;
	}
}

static int CheckRxLinks(IMA_CFG imaCfg)
{
	unsigned int i, j;
	char chRxLinks[MAX_LNK_NUM];

	memset(chRxLinks, 0, sizeof(chRxLinks));
	for (i = 0; i < imaCfg.grpNumAll; i++) {
		for (j = 0; j < imaCfg.grpCfg[i].numRxLinks; j++) {
			if (chRxLinks[imaCfg.grpCfg[i].rxLinks[j]] == 1) {
				LOGWARN("load_ima_cfg: Group[%d] RxLink[%d],"
						"the physical link configuration duplicate", i, j);
				return -1;
			}
			chRxLinks[imaCfg.grpCfg[i].rxLinks[j]] = 1;
		}
	}
	return 0;
}

static int CheckTxvLinks(IMA_CFG imaCfg)
{
	unsigned int i, j;
	char chTxLinks[IMA_MAX_LINKS_OF_IMA_GRP];

	for (i = 0; i < imaCfg.grpNumAll; i++) {
		if (imaCfg.grpMode[i] != 2)
			continue;

		memset(chTxLinks, 0, sizeof(chTxLinks));
		for (j = 0; j < imaCfg.grpCfg[i].numTxLinks; j++) {
			if (chTxLinks[imaCfg.grpCfg[i].txLinks[j].lid] == 1) {
				LOGWARN("load_ima_cfg: Group[%d] TxLink[%d],"
						"the link id configuration duplicate", i, j);
				return -1;
			}
			chTxLinks[imaCfg.grpCfg[i].txLinks[j].lid] = 1;
		}
	}
	return 0;
}

static int CheckTxpLinks(IMA_CFG imaCfg)
{
	unsigned int i, j;
	char chTxLinks[MAX_LNK_NUM];

	memset(chTxLinks, 0, sizeof(chTxLinks));
	for (i = 0; i < imaCfg.grpNumAll; i++) {
		if (imaCfg.grpMode[i] != 2)
			continue;

		for (j = 0; j < imaCfg.grpCfg[i].numTxLinks; j++) {
			if (chTxLinks[imaCfg.grpCfg[i].txLinks[j].phyLink] == 1) {
				LOGWARN("load_ima_cfg: Group[%d] TxLink[%d],"
						"the physical link configuration duplicate", i, j);
				return -1;
			}
			chTxLinks[imaCfg.grpCfg[i].txLinks[j].phyLink] = 1;
		}
	}
	return 0;
}

static int LoadImaGrpLinksCfg(unsigned long hd, const char *sname,
		int grpindex, IMA_CFG *imaCfg)
{
	int i = 0;
	const char *rid = "RxLink";
	const char *tid = "TxLink";
	int linkRCount = 0;
	int linkTCount = 0;
	char value[128];
	int lid, offset, phyLink;
	int phy;

	linkRCount = CfgGetCount(hd, sname, rid, grpindex + 1);
	linkTCount = CfgGetCount(hd, sname, tid, grpindex + 1);

	if ((imaCfg->grpCfg[grpindex].numRxLinks <=0) ||
		(imaCfg->grpCfg[grpindex].numTxLinks <=0) ||
		(imaCfg->grpCfg[grpindex].numRxLinks != linkRCount) ||
		(imaCfg->grpCfg[grpindex].numTxLinks != linkTCount)) {
		LOGWARN("load_ima_cfg: Loading [%s%d] Links option failed",
			sname, grpindex);
		return -1;
	}

	for (i = 0; i < IMA_MAX_LINKS_OF_IMA_GRP; i++) {
		if (i < linkRCount)  {
			if (CfgGetValue(hd, sname, rid, value, i + 1, grpindex + 1) != -1) {
				phy = GetNphylink(atoi(value));
				imaCfg->grpCfg[grpindex].rxLinks[i] = phy;
				imaCfg->linkInGrp[phy] = grpindex + 1;
			} else {
				LOGWARN("load_ima_cfg: Loading [%s%d] Links Rx option failed",
						sname, grpindex);
				return -1;
			}
		}

		if (i < linkTCount) {
			if (CfgGetValue(hd, sname, tid, value, i + 1, grpindex + 1) != -1) {
				if(3 != sscanf(value, "%d[%d]-%d", &lid, &offset, &phyLink))
					break;
				phy = GetNphylink(phyLink);
				imaCfg->grpCfg[grpindex].txLinks[i].lid       = lid;
				imaCfg->grpCfg[grpindex].txLinks[i].icpOffset = offset;
				imaCfg->grpCfg[grpindex].txLinks[i].phyLink   = phy;
				imaCfg->linkInGrp[phy] = grpindex + 1;
			} else {
				LOGWARN("load_ima_cfg: Loading [%s%d] Links Tx option failed",
						sname, grpindex);
				return -1;
			}
		}

		if ((i >= linkRCount) && (i >= linkTCount))
			break;
	}

	return 0;
}

static int LoadImaGroupCfg(unsigned long hd, IMA_CFG *imaCfg, int i)
{
	const char *sn = "IMA Group";

	imaCfg->grpCfg[i].numRxLinks  = GetValue(hd, sn, "Rx Links Num", i, 0);
	imaCfg->grpCfg[i].numTxLinks  = GetValue(hd, sn, "Tx Links Num", i, 0);
	if (LoadImaGrpLinksCfg(hd, sn, i, imaCfg) < 0) {
		return -1;
	}

	imaCfg->grpCfg[i].clockMd     = GetClockValue(hd, sn, i);
	imaCfg->grpCfg[i].symmetryMd  = GetValue(hd, sn, "Symmetry Mode", i, 0);
	imaCfg->grpCfg[i].imaVersion  = GetVersionValue(hd, sn, "Version", i);
	imaCfg->grpCfg[i].imaOamLabel = GetVersionValue(hd, sn, "Label", i);
	imaCfg->grpCfg[i].rxImaIdCfgEn= GetValue(hd, sn, "Rx ImaId Cfg", i, 0);
	imaCfg->grpCfg[i].rxImaIdCfg  = GetValue(hd, sn, "Rx ImaId", i, 0);
	imaCfg->grpCfg[i].txImaId     = GetValue(hd, sn, "Tx ImaId", i, i+1);
	imaCfg->grpCfg[i].txVphy      = GetValue(hd, sn, "Tx Vphy ID", i, i);
	imaCfg->grpCfg[i].rxVphy      = GetValue(hd, sn, "Rx Vphy ID", i, i);
	imaCfg->grpCfg[i].rxM         = 0xf;
	imaCfg->grpCfg[i].pRx         = GetValue(hd, sn, "Min Rx links", i, 2);
	imaCfg->grpCfg[i].pTx         = GetValue(hd, sn, "Min Tx links", i, 2);
	imaCfg->grpCfg[i].txM         = GetTxMValue(hd, sn, i);
	imaCfg->grpCfg[i].maxDelay    = GetValue(hd, sn, "Max Delay", i, 1023);
	imaCfg->grpCfg[i].delayGb     = GetValue(hd, sn,
			"Delay Guardband", i, 10);
	imaCfg->grpCfg[i].delayAddEn  = GetEn(hd, sn, "Delay AddLink En", i);
	imaCfg->grpCfg[i].stuffMd     = imaCfg->grpCfg[i].clockMd;
	imaCfg->grpCfg[i].stuffAdvMd  = GetValue(hd, sn,
			"Stuff Advertise Mode", i, 1);
	imaCfg->grpCfg[i].txTrl       = GetValue(hd, sn, "Tx Trl PHY ID", i,
			imaCfg->grpCfg[i].txLinks[0].phyLink);

	return 0;
}

static int LoadTcCfg(unsigned long hd, IMA_CFG *imaCfg, int index, int *i)
{
	int j;
	const char *sn = "IMA Group";
	const char *id = "PhyLinkID";
	char value[512];
	char *p, *q;
	int phy;

	if (CfgGetValue(hd, sn, "PhyLinkID", value, 1, index + 1) == -1) {
		LOGWARN("load_ima_cfg: Can't find [%s] %s", sn, id);
		return -1;
	}

	p = value;

	j = 0;
	while (*p && (j < MAX_LNK_NUM)) {
		phy = strtol(p, &q, 0);
		if ((phy <= 0) || (*q && (*q != ','))) {
			return -1;
		}

		p = q + ((*q == ',') ? 1 : 0);
		j++;

		phy = GetNphylink(phy);
		imaCfg->grpCfg[*i].txVphy = *i;
		imaCfg->grpCfg[*i].rxVphy = *i;
		imaCfg->grpCfg[*i].rxLinks[0] = phy;
		imaCfg->linkInGrp[phy] = LINK_IN_TC;
		imaCfg->grpMode[*i] = 1;
		imaCfg->tcNum++;
		(*i)++;
	}
	imaCfg->grpNumAll += j - 1;

	return 0;
}

static int LoadImaModeCfg(unsigned long hd, int index)
{
	const char *id = "mode";
	char value[128];
	char rc;

	/* Mode option */
	if (CfgGetValue(hd, "IMA Group", id, value, 1, index + 1) == -1) {
		LOGWARN("load_ima_cfg: Loading option [%s] failed.", id);
		return -1;
	}

	if (strcmp(value, "IMA") == 0) {
		rc = 2;
	} else if (strcmp(value, "TC") == 0) {
		rc = 1;
	} else {
		LOGWARN("load_ima_cfg: Mode[%s] Unknown.", value);
		return -1;
	}

	LOGDEBUG("IMA Group: Option [%s]: %s", id, value);
	return rc;
}

int imacfg_load(unsigned long hd, void *cfg)
{
	IMA_CFG *imaCfg = (IMA_CFG *)cfg;
	int grpCount = 0;
	int i = 0, j = 0;
	int mode;

	memset(imaCfg, 0, sizeof(IMA_CFG));
	memset(imaCfg->linkInGrp, LINK_NOT_GRP, sizeof(imaCfg->linkInGrp));

	grpCount = CfgGetCount(hd, "IMA Group", NULL, 0);
	imaCfg->grpNumAll = grpCount;

	for (i = 0; i < grpCount; i++) {
		mode = LoadImaModeCfg(hd, i);
		imaCfg->grpMode[i] = mode;

		if (mode == 2) {
			/* Group option */
			if (LoadImaGroupCfg(hd, imaCfg, i) < 0) {
				LOGWARN("load_ima_cfg: Loading Group option failed.");
				return -1;
			}
			imaCfg->grpId[i] = imaCfg->groupNum;
			imaCfg->groupNum++;
			j++;
		} else if (mode == 1) {
			if (LoadTcCfg(hd, imaCfg, i, &j) < 0) {
				LOGWARN("load_ima_cfg: Loading TC option failed.");
				return -1;
			}
		}
	}

	if (CheckRxLinks(*imaCfg) < 0)
		return -1;
	if (CheckTxpLinks(*imaCfg) < 0)
		return -1;
	if (CheckTxvLinks(*imaCfg) < 0)
		return -1;

	return 0;
}

static void dump_cfg_ima_links(FILE *fp, IMA_CFG *imaCfg, int gNum)
{
	int i = 0;
	int phy;

	fprintf(fp, "Rx Links Num\t= %u\n", imaCfg->grpCfg[gNum].numRxLinks);
	for (i = 0; i < imaCfg->grpCfg[gNum].numRxLinks; i++) {
		phy = imaCfg->grpCfg[gNum].rxLinks[i];
		phy = GetMphylink(phy);
		fprintf(fp, "RxLink\t\t\t= %u\n", phy);
	}

	fprintf(fp, "Tx Links Num\t= %u\n", imaCfg->grpCfg[gNum].numTxLinks);
	for (i = 0; i < imaCfg->grpCfg[gNum].numTxLinks; i++) {
		phy = imaCfg->grpCfg[gNum].txLinks[i].phyLink;
		phy = GetMphylink(phy);
		fprintf(fp, "TxLink\t\t\t= %u[%u]-%u\n",
						imaCfg->grpCfg[gNum].txLinks[i].lid,
						imaCfg->grpCfg[gNum].txLinks[i].icpOffset,
						phy);
	}
}

static void dump_cfg_tc(FILE *fp, IMA_CFG *imaCfg)
{
	int phy;
	char data[512];
	int len = 0;
	int i;

	memset(data, 0, sizeof(data));
	fprintf(fp, "\n[IMA Group]\n");
	fprintf(fp, "mode = TC\n");

	for (i = 0; i < imaCfg->grpNumAll; i++) {
		if (imaCfg->grpMode[i] != 1)
			continue;

		phy = imaCfg->grpCfg[i].rxLinks[0];
		phy = GetMphylink(phy);
		if (len == 0)
			len += sprintf(data + len, "PhyLinkID = %u", phy);
		else
			len += sprintf(data + len, ",%u", phy);
	}

	fprintf(fp, "%s\n", data);

	fprintf(fp, "\n");
}

static void dump_cfg_ima_group(FILE *fp, IMA_CFG *imaCfg, int i)
{
	int phy;

	fprintf(fp, "\n[IMA Group]\n");
	fprintf(fp, "mode = IMA\n");

	if (imaCfg->grpCfg[i].clockMd == 1)
		fprintf(fp, "Clock Mode\t\t= ITC\n");
	else if (imaCfg->grpCfg[i].clockMd == 2)
		fprintf(fp, "Clock Mode\t\t= CTC\n");

	fprintf(fp, "Symmetry Mode\t= %u\n", imaCfg->grpCfg[i].symmetryMd);

	if (imaCfg->grpCfg[i].imaVersion == 1)
		fprintf(fp, "Version\t\t\t= 1.0\n");
	else if (imaCfg->grpCfg[i].imaVersion == 0)
		fprintf(fp, "Version\t\t\t= 1.1\n");

	fprintf(fp, "Rx ImaId Cfg\t= %u\n", imaCfg->grpCfg[i].rxImaIdCfgEn);
	fprintf(fp, "Rx ImaId\t\t= %u\n", imaCfg->grpCfg[i].rxImaIdCfg);
	fprintf(fp, "Tx ImaId\t\t= %u\n", imaCfg->grpCfg[i].txImaId);
	fprintf(fp, "Tx Vphy ID\t\t= %u\n", imaCfg->grpCfg[i].txVphy);
	fprintf(fp, "Rx Vphy ID\t\t= %u\n", imaCfg->grpCfg[i].rxVphy);
	fprintf(fp, "Min Rx links\t= %u\n", imaCfg->grpCfg[i].pRx);
	fprintf(fp, "Min Tx links\t= %u\n", imaCfg->grpCfg[i].pTx);

	if (imaCfg->grpCfg[i].txM == 0)
		fprintf(fp, "Tx M value\t\t= 32\n");
	else if (imaCfg->grpCfg[i].txM == 1)
		fprintf(fp, "Tx M value\t\t= 64\n");
	else if (imaCfg->grpCfg[i].txM == 2)
		fprintf(fp, "Tx M value\t\t= 128\n");
	else if (imaCfg->grpCfg[i].txM == 3)
		fprintf(fp, "Tx M value\t\t= 256\n");

	fprintf(fp, "Max Delay\t\t= %u\n",
			imaCfg->grpCfg[i].maxDelay);
	fprintf(fp, "Delay Guardband\t= %u\n", imaCfg->grpCfg[i].delayGb);

	if (imaCfg->grpCfg[i].delayAddEn == 0)
		fprintf(fp, "Delay AddLink En = no\n");
	else if (imaCfg->grpCfg[i].delayAddEn == 1)
		fprintf(fp, "Delay AddLink En = yes\n");

	fprintf(fp, "Stuff Advertise Mode\t= %u\n",
			imaCfg->grpCfg[i].stuffAdvMd);
	phy = imaCfg->grpCfg[i].txTrl;
	phy = GetMphylink(phy);
	fprintf(fp, "Tx Trl PHY ID\t= %u\n", phy);

	dump_cfg_ima_links(fp, imaCfg, i);
}

void imacfg_dump(FILE *fp, void *cfg)
{
	int i;
	IMA_CFG *imaCfg = (IMA_CFG *)cfg;

	for (i = 0; i < imaCfg->grpNumAll; i++) {
		if (imaCfg->grpMode[i] == 2) {
			dump_cfg_ima_group(fp, imaCfg, i);
		}
	}
	if (imaCfg->tcNum > 0) {
		dump_cfg_tc(fp, imaCfg);
	}
}

char *imacfg_print(IMA_CFG *cfg)
{
	static char buff[1000];
	int len = 0;
	unsigned int i, j, k = 0;
	int phy;

	if (cfg == NULL)
		return NULL;

	memset(buff, 0, sizeof(buff));

	len += sprintf(buff + len, "\nIMA Group number: %u.\n", cfg->groupNum);
	for (i = 0; i < cfg->grpNumAll; i++) {
		if (cfg->grpMode[i] != 2)
			continue;

		len += sprintf(buff + len, "Group[%u]: IMAID[%u] LinkNum[%u] M[%u]\n",
				k, cfg->grpId[i], cfg->grpCfg[i].numTxLinks,
				ima_m[cfg->grpCfg[i].txM]);
		for (j = 0; j < cfg->grpCfg[i].numTxLinks; j++) {
			phy = cfg->grpCfg[i].txLinks[j].phyLink,
			phy = GetMphylink(phy);
			len += sprintf(buff + len,
					"\tLinkId[%u]: PhyLink[%u] Offset[%u]\n",
					cfg->grpCfg[i].txLinks[j].lid,
					phy,
					cfg->grpCfg[i].txLinks[j].icpOffset);
		}
		k++;
	}

	LOGDEBUG("Len %u IMACFG: %s", len, buff);

	return buff;
}
