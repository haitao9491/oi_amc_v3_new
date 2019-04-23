/*
 * (C) Copyright 2019
 * liye <ye.li@raycores.com>
 *
 * gfplib.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <ctype.h>
#include "oic.h"
#include "gfplib.h"

#define DEBUG
struct gfplib_hd {
    uint8_t fpgaid;
    int     fd;
};
FILE    *gfplogfd = NULL;

#if defined(DEBUG)
#define GFP_LOGERROR(fmt, args...)		fprintf(gfplogfd, "[ERROR] " fmt "\n", ##args);fflush(gfplogfd);
#define GFP_LOGINFO(fmt, args...)		fprintf(gfplogfd, "[INFO] " fmt "\n", ##args);fflush(gfplogfd);
#define GFP_LOGDEBUG(fmt, args...)		fprintf(gfplogfd, "[DEBUG] " fmt "\n", ##args);fflush(gfplogfd);
#else
#define GFP_LOGERROR(fmt, args...)		do { } while (0)
#define GFP_LOGINFO(fmt, args...)		do { } while (0)
#define GFP_LOGDEBUG(fmt, args...)		do { } while (0)
#endif

#define TIMEOUT_CNT 10

int gfplib_get_au4status(void *hd, uint8_t au4, struct gfp_au4status *status)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;
    struct fpga_board_runinfo_port_ex pinfo;

    if (gfp_hd == NULL) {
        GFP_LOGERROR("gfp_hd is null."); 
        return -1;
    }


    memset(&pinfo, 0, sizeof(&pinfo));
    pinfo.fiber = au4 - 1;
    if (ioctl(gfp_hd->fd, OIC_GET_SDH_GET_AU4_STATUS, &pinfo) != 0) {
        GFP_LOGERROR("ioctl get au4 status failed.");
        return -1;
    }

    status->los = pinfo.los;
    status->lof = pinfo.stm1_synced;

    return 0;
}

int gfplib_get_linkinfo_begin(void *hd)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;

    if (!gfp_hd)
        return -1;

	return ioctl(gfp_hd->fd, OIC_SET_FPGA_SLINK_INFO_START, NULL);
}

int gfplib_get_linkinfo(void *hd, uint8_t au4, uint8_t e1, struct gfp_linkinfo *info)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;
    struct gfp_local2global map;
    struct slink_info slink;

    if (!gfp_hd)
        return -1;

    memset(&map, 0, sizeof(&map));
    map.local_au4 = au4 - 1;
    map.local_e1 = e1 - 1;

    if (ioctl(gfp_hd->fd, OIC_GET_SDH_CHANNEL2GLOBAL, &map) != 0) {
        GFP_LOGERROR("ioctl get channel to global error.");
        return -1;
    }

    info->chassisid = map.chassisid;
    info->slot = map.slot;
    info->port = map.port;
    info->au4 = map.au4;
    info->e1 = map.e1;

    memset(&slink, 0, sizeof(&slink));
    slink.fiber = au4 - 1;
    slink.channel = e1 - 1;

    if (ioctl(gfp_hd->fd, OIC_GET_FPGA_SLINK_INFO, &slink) != 0) {
        GFP_LOGERROR( "ioctl get fpga slink info error.");
        return -1;
    }

    info->e1_rate = slink.channel_rate;
    info->vc_valid = slink.vc_valid;
    info->is_lcas = slink.is_lcas;
    info->is_member = slink.is_member;
    info->is_last_member = slink.is_last_member;
    info->mfi = slink.mfi;
    info->sq = slink.sq;
    info->pre_gid = slink.pre_gid;
    info->cur_gid = slink.cur_gid;
    info->svc_type = slink.svc_type;

    return 0;
}

int gfplib_get_linkinfo_end(void *hd)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;

    if (!gfp_hd)
        return -1;

	return ioctl(gfp_hd->fd, OIC_SET_FPGA_SLINK_INFO_END, NULL);
}

static int data2driver(char *type, struct gfp_groups *data, struct sgroup_all_info *drv)
{
    uint8_t gsize = data->groups_num;
    uint8_t lsize = 0;
    int sum_linksnum = 0;
    int i = 0;
    int j = 0;

    //GFP_LOGINFO("line[%d] groups num %d.", __LINE__, gsize);

    drv->ginfo = (struct sgroup_info *)malloc(sizeof(struct sgroup_info) * gsize);
    memset(drv->ginfo, 0, sizeof(struct sgroup_info));

    for(i = 0; i < gsize; i++) {
        lsize = data->groups[i].links_num;

        drv->ginfo[i].linkarrays_size = lsize;
        if ((lsize < 1) || (lsize > 64)) {
            GFP_LOGERROR("line[%d] group[%d] links_num %d err.", __LINE__, (i+1), lsize);
            return -1;
        }
        drv->ginfo[i].linkarrays = (struct slink_info *)malloc(sizeof(struct slink_info) * lsize);
        memset(drv->ginfo[i].linkarrays, 0, sizeof(struct slink_info) * lsize);

        if (!strcmp(type, "try")) {
            drv->ginfo[i].group_id = data->groups[i].id;
        } else {
            drv->ginfo[i].group_id = ((data->groups[i].links[0].au4 - 1) & 0x3f) << 6;
            drv->ginfo[i].group_id |= ((data->groups[i].links[0].e1- 1) & 0x3f) << 0;
        }

        if (drv->ginfo[i].linkarrays == NULL) {
            GFP_LOGERROR("group[%d] linkarrays malloc error.", (i+1));
            continue;
        }
        drv->ginfo[i].is_valid = 1;

        for(j = 0; j < lsize; j++) {
            drv->ginfo[i].linkarrays[j].fiber = data->groups[i].links[j].au4 - 1;
            drv->ginfo[i].linkarrays[j].channel = data->groups[i].links[j].e1 - 1;
            /* fiber rate default C4, channel rate default C12 */
            drv->ginfo[i].linkarrays[j].fiber_rate = 0;
            drv->ginfo[i].linkarrays[j].channel_rate = 2;
        }
        sum_linksnum += lsize;
    }

    if (!strcmp(type, "try")) {
        if (sum_linksnum > GROUP_TRIAL_LINK_MAXNUM) {
            GFP_LOGERROR("try sum_linksnum is err.");
            return -1;
        }
        //GFP_LOGINFO("try sum_linksnum is ok.");

    } else {
        if (sum_linksnum > GROUP_FORMAL_LINK_MAXNUM) {
            GFP_LOGERROR("set sum_linksnum is err.");
            return -1;
        }
        //GFP_LOGINFO("set sum_linksnum is ok.");
    }

    return 0;
}

void driver2data(struct gfp_trial_results *results, struct sgroup_all_info *gsinfo)
{
    int i = 0;

    results->groups_num = gsinfo->gsize;
    for (i = 0; i < gsinfo->gsize; i++) {
        results->results[i].id = gsinfo->ginfo[i].group_id;
        results->results[i].succs_pkts = gsinfo->ginfo[i].resokpkt;
        results->results[i].error_pkts = gsinfo->ginfo[i].reserrpkt;
#if 0
        GFP_LOGDEBUG("gfp trial results num %d group[%d] id 0x%x ok %d err %d", results->groups_num, (i+1), \
                     results->results[i].id, results->results[i].succs_pkts, results->results[i].error_pkts);
#endif
    }
}

int gfplib_set_trial_groups(void *hd, struct gfp_groups *groups, struct gfp_trial_results *results)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;
    struct sgroup_all_info *gsinfo = NULL;
    int timeout_cnt = 0;
    int rv = -1;
    int i = 0;

    if (!gfp_hd)
        return -1;

    if (groups->groups_num < 1) {
        GFP_LOGERROR( "groups null num is %d .\n", groups->groups_num);
        return 0;
    }

    if (groups->groups_num > GROUPS_TRIAL_GROUP_MAXNUM) {
        GFP_LOGERROR("groups max num is %d, but num is %d.\n", GROUPS_GROUP_MAXNUM, groups->groups_num);
        return -1;
    }

//    GFP_LOGINFO( "trial groups: groups num %d\n", groups->groups_num);

    gsinfo = (struct sgroup_all_info *)malloc(sizeof(struct sgroup_all_info));
    if (gsinfo == NULL) {
        GFP_LOGERROR("LINE[%d] malloc error.", __LINE__);
        return -1;
    }
    gsinfo->gsize = groups->groups_num;
    if (data2driver("try", groups, gsinfo) != 0) {
        return -1;
    }

#if 0
    for (i = 0; i < gsinfo->gsize; i++) {
        GFP_LOGINFO("line[%d] groups[%d] links num [%d]", __LINE__, gsinfo->gsize, gsinfo->ginfo[i].linkarrays_size);

        for (j = 0; j < gsinfo->ginfo[i].linkarrays_size; j++) {

            GFP_LOGINFO("group[%d] links_num[%d] link[%d] %d-%d\n", \
                        i, gsinfo->ginfo[i].linkarrays_size, j, gsinfo->ginfo[i].linkarrays[j].fiber, gsinfo->ginfo[i].linkarrays[j].channel);
        }
    }
#endif

    if (ioctl(gfp_hd->fd, OIC_SET_FPGA_TRY_GROUP, gsinfo) != 0) {
        GFP_LOGERROR( "ioctl failed to set fpga try group\n");
        goto trial_group_exit;
    }

    /* wait try result by TIMEOUT_CNT */
    timeout_cnt = TIMEOUT_CNT;
//    sleep(1);

    rv = -1;
    do {
        rv = ioctl(gfp_hd->fd, OIC_GET_FPGA_TRY_RESULT, gsinfo);

		if (rv == TRY_NO_RESULT) {
			//GFP_LOGINFO("time[%d] get try result is null.", timeout_cnt);
            usleep(10000);

        } else if (rv == 0) {
            driver2data(results, gsinfo);
            goto trial_group_exit;

        } else if (rv < 0) {
			GFP_LOGERROR("get try result err.");
            goto trial_group_exit;

		} else {
			GFP_LOGERROR("return unkown %d.", rv);
		}

        timeout_cnt--;
    } while (timeout_cnt);

trial_group_exit:

    for(i = 0; i < gsinfo->gsize; i++) {
        if (gsinfo->ginfo[i].linkarrays)
            free(gsinfo->ginfo[i].linkarrays);
    }

    if (gsinfo->ginfo)
        free(gsinfo->ginfo);

    if (gsinfo)
        free(gsinfo);

    return rv;
}

int gfplib_set_groups(void *hd, struct gfp_groups *groups)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;
    struct sgroup_all_info *gsinfo = NULL;
    int rv = 0;
    int i = 0;

    if (!gfp_hd)
        return -1;

    if (groups->groups_num < 1) {
        GFP_LOGERROR( "groups null num is %d .\n", groups->groups_num);
        return 0;
    }

    if (groups->groups_num > GROUP_FORMAL_LINK_MAXNUM) {
        GFP_LOGERROR("groups max num is %d, but num is %d.\n", GROUPS_FORMAL_GROUP_MAXNUM, groups->groups_num);
        return -1;
    }

    GFP_LOGINFO("set groups: groups num %d\n", groups->groups_num);
    gsinfo = (struct sgroup_all_info *)malloc(sizeof(struct sgroup_all_info));
    if (gsinfo == NULL) {
        GFP_LOGERROR("LINE[%d] malloc error.", __LINE__);
        return -1;
    }

    gsinfo->gsize = groups->groups_num;
    if (data2driver("set", groups, gsinfo) != 0) {
        rv = -1;
        goto set_group_exit;
    }

    if (ioctl(gfp_hd->fd, OIC_SET_FPGA_GROUP, gsinfo) != 0) {
        rv = -1;
        GFP_LOGERROR( "ioctl failed to set fpga format group\n");
        goto set_group_exit;
    }

set_group_exit:

    for(i = 0; i < gsinfo->gsize; i++) {
        if (gsinfo->ginfo[i].linkarrays)
            free(gsinfo->ginfo[i].linkarrays);
    }

    if (gsinfo->ginfo)
        free(gsinfo->ginfo);

    if (gsinfo)
        free(gsinfo);

    return rv;
}

int gfplib_channel_local2global(void *hd, uint8_t local_au4, uint8_t local_e1, struct gfp_linkinfo *info)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;
    struct gfp_local2global map;

    if (!gfp_hd)
        return -1;

    memset(&map, 0, sizeof(&map));
    map.local_au4 = local_au4 - 1;
    map.local_e1 = local_e1 - 1;

    if (ioctl(gfp_hd->fd, OIC_GET_SDH_CHANNEL2GLOBAL, &map)) {
        GFP_LOGERROR("ioctl get sdh channel to global error. local au4[%d]e1[%d]", local_au4, local_e1);
        return -1;
    }
#if 0
    GFP_LOGINFO("line[%d] local2global au4 %d e1 %d -- src chassisid[%d] slot[%d] port[%d] au4[%d] e1[%d]", \
                __LINE__, local_au4, local_e1, map.chassisid, map.slot, map.port, map.au4, map.e1);
#endif

    info->chassisid = map.chassisid;
    info->slot = map.slot;
    info->port = map.port;
    info->au4 = map.au4;
    info->e1 = map.e1;

    return 0;
}

void *gfplib_open(uint8_t fpgaid)
{
	struct gfplib_hd *hd = NULL;
    char logfile[32];

    memset(logfile, 0, sizeof(logfile));
    sprintf(logfile, "/tmp/gfp%dlib.log", fpgaid);
    gfplogfd = fopen(logfile, "a+");
    if (gfplogfd == NULL) {
        fprintf(stdout,"gfplib fopen %s failed.", logfile);
        fflush(stdout);
        return hd;
    }

    hd = (struct gfplib_hd *)malloc(sizeof(struct gfplib_hd));
	if (!hd) {
        GFP_LOGERROR("gfp open malloc failed.");
		return NULL;
	}
    memset(hd, 0, sizeof(struct gfplib_hd));

    switch (fpgaid) {
        case 1:
            hd->fd = open("/dev/oicdev1", O_RDWR);
        break;

        case 2:
            hd->fd = open("/dev/oicdev2", O_RDWR);
         break;

        case 3:
            hd->fd = open("/dev/oicdev3", O_RDWR);
         break;

        case 4:
            hd->fd = open("/dev/oicdev4", O_RDWR);
         break;

        default:
            GFP_LOGERROR("Unkown fpgaid %d.", fpgaid);
            goto err_exit;
    }

    if (hd->fd < 0) {
        GFP_LOGERROR("failed to open oic.");
        goto err_exit;
    }

	return hd;

err_exit:
    free(hd);
    return hd;
}

int gfplib_close(void *hd)
{
    struct gfplib_hd *gfp_hd = (struct gfplib_hd *) hd;

    if (gfplogfd)
        fclose(gfplogfd);

    if (gfp_hd->fd)
        close(gfp_hd->fd);

    if (hd)
        free(hd);

	return 0;
}
