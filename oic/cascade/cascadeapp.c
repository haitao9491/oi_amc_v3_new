/*
 * (C) Copyright 2017
 * Zhao Mingjing <mingjing.zhao@hlytec.com>
 *
 * cascade.cpp - cascade
 *
 */

#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include<stdio.h>
#include<string.h>
#include<ctype.h>

#include "os.h"
#include "apfrm.h"
#include "aplog.h"
#include "cconfig.h"
#include "misc.h"
#include "grpsm.h"
#include "sgfplib.h"

#define MAX_CHANNEL 64
//#define MAX_GROUP 63
#define MAX_GROUP 256
#define MAX_LINK 255

/* configuration file */
static char *cfgfile = NULL;

static void *sgfp = NULL;
static void *grpsm = NULL;

static unsigned char m_nFiberNum = 0;
static unsigned char m_nTryCount = (unsigned char)-1;
static unsigned short m_nOkPktCount = 0;
static unsigned short m_nErrPktCount = 0;
static unsigned char m_nVaildCount = 0;

static unsigned char m_nLinkNum = 0;

static struct slink_info m_objSLinkInfo[MAX_CHANNEL];
static struct link_info m_objLinkInfo[MAX_CHANNEL];

static int m_nGroupCfgNum = 0;
static struct group_cfg  *m_objGroupCfg = NULL;

static struct sgroup_all_info m_objTryGroup;
static struct sgroup_all_info m_objTmpTryGroup;

static int m_objVaildCount[MAX_GROUP];

static struct sgroup_all_info m_objSetGroup;
static struct sgroup_all_info g_OldAllGroup;
static struct sgroup_all_info g_OldAllGroup_deal;
static struct sgroup_all_info g_NewAllGroup;
static struct sgroup_all_info m_objSetGroupC4;

static char m_cFileName[512] = { 0 };
static char m_cSuffix[128] = { 0 };

static int m_nSetGroupInfoCout = 0;
static int m_nOldGroupDealCout = 0;
static int g_NewGroupCout = 0;
static int m_nSetGroupInfoCoutC4 = 0;
static int m_nPreTryGroupNum = 0;
static int g_agingtimes = 0;
static int g_fpgaid = 0;

struct port_valid {
	int port;
	int valid;
};

struct channel_to_groupid {
	int groupid;
	int linksize;
	unsigned char vc_valid;
	unsigned int svc_type;
	int valid;
};

static void cascade_show_usage(char *progname)
{
	printf("      --cfgfile <filename>: configuration file name\n");
}

static int cascade_parse_args(int argc, char **argv)
{
	int i = 0;
	
	while(i < argc) {
		if (strcmp(argv[i], "--cfgfile") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			cfgfile = argv[i];
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	if (cfgfile == NULL) {
		fprintf(stderr, "No configuration file specified.\n");
		return -1;
	}

	LOGDEBUG("cascade parse args end");
	return 0;
}

static void PrintStatistics()
{
}

static void clear(void)
{
	LOGDEBUG("clear start");
	memset(m_objSLinkInfo, 0, MAX_CHANNEL * sizeof(struct slink_info));

	memset(m_objVaildCount, 0, sizeof(m_objVaildCount));
	m_nGroupCfgNum = 0;
	m_objGroupCfg = NULL;
	m_nLinkNum = 0;
	LOGDEBUG("clear end");
}

static void clear_setinfo(void)
{
	int i = 0;
	struct sgroup_info *sgroup = NULL;

	//free m_objSetGroup
	for (i = 0; i < m_nSetGroupInfoCout; ++i) {
		sgroup = m_objSetGroup.ginfo;
		if (sgroup == NULL)
			break;

		if (sgroup + i) {
			if ((sgroup + i)->linkarrays) {
				free((sgroup + i)->linkarrays);
				(sgroup + i)->linkarrays = NULL;
			}
		}
	}

	m_nSetGroupInfoCout = 0;
	//end of free m_objSetGroup

	//free g_OldAllGroup
	for (i = 0; i < g_OldAllGroup.gsize; ++i) {
		sgroup = g_OldAllGroup.ginfo;
		if (sgroup == NULL)
			break;

		if (sgroup + i) {
			if ((sgroup + i)->linkarrays) {
				free((sgroup + i)->linkarrays);
				(sgroup + i)->linkarrays = NULL;
			}
		}
	}

	g_OldAllGroup.gsize = 0;
	//end of free g_OldAllGroup

	//free g_OldAllGroup_deal
	for (i = 0; i < m_nOldGroupDealCout; ++i) {
		sgroup = g_OldAllGroup_deal.ginfo;
		if (sgroup == NULL)
			break;

		if (sgroup + i) {
			if ((sgroup + i)->linkarrays) {
				free((sgroup + i)->linkarrays);
				(sgroup + i)->linkarrays = NULL;
			}
		}
	}

	m_nOldGroupDealCout = 0;
	g_OldAllGroup_deal.gsize = 0;
	//end of free g_OldAllGroup_deal

	//free g_NewAllGroup
	for (i = 0; i < g_NewGroupCout; ++i) {
		sgroup = g_NewAllGroup.ginfo;
		if (sgroup == NULL)
			break;

		if (sgroup + i) {
			if ((sgroup + i)->linkarrays) {
				free((sgroup + i)->linkarrays);
				(sgroup + i)->linkarrays = NULL;
			}
		}
	}

	g_NewGroupCout = 0;
	//end of free g_NewAllGroup

	//free m_nSetGroupInfoCoutC4
	for (i = 0; i < m_nSetGroupInfoCoutC4; ++i) {
		struct sgroup_info *sgroupc4 = m_objSetGroupC4.ginfo;
		if (sgroupc4 == NULL)
			break;

		if (sgroupc4 + i) {
			if ((sgroupc4 + i)->linkarrays) {
				free((sgroupc4 + i)->linkarrays);
				(sgroupc4 + i)->linkarrays = NULL;
			}
		}
	}

	m_nSetGroupInfoCoutC4 = 0;
	//end free m_nSetGroupInfoCoutC4	
}

static void exit_env(void)
{
	LOGDEBUG("exit env start");
	PrintStatistics();

	clear();
	clear_setinfo();

	if (m_objSetGroup.ginfo) {
		free(m_objSetGroup.ginfo);
		m_objSetGroup.ginfo = NULL;
	}
	memset(&m_objSetGroup, 0, sizeof(struct sgroup_all_info));

	if (g_OldAllGroup.ginfo) {
		free(g_OldAllGroup.ginfo);
		g_OldAllGroup.ginfo = NULL;
	}
	memset(&g_OldAllGroup, 0, sizeof(struct sgroup_all_info));

	if (g_OldAllGroup_deal.ginfo) {
		free(g_OldAllGroup_deal.ginfo);
		g_OldAllGroup_deal.ginfo = NULL;
	}
	memset(&g_OldAllGroup_deal, 0, sizeof(struct sgroup_all_info));

	if (g_NewAllGroup.ginfo) {
		free(g_NewAllGroup.ginfo);
		g_NewAllGroup.ginfo = NULL;
	}
	memset(&g_NewAllGroup, 0, sizeof(struct sgroup_all_info));

	if (m_objSetGroupC4.ginfo) {
		free(m_objSetGroupC4.ginfo);
		m_objSetGroupC4.ginfo = NULL;
	}
	memset(&m_objSetGroupC4, 0, sizeof(struct sgroup_all_info));

	if (sgfp) {
		sgfplib_close(sgfp);
		sgfp = NULL;
	}

	if (grpsm) {
		grpsm_close(grpsm);
		grpsm = NULL;
	}
	LOGDEBUG("exit env end");
}

static int init_env(long instance)
{
	char value[128] = { 0 };
	unsigned long hd = 0;
	const char *section = "Setting";
	const char *id = "Fiber";

	LOGDEBUG("init env start");
	if ((hd = CfgInitialize(cfgfile)) == 0ul) {
		LOGERROR("Parsing configuration file [%s] failed.", cfgfile);
		return -1;
	}

	grpsm = grpsm_open();
	if (grpsm == NULL) {
		LOGERROR("grpsm init failed.");
		CfgInvalidate(hd);
		return -1;
	}

	g_OldAllGroup.ginfo = (struct sgroup_info *)malloc(MAX_GROUP *
			sizeof(struct sgroup_info));
	if (g_OldAllGroup.ginfo == NULL) {
		LOGERROR("malloc old all group ginfo failed!");
		return -1;
	}

	g_OldAllGroup_deal.ginfo = (struct sgroup_info *)malloc(MAX_GROUP *
			sizeof(struct sgroup_info));
	if (g_OldAllGroup_deal.ginfo == NULL) {
		LOGERROR("malloc old all group ginfo failed!");
		return -1;
	}

	g_NewAllGroup.ginfo = (struct sgroup_info *)malloc(MAX_GROUP *
			sizeof(struct sgroup_info));
	if (g_NewAllGroup.ginfo == NULL) {
		LOGERROR("malloc new all group ginfo failed!");
		return -1;
	}

	m_objSetGroup.ginfo = (struct sgroup_info *)malloc(MAX_GROUP *
			sizeof(struct sgroup_info));
	if (m_objSetGroup.ginfo == NULL) {
		LOGERROR("malloc setgroup ginfo failed!");
		return -1;
	}

	m_objSetGroupC4.ginfo = (struct sgroup_info *)malloc(MAX_GROUP * 
			sizeof(struct sgroup_info));
	if (m_objSetGroupC4.ginfo == NULL) {
		LOGERROR("malloc setgroupc4 ginfo failed!");
		return -1;
	}

	id = "FiberNum";
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		m_nFiberNum = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, m_nFiberNum);

	id = "TryCount";
	m_nTryCount = 5;
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		m_nTryCount = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, m_nTryCount);

	id = "OkPktCount";
	m_nOkPktCount = 5;
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		m_nOkPktCount = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, m_nOkPktCount);

	id = "ErrPktCount";
	m_nErrPktCount = 5;
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		m_nErrPktCount = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, m_nErrPktCount);

	id = "ValidCount";
	m_nVaildCount = 5;
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		m_nVaildCount = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, m_nVaildCount);

	id = "filename";
	if (CfgGetValue(hd, section, id, m_cFileName, 1, 1) == 0) {
		LOG("[%s] %s: set to [%s]", section, id, m_cFileName);
	}

	id = "suffix";
	if (CfgGetValue(hd, section, id, m_cSuffix, 1, 1) == 0) {
		LOG("[%s] %s: set to [%s]", section, id, m_cSuffix);
	}

	id = "agingtimes";
	g_agingtimes = 10;
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		g_agingtimes = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, g_agingtimes);

	id = "fpgadev";
	g_fpgaid = 1;
	if (CfgGetValue(hd, section, id, value, 1, 1) == 0) {
		g_fpgaid = atoi(value);
	}
	LOG("[%s] %s: set to [%hhu]", section, id, g_fpgaid);

	CfgInvalidate(hd);

	sgfp = sgfplib_open(g_fpgaid);
	if (sgfp == NULL) {
		LOGERROR("sgfp init failed.");
		CfgInvalidate(hd);
		return -1;
	}

	LOGDEBUG("init env end");

	return 0;
}

int get_slinkinfo(unsigned char fiber)
{
	unsigned char i = 0;

	memset(m_objSLinkInfo, 0, MAX_CHANNEL * sizeof(struct slink_info));
	sgfplib_set_linkinfo_start(sgfp);

	for (i = 0; i < MAX_CHANNEL; ++i) {
		m_objSLinkInfo[i].fiber = fiber;
		m_objSLinkInfo[i].channel = i;
		sgfplib_get_linkinfo(sgfp, &m_objSLinkInfo[i]);
	}

	sgfplib_set_linkinfo_end(sgfp);

	return 0;
}

int convert_linkinfo()
{
	unsigned char i = 0;
	LOGDEBUG("convert linkinfo start");
	for (i = 0; i < MAX_CHANNEL; ++i) {
		//高阶无效直接成组
		//有效需要学习
		//低阶无效不参与学习
		//svc_type; /* 1-GFP/2-LAPS/3-PPP/4-ATM/0-OTH */
		if ((m_objSLinkInfo[i].channel_rate == GRPSM_CHANNELRATE_C4) &&
				(m_objSLinkInfo[i].vc_valid == 0)) {
			return 1;
			unsigned int m_nSize = sizeof(struct slink_info);
			struct sgroup_info *sginfo = m_objSetGroupC4.ginfo;

			(sginfo + m_nSetGroupInfoCoutC4)->linkarrays =
				(struct slink_info *)malloc(m_nSize);
			(sginfo + m_nSetGroupInfoCoutC4)->is_valid = 1;   //set valid
			(sginfo + m_nSetGroupInfoCoutC4)->linkarrays_size = 1;
			if ((sginfo + m_nSetGroupInfoCoutC4)->linkarrays == NULL) {
				LOGERROR("C4 linkarray is null");
				free(sginfo);
				sginfo = NULL;
				return -1;
			}
			struct slink_info *linkarrays = (sginfo + m_nSetGroupInfoCoutC4)->linkarrays;
			(linkarrays + 0)->fiber = m_objSLinkInfo[i].fiber;
			(linkarrays + 0)->fiber_rate = m_objSLinkInfo[i].fiber_rate;
			(linkarrays + 0)->channel = m_objSLinkInfo[i].channel;
			(linkarrays + 0)->channel_rate = m_objSLinkInfo[i].channel_rate;

			m_nSetGroupInfoCoutC4++;
			LOGDEBUG("convert linkinfo C4 end");

			return 1;
		}
		else if ((m_objSLinkInfo[i].channel_rate == GRPSM_CHANNELRATE_C12)) {
			if (m_nLinkNum < MAX_CHANNEL) {
				m_objLinkInfo[m_nLinkNum].fiber = m_objSLinkInfo[i].fiber;
				m_objLinkInfo[m_nLinkNum].fiber_rate =
					m_objSLinkInfo[i].fiber_rate;
				m_objLinkInfo[m_nLinkNum].channel =
					m_objSLinkInfo[i].channel;
				m_objLinkInfo[m_nLinkNum].channel_rate =
					m_objSLinkInfo[i].channel_rate;
				m_objLinkInfo[m_nLinkNum].vc_valid =
					m_objSLinkInfo[i].vc_valid;
				m_objLinkInfo[m_nLinkNum].svc_type =
					m_objSLinkInfo[i].svc_type;
				m_objLinkInfo[m_nLinkNum].is_lcas =
					m_objSLinkInfo[i].is_lcas;
				m_objLinkInfo[m_nLinkNum].is_member =
					m_objSLinkInfo[i].is_member;
				m_objLinkInfo[m_nLinkNum].is_last_member =
					m_objSLinkInfo[i].is_last_member;
				m_objLinkInfo[m_nLinkNum].mfi = m_objSLinkInfo[i].mfi;
				m_objLinkInfo[m_nLinkNum].sq = m_objSLinkInfo[i].sq;
				m_objLinkInfo[m_nLinkNum].pre_gid =
					m_objSLinkInfo[i].pre_gid;
				m_objLinkInfo[m_nLinkNum].cur_gid =
					m_objSLinkInfo[i].cur_gid;
				++m_nLinkNum;
			}
		}
		else {
			LOGDEBUG("NO such channel rate!");
			return -1;
		}
	}

	LOGDEBUG("convert linkinfo end");
	return 0;
}

int clear_try_group()
{
	int i = 0;

	for (i = 0; i < m_nGroupCfgNum; i++) {
		if (m_objTryGroup.ginfo[i].linkarrays) {
			free(m_objTryGroup.ginfo[i].linkarrays);
			m_objTryGroup.ginfo[i].linkarrays = NULL;
		}
	}
	if (m_objTryGroup.ginfo) {
		free(m_objTryGroup.ginfo);
		m_objTryGroup.ginfo = NULL;
	}
	memset(&m_objTryGroup, 0, sizeof(struct sgroup_all_info));
	
	return 0;
}

int Get()
{
	LOG("Get start");
	int m_nCount = 0;
	int i, j;
	int m_nTryGroupNum = 0;
	static struct group_cfg  *m_objPreGetGroupCfg = NULL;

	if (m_nGroupCfgNum <= 0) {
		m_nCount = grpsm_get(grpsm, NULL, 0, 0, &m_objGroupCfg);
		m_objPreGetGroupCfg = m_objGroupCfg;
	}
	else {
		m_objGroupCfg = NULL;
		LOG("get again");
		m_nCount = grpsm_get(grpsm, m_objPreGetGroupCfg, m_nGroupCfgNum,
				m_nPreTryGroupNum,	&m_objGroupCfg);
		LOGDEBUG("out get again");
		m_objPreGetGroupCfg = m_objGroupCfg;
	}

	m_nGroupCfgNum = m_nCount;
	LOGDEBUG("group count num = %d", m_nCount);
	if ((m_nGroupCfgNum > 0 && m_objGroupCfg == NULL) ||
		m_nGroupCfgNum < 0) {
		LOGERROR("m_objGroupCfg is zero,but group num is not zero");
		return -1;
	}

	if (m_nGroupCfgNum == 0) {
		LOGDEBUG("group count num is zero");
		return 0;
	}

	struct sgroup_info *ginfo = NULL;
	m_objTryGroup.ginfo = (struct sgroup_info *)malloc(m_nGroupCfgNum *
			sizeof(struct sgroup_info));
	if (m_objTryGroup.ginfo == NULL) {
		LOGERROR("malloc trygroup failed");
		return -1;
	}
	ginfo = m_objTryGroup.ginfo;

	for (i = 0; i < m_nGroupCfgNum; ++i) {
		if ((m_objGroupCfg + i)->chs == NULL) {
			LOGERROR("chs is null");
			clear_try_group();
			return -1;
		}

		unsigned int num_chs = (m_objGroupCfg + i)->num_chs;
		(ginfo + m_nTryGroupNum)->linkarrays_size = num_chs;
		unsigned int m_nSize = num_chs * sizeof(struct slink_info);
		(ginfo + m_nTryGroupNum)->linkarrays =
			(struct slink_info *)malloc(m_nSize);
		if ((ginfo + m_nTryGroupNum)->linkarrays == NULL) {
			LOGERROR("malloc linkarrays failed");
			continue;
		}

		for (j = 0; j < num_chs; ++j) {
			struct slink_info *slinka =
				(ginfo + m_nTryGroupNum)->linkarrays;
			struct group_channel *chsa = (m_objGroupCfg + i)->chs;

			(slinka + j)->fiber =  (chsa + j)->fiber;
			(slinka + j)->fiber_rate =  (chsa + j)->fiber_rate;
			(slinka + j)->channel =  (chsa + j)->channel;
			(slinka + j)->channel_rate =  (chsa + j)->channel_rate;
		}
		(ginfo + m_nTryGroupNum)->is_valid = 1;
		++m_nTryGroupNum;
	}
	m_objTryGroup.gsize = m_nTryGroupNum;
/*	LOG("<-------------------Try Group-------------------->");
	for (i = 0; i < m_nTryGroupNum; i++) {
	LOG("[Sgfp Group Info]");
	int link_nums = (ginfo + i)->linkarrays_size;
	struct slink_info *tlinkarrays = (ginfo + i)->linkarrays;
	for (j = 0; j < link_nums; j++) {
		LOG("link = %u,%u,%u,%u", (tlinkarrays + j)->fiber, (tlinkarrays + j)->fiber_rate,
		(tlinkarrays + j)->channel, (tlinkarrays + j)->channel_rate);
	}
												    }
	LOG("<-------------------Try Group End-------------------->");
*/
	LOG("Get end");
	return 1;
}

int alloc_tmptry(int group_size, int cur_group, int id)
{
	LOGDEBUG("alloc tmptry start");
	int i = 0;

	m_objTmpTryGroup.gsize = group_size;
	m_objTmpTryGroup.port_group = id;
	m_objTmpTryGroup.ginfo = (struct sgroup_info *)malloc(group_size *
			sizeof(struct sgroup_info));

	if (m_objTmpTryGroup.ginfo == NULL) {
		LOGERROR("malloc tmp try group failed");
		return -1;
	}

	for (i = 0; i < group_size; i++) {
		m_objTmpTryGroup.ginfo[i].linkarrays =
			(struct slink_info*)malloc(m_objTryGroup.ginfo[i+cur_group].linkarrays_size *
			sizeof(struct slink_info));
		if (m_objTmpTryGroup.ginfo[i].linkarrays == NULL) {
			LOGERROR("malloc tmp try linkarrays failed");
			return -1;
		}
		memcpy(m_objTmpTryGroup.ginfo[i].linkarrays,
				m_objTryGroup.ginfo[i+cur_group].linkarrays,
				m_objTryGroup.ginfo[i+cur_group].linkarrays_size *
				sizeof(struct slink_info));
		m_objTmpTryGroup.ginfo[i].group_id = i;
		m_objTmpTryGroup.ginfo[i].is_valid = 1;
		m_objTmpTryGroup.ginfo[i].linkarrays_size =
			m_objTryGroup.ginfo[i+cur_group].linkarrays_size;
	}
	
	LOGDEBUG("m_objTmpTryGroup.gsize=%d", m_objTmpTryGroup.gsize);
/*  LOG("<-------------------Tmp Try Group-------------------->");
	int j = 0;
	for (i = 0; i < group_size; i++) {
		LOG("[Sgfp Group Info]");
		int tmplink_nums = m_objTmpTryGroup.ginfo[i].linkarrays_size;
		struct slink_info *tmplinkarrays = m_objTmpTryGroup.ginfo[i].linkarrays;
		for (j = 0; j < tmplink_nums; j++) {
			LOG("link = %u,%u,%u,%u", (tmplinkarrays + j)->fiber, (tmplinkarrays + j)->fiber_rate,
			(tmplinkarrays + j)->channel, (tmplinkarrays + j)->channel_rate);
		}
	}
	LOG("<-------------------Tmp Try Group End-------------------->");
*/
	LOGDEBUG("alloc tmptry end");
	return 0;
}

int clear_tmptry_group()
{
	int i = 0;

	for (i = 0; i < m_objTmpTryGroup.gsize; i++) {
		if (m_objTmpTryGroup.ginfo[i].linkarrays) {
			free(m_objTmpTryGroup.ginfo[i].linkarrays);
			m_objTmpTryGroup.ginfo[i].linkarrays = NULL;
		}
	}

	if (m_objTmpTryGroup.ginfo) {
		free(m_objTmpTryGroup.ginfo);
		m_objTmpTryGroup.ginfo = NULL;
	}

	memset(&m_objTmpTryGroup, 0, sizeof(struct sgroup_all_info));
	return 0;
}

int try(int id)
{
	int i = 0, j = 0 , k = 0;
	int rc = 0;
	int PreTmpGroupNum = 0;
	int TmpGroupNum = 0;
	int CurLinkNum = 0;
	int CurGroupSize = 0;
	int okflag = 0;
	int showflag = 0;

	LOG("tmp try start");
	m_nPreTryGroupNum = 0;
	LOG("try group total num[%u]", m_nGroupCfgNum);
	while ((i < m_nGroupCfgNum) && (ap_is_running())) {
		for (i = TmpGroupNum; i < m_nGroupCfgNum && ap_is_running(); i++) {
			showflag++;
			if (showflag >= 1000) {
				showflag = 0;
				LOG("now try group number[%d]", i);
			}
			CurLinkNum +=  m_objTryGroup.ginfo[i].linkarrays_size;
			if ((CurLinkNum > MAX_LINK) ||
					(i == (m_nGroupCfgNum - 1))) {
				TmpGroupNum = i;
				if (i == (m_nGroupCfgNum - 1))
					TmpGroupNum = m_nGroupCfgNum;
				if ((CurLinkNum > MAX_LINK) &&
						(i == (m_nGroupCfgNum - 1))) {
					TmpGroupNum = (m_nGroupCfgNum - 1);
				}

				CurGroupSize = TmpGroupNum - PreTmpGroupNum;
				rc = alloc_tmptry(CurGroupSize, PreTmpGroupNum, id);
				if (rc < 0) {
					clear_try_group();
					clear_tmptry_group();
					return -1;
				}

				for (k = 0; k < m_nTryCount; ++k) {
					sgfplib_set_try_group(sgfp, &m_objTmpTryGroup);
					for (j = 0; j < m_objTmpTryGroup.gsize; ++j) {
						struct sgroup_info *ginfo = m_objTmpTryGroup.ginfo;

						if (((ginfo + j)->resokpkt >= m_nOkPktCount) &&
							((ginfo + j)->reserrpkt <= m_nErrPktCount)) {
							(m_objVaildCount[j])++;
						}
					}
				}

				for (k = 0; k < m_objTmpTryGroup.gsize; ++k) {
					m_nPreTryGroupNum++;
					if (m_objVaildCount[k] < m_nVaildCount) {
						(m_objGroupCfg + k + PreTmpGroupNum)->result = 0;
					}
					else {
						okflag = 1;
						(m_objGroupCfg + k + PreTmpGroupNum)->result = 1;
						unsigned char num = (m_objGroupCfg + k +
								PreTmpGroupNum)->num_chs;

						if (m_nSetGroupInfoCout > MAX_GROUP) {
							LOGERROR("out group range");
							clear_try_group();
							clear_tmptry_group();
							return -1;
						}

						memcpy((m_objSetGroup.ginfo + m_nSetGroupInfoCout),
								&m_objTmpTryGroup.ginfo[k],
								sizeof(struct sgroup_info));
						(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays = 
							(struct slink_info *)malloc(num *sizeof(struct slink_info));
						if ((m_objSetGroup.ginfo +
								m_nSetGroupInfoCout)->linkarrays == NULL) {
							LOGERROR("malloc setgroup linkarrays failed");
							clear_try_group();
							clear_tmptry_group();
							return -1;
						}

						memcpy((m_objSetGroup.ginfo +
									m_nSetGroupInfoCout)->linkarrays,
								m_objTmpTryGroup.ginfo[k].linkarrays,
								num *sizeof(struct slink_info));
						(m_objSetGroup.ginfo +
						 m_nSetGroupInfoCout)->is_valid = 1;
						(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->group_id =
							((m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays[0].channel << 2) +
							(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays[0].fiber%4;
						m_nSetGroupInfoCout++; 
					}
					m_objVaildCount[k] = 0;
				}

				PreTmpGroupNum = TmpGroupNum;
				CurLinkNum = 0;
				CurGroupSize = 0; 

				clear_tmptry_group();
				if ((okflag == 1) || (TmpGroupNum == m_nGroupCfgNum)) {
					clear_try_group();
					return 0;
				}
				break;
			}
		}
	}
	clear_try_group();
	clear_tmptry_group();
	return 0;
}

void study(int id)
{
	grpsm_push(grpsm, m_objLinkInfo, m_nLinkNum, NULL, 0);

	while ((Get() > 0) && (ap_is_running())) {
		if (try(id) < 0) {
			LOGERROR("try failed!");
			return;
		}
	}

}

int print_file(int id)
{
	char value[1024] = { 0 };
	char m_cEnd[2] = { 0x0a, 0};
	char section[128] = { 0 };

	FILE *fp = NULL;
	FILE *learned = NULL;
	FILE *aged = NULL;
	char partname[512] = { 0 };
	char agedfile[64] = { 0 };
	unsigned char i, j;
	sprintf(partname, "%s%d/sgfp%d%s", m_cFileName, g_fpgaid-1, id, m_cSuffix);
	sprintf(agedfile, "/tmp/fpga%d/agedfile_%d", g_fpgaid-1, id);
	sprintf(section, "[Sgfp Group Info]%s", m_cEnd);

	learned = fopen(partname, "w");
	if (learned == NULL) {
		LOGERROR("Archive: file %s openning failed.", partname);
		return -1;
	}

	aged = fopen(agedfile, "w");
	if (aged == NULL) {
		LOGERROR("Archive: file agedfile openning failed.");
		return -1;
	}

	for (i = 0; i < m_nSetGroupInfoCout; ++i) {
		struct sgroup_info *sgroup = m_objSetGroup.ginfo;
		if (sgroup == NULL) {
			LOGERROR("ginfo is NULL");
			return -1;
		}
		if ((sgroup + i)->is_valid == 0) {
			fp = aged;
		}
		else {
			fp = learned;
		}
		unsigned char m_nSize = (sgroup + i)->linkarrays_size;
		struct slink_info *link = (sgroup + i)->linkarrays;
		fwrite(section, strlen(section), 1, fp);
		for (j = 0; j < m_nSize; ++j) {
			sprintf(value, "link = %hhu,%hhu,%hhu,%hhu%s", (link + j)->fiber,
					(link + j)->fiber_rate, (link + j)->channel,
					(link + j)->channel_rate, m_cEnd);
			fwrite(value, strlen(value), 1, fp);
		}
	}

	for (i = 0; i < m_nSetGroupInfoCoutC4; ++i) {
		struct sgroup_info *sgroupc4 = m_objSetGroupC4.ginfo;
		if (sgroupc4 == NULL) {
			LOGERROR("c4 ginfo is NULL");
			return -1;
		}
		if ((sgroupc4 + i)->is_valid == 0) {
			fp = aged;
		}
		else {
			fp = learned;
		}
		unsigned char m_nSizeC4 = (sgroupc4 + i)->linkarrays_size;
		struct slink_info *linkc4 = (sgroupc4 + i)->linkarrays;
		fwrite(section, strlen(section), 1, fp);
		for (j = 0; j < m_nSizeC4; ++j) {
			sprintf(value, "link = %hhu,%hhu,%hhu,%hhu%s", (linkc4 + j)->fiber,
					(linkc4 + j)->fiber_rate, (linkc4 + j)->channel,
					(linkc4 + j)->channel_rate, m_cEnd);
			fwrite(value, strlen(value), 1, fp);
		}
	}


	fclose(learned);
	fclose(aged);
	learned = NULL;
	aged = NULL;
	fp = NULL;

	return 0;
}

//first 1:first study 0:after study
int print_groupfile(struct sgroup_all_info *all_group, int first, char *filename)
{
	char value[1024] = { 0 };
	char m_cEnd[2] = { 0x0a, 0};
	char section[128] = { 0 };

	FILE *fp = NULL;
	unsigned char i, j;
	sprintf(section, "[Sgfp Group Info]%s", m_cEnd);

	fp = fopen(filename, "w");
	if (fp == NULL) {
		LOGERROR("Archive: file %s openning failed.", filename);
		return -1;
	}

	if (first) {
		all_group->gsize = m_nSetGroupInfoCout;
	}
	else {
		all_group->gsize = g_NewGroupCout;
	}
	for (i = 0; i < all_group->gsize; ++i) {
		struct sgroup_info *sgroup = all_group->ginfo;
		if (sgroup == NULL) {
			LOGERROR("ginfo is NULL");
			return -1;
		}
		unsigned char m_nSize = (sgroup + i)->linkarrays_size;
		struct slink_info *link = (sgroup + i)->linkarrays;
		fwrite(section, strlen(section), 1, fp);
		sprintf(value, "groupid = %d%s", (sgroup + i)->group_id, m_cEnd);
		fwrite(value, strlen(value), 1, fp);
		if (first) {
			sprintf(value, "flag = 1%s", m_cEnd);
			fwrite(value, strlen(value), 1, fp);
		}
		else {
			sprintf(value, "flag = %d%s", (sgroup + i)->flag, m_cEnd);
			fwrite(value, strlen(value), 1, fp);
		}
		for (j = 0; j < m_nSize; ++j) {
			sprintf(value, "link = %hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,\
					%hu,%hhu,%u,%u,%u,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,\
					%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hhu,%hu,%s", (link + j)->fiber,
					(link + j)->fiber_rate, (link + j)->channel,(link + j)->channel_rate,
					(link + j)->vc_valid, (link + j)->is_lcas, (link + j)->is_member,
					(link + j)->is_last_member, (link + j)->mfi, (link + j)->sq,
					(link + j)->pre_gid, (link + j)->cur_gid, (link + j)->svc_type,
					(link + j)->tuptr_0110_rev, (link + j)->tuptr_0110_no, (link + j)->tuptr_1001_rev,
					(link + j)->tuptr_1001_no, (link + j)->v5_sync_cnt, (link + j)->k4_sync_cnt,
					(link + j)->sq_cnt, (link + j)->mfi_cnt, (link + j)->e1_sync, (link + j)->e1_sync_err,
					(link + j)->nfm_e1_sync, (link + j)->nfm_e1_sync_err, (link + j)->tuptr_val,
					(link + j)->v5_val, (link + j)->v5_cnt, m_cEnd);
			fwrite(value, strlen(value), 1, fp);
		}
	}

	fclose(fp);
	fp = NULL;

	return 0;
}

int read_cfgid(unsigned long hd, int index, int sec_index)
{
	char *p, *q, value[1024], buftmp[64];
	int i = 0;
	long result[30] = { 0 };

	p = NULL;
	q = NULL;
	memset(value, 0, sizeof(value));
	memset(result, 0 ,sizeof(result));
	if (CfgGetValue(hd, (char *)"Sgfp Group Info",
                 (char *)"link", value, index+1, sec_index+1) == -1) {
        LOGERROR("Loading link failed.");
        return -1;
    }

	p = value;
	while(1) {
		memset(buftmp, 0, sizeof(buftmp));
		char* q = strchr(p, ',');
		if (q == NULL && p != NULL) {
			result[i] = strtol(p, NULL, 0);
			break;
		}
		if ((q - p) <= 0) break;
		strncpy(buftmp, p, q - p);
		result[i] = strtol(buftmp, NULL, 0);
		i++;
		p = q + 1;
	}

	g_OldAllGroup.ginfo[sec_index].linkarrays[index].fiber = (unsigned char)result[0];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].fiber_rate = (unsigned char)result[1];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].channel = (unsigned char)result[2];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].channel_rate = (unsigned char)result[3];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].vc_valid = (unsigned char)result[4];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].is_lcas = (unsigned char)result[5];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].is_member = (unsigned char)result[6];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].is_last_member = (unsigned char)result[7];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].mfi = (unsigned short)result[8];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].sq = (unsigned char)result[9];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].pre_gid = (unsigned int)result[10];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].cur_gid = (unsigned int)result[11];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].svc_type = (unsigned int)result[12];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].tuptr_0110_rev = (unsigned char)result[13];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].tuptr_0110_no = (unsigned char)result[14];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].tuptr_1001_rev = (unsigned char)result[15];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].tuptr_1001_no = (unsigned char)result[16];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].v5_sync_cnt = (unsigned char)result[17];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].k4_sync_cnt = (unsigned char)result[18];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].sq_cnt = (unsigned char)result[19];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].mfi_cnt = (unsigned char)result[20];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].e1_sync = (unsigned char)result[21];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].e1_sync_err = (unsigned char)result[22];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].nfm_e1_sync = (unsigned char)result[23];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].nfm_e1_sync_err = (unsigned char)result[24];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].tuptr_val = (unsigned char)result[25];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].v5_val = (unsigned char)result[26];
	g_OldAllGroup.ginfo[sec_index].linkarrays[index].v5_cnt = (unsigned short)result[27];
	return 0;
}

int load_old_group_file(char *cfgfile)
{
	unsigned long cfghd;
    int secnum = 0;
	int idnum = 0;
	int i, j;
	char value[128] = { 0 };
	FILE *fp = NULL;

	if ((fp = fopen(cfgfile, "r")) == NULL) {
		LOG("group file not exist");
		return 1;
	}
	else {
		fclose(fp);
	}
	cfghd = CfgInitialize(cfgfile);
	if (cfghd == 0ul) {
		LOGERROR("cfghd error!");
		return -1;
	}

    secnum = CfgGetCount(cfghd, (char *)"Sgfp Group Info", NULL, 0);
	g_OldAllGroup.gsize = secnum;
	if (g_OldAllGroup.ginfo == NULL) {
		LOGERROR("malloc group info failed!\n");
		return -1;
	}
	for (i = 0; i < secnum; i++) {
		if (CfgGetValue(cfghd, (char *)"Sgfp Group Info", (char *)"groupid", value, 1, i + 1) == 0) {
			g_OldAllGroup.ginfo[i].group_id = atoi(value);
		}
		if (CfgGetValue(cfghd, (char *)"Sgfp Group Info", (char *)"flag", value, 1, i + 1) == 0) {
//			g_OldAllGroup.ginfo[i].flag = atoi(value) + 1;
		}
		g_OldAllGroup.ginfo[i].flag = 0;
		idnum = CfgGetCount(cfghd, (char *)"Sgfp Group Info", (char *)"link", i+1);
		g_OldAllGroup.ginfo[i].linkarrays_size = idnum;
		g_OldAllGroup.ginfo[i].linkarrays = (struct slink_info *)malloc(sizeof(struct slink_info)*idnum);
		memset(g_OldAllGroup.ginfo[i].linkarrays, 0 , sizeof(struct slink_info)*idnum);
		if (g_OldAllGroup.ginfo[i].linkarrays == NULL) {
			LOGERROR("malloc linkinfo of group info faild!");
			return -1;
		}
		for (j = 0; j < idnum; j++) {
			if (read_cfgid(cfghd, j, i) < 0) {
				LOGERROR("read config file faild!");
				return -1;
			}
		}
	}
	CfgInvalidate(cfghd);
	return 0;
}

//flags: set to 1(find); bigger than 1(go on aging)
int update_new_map()
{
	int i = 0, j = 0;
	int num = 0;

	for (i = 0; i < m_nSetGroupInfoCout; i++) {
		for (j = 0; j < g_OldAllGroup_deal.gsize; j++) {
			if (m_objSetGroup.ginfo[i].group_id ==
					g_OldAllGroup_deal.ginfo[j].group_id) {
				g_OldAllGroup_deal.ginfo[j].flag = 1;
				break;
			}
		}
		num = m_objSetGroup.ginfo[i].linkarrays_size;
		(g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays_size = num;
		(g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays =
					(struct slink_info *)malloc(num *sizeof(struct slink_info));
		if ((g_NewAllGroup.ginfo +
				g_NewGroupCout)->linkarrays == NULL) {
			LOGERROR("malloc new group linkarrays failed");
			return -1;
		}
		memcpy((g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays,
				m_objSetGroup.ginfo[i].linkarrays, num * sizeof(struct slink_info));
		(g_NewAllGroup.ginfo + g_NewGroupCout)->group_id =
			m_objSetGroup.ginfo[i].group_id;
		(g_NewAllGroup.ginfo + g_NewGroupCout)->flag = 1;
		g_NewGroupCout++;
		if (g_NewGroupCout > MAX_GROUP) {
			LOGERROR("new group cout is out of range!");
			return -1;
		}
	}

	for (i = 0; i < g_OldAllGroup_deal.gsize; i++) {
#if 0
		if (g_OldAllGroup_deal.ginfo[i].flag > g_agingtimes) {
			(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->group_id =
				g_OldAllGroup_deal.ginfo[i].group_id;
			(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->is_valid = 0;
			(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays_size = 1;
				(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays =
							(struct slink_info *)malloc(sizeof(struct slink_info));
				if ((m_objSetGroup.ginfo +
						m_nSetGroupInfoCout)->linkarrays == NULL) {
					LOGERROR("malloc setgroup linkarrays failed");
					return -1;
				}
				memcpy((m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays,
						g_OldAllGroup_deal.ginfo[i].linkarrays, sizeof(struct slink_info));

			m_nSetGroupInfoCout++;
		}
		else if ((g_OldAllGroup_deal.ginfo[i].flag > 1) &&
				(g_OldAllGroup_deal.ginfo[i].flag <= g_agingtimes)) {
				num = g_OldAllGroup_deal.ginfo[i].linkarrays_size;
				(g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays_size = num;
				(g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays =
							(struct slink_info *)malloc(num *sizeof(struct slink_info));
				if ((g_NewAllGroup.ginfo +
						g_NewGroupCout)->linkarrays == NULL) {
					LOGERROR("malloc newgroup linkarrays failed");
					return -1;
				}
				memcpy((g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays,
						g_OldAllGroup_deal.ginfo[i].linkarrays, num * sizeof(struct slink_info));
				(g_NewAllGroup.ginfo + g_NewGroupCout)->group_id =
					g_OldAllGroup_deal.ginfo[i].group_id;
				(g_NewAllGroup.ginfo + g_NewGroupCout)->flag = g_OldAllGroup_deal.ginfo[i].flag;
				g_NewGroupCout++;

				(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->group_id =
				g_OldAllGroup_deal.ginfo[i].group_id;
				(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->is_valid = 1;
				(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays_size = num;
				(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays =
							(struct slink_info *)malloc(num *sizeof(struct slink_info));
				if ((m_objSetGroup.ginfo +
						m_nSetGroupInfoCout)->linkarrays == NULL) {
					LOGERROR("malloc setgroup linkarrays 2 failed");
					return -1;
				}
				memcpy((m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays,
						g_OldAllGroup_deal.ginfo[i].linkarrays,num * sizeof(struct slink_info));

				m_nSetGroupInfoCout++;
		}
#endif
		if (g_OldAllGroup_deal.ginfo[i].flag == 1)
			continue;
		num = g_OldAllGroup_deal.ginfo[i].linkarrays_size;
		(g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays_size = num;
		(g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays =
					(struct slink_info *)malloc(num *sizeof(struct slink_info));
		if ((g_NewAllGroup.ginfo +
				g_NewGroupCout)->linkarrays == NULL) {
			LOGERROR("malloc newgroup linkarrays failed");
			return -1;
		}
		memcpy((g_NewAllGroup.ginfo + g_NewGroupCout)->linkarrays,
				g_OldAllGroup_deal.ginfo[i].linkarrays, num * sizeof(struct slink_info));
		(g_NewAllGroup.ginfo + g_NewGroupCout)->group_id =
			g_OldAllGroup_deal.ginfo[i].group_id;
		(g_NewAllGroup.ginfo + g_NewGroupCout)->flag = g_OldAllGroup_deal.ginfo[i].flag;
		g_NewGroupCout++;
		if (g_NewGroupCout > MAX_GROUP) {
			LOGERROR("new group cout 2 is out of range!");
			return -1;
		}

		(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->group_id =
		g_OldAllGroup_deal.ginfo[i].group_id;
		(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->is_valid = 1;
		(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays_size = num;
		(m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays =
					(struct slink_info *)malloc(num *sizeof(struct slink_info));
		if ((m_objSetGroup.ginfo +
				m_nSetGroupInfoCout)->linkarrays == NULL) {
			LOGERROR("malloc setgroup linkarrays 2 failed");
			return -1;
		}
		memcpy((m_objSetGroup.ginfo + m_nSetGroupInfoCout)->linkarrays,
				g_OldAllGroup_deal.ginfo[i].linkarrays,num * sizeof(struct slink_info));

		m_nSetGroupInfoCout++;
		if (m_nSetGroupInfoCout > MAX_GROUP) {
			LOGERROR("set group info cout is out of range!");
			return -1;
		}
	}
	return 0;
}

int check_old_group(unsigned int start_port)
{
	int i = 0, j = 0, flag = 0, rc = 0;
	int tmpport = 0, port = 0, channel = 0;
	struct fpga_board_runinfo_ex port_states;
	struct channel_to_groupid setmap[16][64];
	memset(setmap, 0, 16*64*sizeof(struct channel_to_groupid));
	for (i = 0; i < m_nSetGroupInfoCout; i++) {
		for (j = 0; j < m_objSetGroup.ginfo[i].linkarrays_size; j++) {
			channel = m_objSetGroup.ginfo[i].linkarrays[j].channel;
			port = m_objSetGroup.ginfo[i].linkarrays[j].fiber;
			setmap[port][channel].groupid = m_objSetGroup.ginfo[i].group_id;
			setmap[port][channel].linksize = m_objSetGroup.ginfo[i].linkarrays_size;
			setmap[port][channel].vc_valid = m_objSetGroup.ginfo[i].linkarrays[j].vc_valid;
			setmap[port][channel].svc_type = m_objSetGroup.ginfo[i].linkarrays[j].svc_type;
			setmap[port][channel].valid = 1;
		}
	}

	memset(&port_states, 0, sizeof(struct fpga_board_runinfo_ex));
	port_states.start_port = start_port;
	rc = sgfplib_get_fpga_bd_runinfo_ex(sgfp, &port_states);
	if (rc) {
		LOGERROR("get state failed\n");
		return -1;
	}
	struct port_valid portmap[4];
	memset(portmap, 0, 4*sizeof(struct port_valid));

	for (i = 0; i < g_OldAllGroup.gsize; i++) {
		for (j = 0; j < g_OldAllGroup.ginfo[i].linkarrays_size; j++) {
			port = g_OldAllGroup.ginfo[i].linkarrays[j].fiber;
			tmpport = port%4;
			channel = g_OldAllGroup.ginfo[i].linkarrays[j].channel;
			//remove port los is 1; stm1_synced is 1
			if ((port_states.ports[tmpport].stm1_synced == 1) || (port_states.ports[tmpport].los == 1)) {
				flag = 1;
				break;
			}

			//remove conflict group; vc_valid is 0; svc_type is differ frome history
			if (((setmap[port][channel].linksize != g_OldAllGroup.ginfo[i].linkarrays_size) ||
					(setmap[port][channel].groupid != g_OldAllGroup.ginfo[i].group_id) ||
					(setmap[port][channel].vc_valid == 0) ||
					(setmap[port][channel].svc_type != g_OldAllGroup.ginfo[i].linkarrays[j].svc_type))
					&& (setmap[port][channel].valid == 1)) {
				flag = 1;
				break;
			}
		}
		if (!flag) {
			(g_OldAllGroup_deal.ginfo + m_nOldGroupDealCout)->group_id =
				g_OldAllGroup.ginfo[i].group_id;
			(g_OldAllGroup_deal.ginfo + m_nOldGroupDealCout)->flag =
				g_OldAllGroup.ginfo[i].flag;
			(g_OldAllGroup_deal.ginfo + m_nOldGroupDealCout)->is_valid = 1;
			(g_OldAllGroup_deal.ginfo + m_nOldGroupDealCout)->linkarrays_size =
				g_OldAllGroup.ginfo[i].linkarrays_size;
			(g_OldAllGroup_deal.ginfo + m_nOldGroupDealCout)->linkarrays =
						(struct slink_info *)malloc(g_OldAllGroup.ginfo[i].linkarrays_size*sizeof(struct slink_info));
			if ((g_OldAllGroup_deal.ginfo +
					m_nOldGroupDealCout)->linkarrays == NULL) {
				LOGERROR("malloc old group deal linkarrays failed");
				return -1;
			}
			memcpy((g_OldAllGroup_deal.ginfo + m_nOldGroupDealCout)->linkarrays,
					g_OldAllGroup.ginfo[i].linkarrays, g_OldAllGroup.ginfo[i].linkarrays_size* sizeof(struct slink_info));

			m_nOldGroupDealCout++;
			if (m_nOldGroupDealCout > MAX_GROUP) {
				LOGERROR("old group deal count is out of range!");
				return -1;
			}
		}
		flag = 0;
	}
	g_OldAllGroup_deal.gsize = m_nOldGroupDealCout;
	return 0;
}

int put_new_group_file(char *cfgfile, unsigned int start_port)
{
	int rc = 0;
	int load_state = 0;

	load_state = load_old_group_file(cfgfile);
	if (load_state > 0) {
		LOG("first generate group file");
		rc = print_groupfile(&m_objSetGroup, 1, cfgfile);
		if (rc < 0) {
			LOGERROR("print first group file failed!");
			clear_setinfo();
			return -1;
		}
	}
	else if (load_state == 0){
		rc = check_old_group(start_port);
		if (rc < 0) {
			LOGERROR("check old group failed!");
			clear_setinfo();
			return -1;
		}
		rc = update_new_map();
		if (rc < 0) {
			LOGERROR("update new map failed!");
			clear_setinfo();
			return -1;
		}
		rc = print_groupfile(&g_NewAllGroup, 0, cfgfile);
		if (rc < 0) {
			LOGERROR("print group file failed!");
			clear_setinfo();
			return -1;
		}
	}
	else {
		LOGERROR("load file failed!");
		clear_setinfo();
		return -1;
	}
	return 0;
}

int set_group(int id)
{
	int i;
	int set_linkcout = 0; 
	int set_linkcout_c4 = 0; 
	struct sgroup_info *setinfo = NULL;
	struct sgroup_info *setinfoc4 = NULL;

	LOGDEBUG("set group start");
	m_objSetGroup.gsize = m_nSetGroupInfoCout;
	m_objSetGroup.port_group = id;
	setinfo = m_objSetGroup.ginfo; 
	for (i = 0; i < m_nSetGroupInfoCout; i++) {
		set_linkcout += (setinfo + i)->linkarrays_size;
		if (set_linkcout > 255) {
			LOGERROR("set link num is out of range");
			clear_setinfo();
			return -1;
		}
	}

	m_objSetGroupC4.gsize = m_nSetGroupInfoCoutC4;
	setinfoc4 =  m_objSetGroupC4.ginfo;
	for (i = 0; i < m_nSetGroupInfoCoutC4; i++) {
		set_linkcout_c4 += (setinfoc4 + i)->linkarrays_size;
		if (set_linkcout_c4 > 255) {
			LOGERROR("set link num is out of range");
			clear_setinfo();
			return -1;
		}
	}

	if (m_nSetGroupInfoCout > 0) {
		sgfplib_set_group(sgfp, &m_objSetGroup);
	}

	if (m_nSetGroupInfoCoutC4 > 0) {
		sgfplib_set_group(sgfp, &m_objSetGroupC4);
	}
	print_file(id);

	LOGDEBUG("set group end");
	return 0;
}

static int cascade_run(long instance, unsigned long data)
{
	int i = 0, j = 0, rc = 0;
	int curfiber = 0;
	char agingfile[64] = {0};
	struct fpga_board_runinfo_ex port_states;

	if (init_env(instance) < 0) {
		exit_env();
		return -1;
	}

	while (ap_is_running()) {
		for (j = 0; j < m_nFiberNum/4; j++) {
			memset(&port_states, 0, sizeof(struct fpga_board_runinfo_ex));
			port_states.start_port = j*4;
			rc = sgfplib_get_fpga_bd_runinfo_ex(sgfp, &port_states);
			if (rc) {
				LOGERROR("get state failed\n");
				continue;
			}
			for (i = curfiber; i < (j*4 + 4); ++i) {
				if ((port_states.ports[i%4].stm1_synced == 1)
						|| (port_states.ports[i%4].los == 1)) {
					curfiber++;
					continue;
				}
				get_slinkinfo(i);
				rc = convert_linkinfo();
				if (rc > 0) {
					LOGDEBUG("C4 valied");
				}
				else if (rc == 0) {
					study(j);
				}
				else {
					LOGDEBUG("no such channel!");
				}
				clear();
				curfiber++;
				usleep(1000);
			}
			sprintf(agingfile, "/tmp/fpga%d/agingfile_%d", g_fpgaid-1, j);
			put_new_group_file(agingfile, j*4);
			set_group(j);
			clear_setinfo();
		}
		break;
	}
	exit_env();
	return 0;
}

static struct ap_framework cascadeapp = {
	NULL,
	cascade_run,
	0ul,
	NULL,
	NULL,
	NULL,
	cascade_show_usage,
	NULL,
	cascade_parse_args
};

#if defined(__cplusplus)
extern "C"
{
#endif

struct ap_framework *register_ap(void)
{
	return &cascadeapp;
}

#if defined(__cplusplus)
}
#endif

