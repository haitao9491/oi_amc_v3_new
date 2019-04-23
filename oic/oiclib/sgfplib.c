/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * sgfplib.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include "os.h"
#include "oic.h"
#include "oiclib.h"
#include "sgfplib.h"
#include "fst.h"
#include "errno.h"

#define GET_CHSTAT_MASK			0x01
#define GET_CHSTAT_LOCAL_MASK	0x02
#define GET_CHSTAT_CFG_MASK		0x04
#define GET_CHSTAT_DB_MASK      0x08

struct sgfplib_hd {
	/*dev fd */
	int fd;
	FILE *std_out;
};

int sgfplib_get_fpga_bd_runinfo_ex(void *hd, struct fpga_board_runinfo_ex *rinfo)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

	return ioctl(ehd->fd, OIC_GET_FPGA_BD_RUNINFO_EX, rinfo);
}

int sgfplib_set_linkinfo_start(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

	return ioctl(ehd->fd, OIC_SET_FPGA_SLINK_INFO_START, NULL);
}

int sgfplib_get_linkinfo(void *hd, struct slink_info *link)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

    //PRINTINFO(hd, "get slink channel: %d\n", link->channel);
	return ioctl(ehd->fd, OIC_GET_FPGA_SLINK_INFO, link);
}

int sgfplib_get_hp_linkinfo(void *hd, struct fpga_board_runinfo_port_ex *link)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

	return ioctl(ehd->fd, OIC_GET_FPGA_SLINK_INFO_HP, link);
}

int sgfplib_set_linkinfo_end(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

	return ioctl(ehd->fd, OIC_SET_FPGA_SLINK_INFO_END, NULL);
}

#define TIMEOUT_MAX 5 
int sgfplib_set_try_group(void *hd, struct sgroup_all_info *gsinfo)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	unsigned int timeout = 0;
    int i = 0; 
    int ret = -1; 

	if (!hd)
		return -1;

    if (gsinfo->gsize < 1) {
        PRINTERR(hd, "gsize: %d err.\n", gsinfo->gsize);
        return -1;
    }
    if (gsinfo->ginfo == NULL) {
        PRINTERR(hd, "gsinfo->ginfo is NULL"); 
        return -1;
    }

    for (i = 0; i < gsinfo->gsize; i++) {
        if (gsinfo->ginfo[i].group_id > 255) {
            PRINTERR(hd, "gsinfo[%d].group_id %d err.", i, gsinfo->ginfo[i].group_id);
            continue;
        }
        if (gsinfo->ginfo[i].linkarrays_size < 1) {
            PRINTERR(hd, "gsinfo[%d].linkarrays_size %d err.", i, gsinfo->ginfo[i].linkarrays_size); 
            continue;
        }
    }

    ret = ioctl(ehd->fd, OIC_SET_FPGA_TRY_GROUP, gsinfo);
    if (ret != 0) {
        PRINTERR(hd, "try failed.");
        return -1;
    }

	sleep(1);
    do {
		ret = ioctl(ehd->fd, OIC_GET_FPGA_TRY_RESULT, gsinfo);
		if (ret == 0) {
			//PRINTINFO(hd, "get try result : %d\n", timeout);
			return 0;
		}

		if (ret < 0) {
			PRINTERR(hd, "get try result err.");
			return ret;
		} else if (ret == TRY_NO_RESULT) {
			sleep(1);
		} else {
			PRINTERR(hd, "return unkown %d.", ret);	
		}
		timeout++;
    } while (timeout < TIMEOUT_MAX);

	PRINTINFO(hd, "Failed get try no result timeout %d\n", timeout);
#if 0
    for(i = 0; i < gsinfo->gsize; i++) {
        PRINTINFO(hd, "g[%d] ok: 0x%x err: 0x%x\n", i, gsinfo->ginfo[i].resokpkt, gsinfo->ginfo[i].reserrpkt);
    }
#endif
	return 0; 
}

int sgfplib_set_group(void *hd, struct sgroup_all_info *gsinfo)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int i;

	if (!hd)
		return -1;

    if (gsinfo->gsize < 1) {
        PRINTERR(hd, "gsize: %d err.\n", gsinfo->gsize);
        return -1;
    }
    if (gsinfo->ginfo == NULL) {
        PRINTERR(hd, "gsinfo->ginfo is NULL\n");
        return -1;
    }

    for (i = 0; i < gsinfo->gsize; i++) {
        if (gsinfo->ginfo[i].group_id > 255) {
            PRINTERR(hd, "gsinfo[%d].group_id %d err.", i, gsinfo->ginfo[i].group_id); 
            continue;
        }
        if ((gsinfo->ginfo[i].linkarrays_size < 1) || (gsinfo->ginfo[i].is_valid == 0)) {
            PRINTERR(hd, "gsinfo[%d].linkarrays_size %d err OR is_valid %d.\n", i, gsinfo->ginfo[i].linkarrays_size, gsinfo->ginfo[i].is_valid); 
            continue;
        }
    }

	return ioctl(ehd->fd, OIC_SET_FPGA_GROUP, gsinfo);
}

#define TRY_MODE 1
#define SET_MODE 2
static int analysis_ram_info(void *hd, struct ram_info *rinfo, struct sgroup_all_info *sg_all, int mode)
{
    int i = 0;
    int j = 0;
    unsigned int group_addr = 0;
    unsigned int gsize = 0;
    unsigned int g_link_size[255];
    char r2val = 0x00;
    int gid = 0;

    if (mode == TRY_MODE) {
        memset(g_link_size, 0, sizeof(g_link_size));
        for (i = 0; i < 255; i++) {
            if (rinfo->ram1[i] == 0)
            continue;

            gsize++;
            g_link_size[i] = (rinfo->ram1[i] & 0xff0000) >> 16;
        }

        sg_all->gsize = gsize;
        sg_all->ginfo = (struct sgroup_info *)malloc(sizeof(struct sgroup_info) * gsize);
        memset(sg_all->ginfo, 0, sizeof(struct sgroup_info) * gsize);
        if (sg_all->ginfo == NULL) {
            PRINTERR(hd, "malloc failed.");
            return -1;
        }

        for (i = 0; i < gsize; i++) {
            group_addr = (rinfo->ram1[i] & 0xff00) >> 8;
            if (group_addr > 255) {
                PRINTERR(hd, "group_addr %d err.\n", group_addr); 
                continue;
            }

            sg_all->ginfo[i].is_valid = 1;
            sg_all->ginfo[i].linkarrays_size = g_link_size[i];
            if (sg_all->ginfo[i].linkarrays_size > 64) {
                PRINTERR(hd, "linkarrays_size %d err.\n", sg_all->ginfo[i].linkarrays_size);
                continue;
            }
            sg_all->ginfo[i].linkarrays = (struct slink_info *)malloc((sizeof(struct slink_info) * g_link_size[i]));
            if (sg_all->ginfo[i].linkarrays == NULL) {
                PRINTERR(hd, "malloc failed.");
                return -1;
            }
            memset(sg_all->ginfo[i].linkarrays, 0, sizeof(struct slink_info) * g_link_size[i]);

            for(j = 0; j < sg_all->ginfo[i].linkarrays_size; j++) {
                r2val = rinfo->ram2[group_addr + j];
                sg_all->ginfo[i].linkarrays[j].fiber = (r2val & 0xc0) >> 6;
                sg_all->ginfo[i].linkarrays[j].fiber_rate = (rinfo->ram1[i] & 0x78000000) >> 27 ;
                sg_all->ginfo[i].linkarrays[j].channel = r2val & 0x3f;
                sg_all->ginfo[i].linkarrays[j].channel_rate = (rinfo->ram1[i] & 0x6000000) >> 25;
                //PRINTINFO(hd, "[%d] [%d] %d %d %d %d\n", i, j, sg_all->ginfo[i].linkarrays[j].fiber, sg_all->ginfo[i].linkarrays[j].fiber_rate, sg_all->ginfo[i].linkarrays[j].channel, sg_all->ginfo[i].linkarrays[j].channel_rate);
            }
        }
    } else if (mode == SET_MODE) {
        for (i = 0; i < 255; i++) {
            if (rinfo->ram1_c4[i]) { 
                gsize++;
            }
            if (rinfo->ram1_c12[i]) {
                gsize++;
            }
        }
        sg_all->gsize = gsize;
        sg_all->ginfo = (struct sgroup_info *)malloc(sizeof(struct sgroup_info) * gsize);
        memset(sg_all->ginfo, 0, sizeof(struct sgroup_info) * gsize);
        if (sg_all->ginfo == NULL) {
            PRINTERR(hd, "malloc failed.");
            return -1;
        }
        //PRINTINFO(hd,"group size : %d \n", gsize);

        gid = 0;
        for (i = 0; i < gsize; i++) {
            if (rinfo->ram1_c4[i]) {
                group_addr = (rinfo->ram1_c4[i] & 0xff00) >> 8;
                if (group_addr > 255) {
                    PRINTERR(hd, "group_addr %d err.\n", group_addr); 
                    continue;
                }
                sg_all->ginfo[gid].is_valid = 1;
                sg_all->ginfo[gid].linkarrays_size = (rinfo->ram1_c4[i] & 0xff0000) >> 16;
                sg_all->ginfo[gid].linkarrays = (struct slink_info *)malloc((sizeof(struct slink_info) * sg_all->ginfo[gid].linkarrays_size));
                if (sg_all->ginfo[gid].linkarrays == NULL) {
                    PRINTERR(hd, "malloc failed.");
                    return -1;
                }
                memset(sg_all->ginfo[gid].linkarrays, 0, sizeof(struct slink_info) * sg_all->ginfo[gid].linkarrays_size);
                for(j = 0; j < sg_all->ginfo[i].linkarrays_size; j++) {
                    r2val = rinfo->ram2_c4[group_addr + j];
                    sg_all->ginfo[gid].linkarrays[j].fiber = (r2val & 0xc0) >> 6;
                    sg_all->ginfo[gid].linkarrays[j].fiber_rate = (rinfo->ram1_c4[i] & 0x78000000) >> 27 ;
                    sg_all->ginfo[gid].linkarrays[j].channel = r2val & 0x3f;
                    sg_all->ginfo[gid].linkarrays[j].channel_rate = (rinfo->ram1_c4[i] & 0x6000000) >> 25;
                    //PRINTINFO(hd, "[%d] [%d] %d %d %d %d\n", i, j, sg_all->ginfo[gid].linkarrays[j].fiber, sg_all->ginfo[gid].linkarrays[j].fiber_rate, sg_all->ginfo[gid].linkarrays[j].channel, sg_all->ginfo[gid].linkarrays[j].channel_rate);
                }
                gid++;
            }

            if (rinfo->ram1_c12[i]) {
                group_addr = (rinfo->ram1_c12[i] & 0xff00) >> 8;
                if (group_addr > 255) {
                    PRINTERR(hd, "group_addr %d err.\n", group_addr); 
                    continue;
                }
                sg_all->ginfo[gid].is_valid = 1;
                sg_all->ginfo[gid].linkarrays_size = (rinfo->ram1_c12[i] & 0xff0000) >> 16;
                sg_all->ginfo[gid].linkarrays = (struct slink_info *)malloc((sizeof(struct slink_info) * sg_all->ginfo[gid].linkarrays_size));
                if (sg_all->ginfo[gid].linkarrays == NULL) {
                    PRINTERR(hd, "malloc failed.");
                    return -1;
                }
                memset(sg_all->ginfo[gid].linkarrays, 0, sizeof(struct slink_info) * sg_all->ginfo[gid].linkarrays_size);
                for(j = 0; j < sg_all->ginfo[gid].linkarrays_size; j++) {
                    r2val = rinfo->ram2_c12[group_addr + j];
                    sg_all->ginfo[gid].linkarrays[j].fiber = (r2val & 0xc0) >> 6;
                    sg_all->ginfo[gid].linkarrays[j].fiber_rate = (rinfo->ram1_c12[i] & 0x78000000) >> 27 ;
                    sg_all->ginfo[gid].linkarrays[j].channel = r2val & 0x3f;
                    sg_all->ginfo[gid].linkarrays[j].channel_rate = (rinfo->ram1_c12[i] & 0x6000000) >> 25;
                    //PRINTINFO(hd, "[%d] [%d] %d %d %d %d\n", i, j, sg_all->ginfo[gid].linkarrays[j].fiber, sg_all->ginfo[gid].linkarrays[j].fiber_rate, sg_all->ginfo[gid].linkarrays[j].channel, sg_all->ginfo[gid].linkarrays[j].channel_rate);
                }
                gid++;
            }
        }
    } else {
        return -1; 
    }
    return 0;
}

int sgfplib_get_try_group(void *hd, struct sgroup_all_info *sg)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	struct ram_info rinfo;
    int ret = -1;

	if (!hd)
		return -1;

	memset(&rinfo, 0, sizeof(struct ram_info));
    ret = ioctl(ehd->fd, OIC_GET_FPGA_TRY_GROUP, &rinfo);
    if(ret < 0) {
        PRINTERR(hd, "ioctl OIC_GET_FPGA_TRY_GROUP err.");
        return -1;
    }

	analysis_ram_info(hd, &rinfo, sg, 1);
    return 0;
}

int sgfplib_get_group(void *hd, struct sgroup_all_info *sg)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	struct ram_info rinfo;
    int ret = -1;

	if (!hd)
		return -1;

	memset(&rinfo, 0, sizeof(struct ram_info));
    ret = ioctl(ehd->fd, OIC_GET_FPGA_GROUP, &rinfo);
    if(ret < 0) {
        PRINTERR(hd, "ioctl OIC_GET_FPGA_TRY_GROUP err.");
        return -1;
    }

	analysis_ram_info(hd, &rinfo, sg, 2);
    return 0;
}


int sgfplib_get_hdlc_chstat(void *hd, int mask, struct hdlc_port_stat *chstat)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int ret = -1;

	if (!hd)
		return -1;

	if (mask & GET_CHSTAT_MASK) {
		ret = ioctl(ehd->fd, OIC_GET_FPGA_HDLC_CHSTAT, chstat);
	} else if (mask & GET_CHSTAT_LOCAL_MASK) {
		ret = ioctl(ehd->fd, OIC_GET_FPGA_HDLC_LOCAL_SCHAN, chstat);
	} else if (mask & GET_CHSTAT_CFG_MASK) {
		ret = ioctl(ehd->fd, OIC_GET_FPGA_HDLC_CONFIG_SCHAN, chstat);
	} else if (mask & GET_CHSTAT_DB_MASK) {
		ret = ioctl(ehd->fd, OIC_GET_FPGA_HDLC_DB_STATUS, chstat);
	} else {
		PRINTERR(hd, " mask %d unkown.\n", mask);	
		return ret;
	}
	if (ret < 0) {
		PRINTERR(hd, "mask %d ioctl failed.\n", mask);	
	}

	return ret;
}

int sgfplib_set_hdlc_cfg(void *hd, struct hdlc_cfg *hcfg)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int ret = -1;

	if (!hd)
		return -1;

    ret = ioctl(ehd->fd, OIC_SET_FPGA_HDLC_CFG, hcfg);
	if(ret < 0) {
        PRINTERR(hd, "ioctl OIC_SET_FPGA_HDLC_CFG err.");
        return -1;
    }

	return 0;
}

int sgfplib_get_hdlc_ge_stat(void *hd, struct bd_ge_stat *gstat)
{
    struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int ret = -1;

	if (!hd)
		return -1;

    ret = ioctl(ehd->fd, OIC_GET_FPGA_HDLC_GE_STAT, gstat);
	if(ret < 0) {
        PRINTERR(hd, "ioctl OIC_GET_FPGA_HDLC_GE_STAT err.");
        return -1;
    }

	return 0;
}

int sgfplib_set_spu_selnum(void *hd, unsigned char selnum)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	int rc = 0;
	
	if (!hd)
		return -1;

	rc = ioctl(ehd->fd, OIC_SET_GFP_FPGA_SPU_SELNUM, &selnum);
	if (rc != 0) {
		printf("ioctl(set spu_selnum): %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int sgfplib_get_spu_selnum(void *hd, unsigned char *value)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	int rc = 0;
	
	if (!hd || !value)
		return -1;
	rc = ioctl(ehd->fd, OIC_GET_GFP_FPGA_SPU_SELNUM, value);
	if (rc != 0) {
		printf("ioctl(get spu_selnum): %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
int sgfplib_set_spu_forward_enable(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	int rc = 0;

		if (!hd)
		return -1;
	rc = ioctl(ehd->fd, OIC_SET_GFP_FPGA_SPU_FORWARD_ENABLE, NULL);
	if (rc != 0) {
		printf("ioctl(set spu_forward_enable): %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
int sgfplib_set_spu_forward_disable(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	int rc = 0;

		if (!hd)
		return -1;
	rc = ioctl(ehd->fd, OIC_SET_GFP_FPGA_SPU_FORWARD_DISABLE, NULL);
	if (rc != 0) {
		printf("ioctl(set spu_forward_disable): %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
int sgfplib_set_spu_forward_rule(void *hd, struct spu_forward_rule rule)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	int rc = 0;
	if (!hd)
		return -1;
	rc = ioctl(ehd->fd, OIC_SET_GFP_FPGA_SPU_FORWARD_RULE, &rule);
	if (rc != 0) {
		printf("ioctl(set spu_forward_rule): %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
int sgfplib_set_spu_channel_forward(void *hd, struct chan_flag chan)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
	int rc = 0;
	if (!hd)
		return -1;
	rc = ioctl(ehd->fd, OIC_SET_GFP_FPGA_SPU_CHANNEL_FORWARD, &chan);
	if (rc != 0) {
		printf("ioctl(set spu_channel_forward): %s\n", strerror(errno));
		return -1;
	}
	return 0;
}

int sdhlib_get_sdh_fpga_stat(void *hd, struct sdh_fpga_stat *stat)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

	if (ioctl(ehd->fd, OIC_SET_SDH_GET_BOARD_STAT, stat) != 0) {
        return -1;
    }
    return 0;
}

int sdhlib_set_board_stat_clear(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int val = SDH_BOARD_STAT_TYPE_CLEAR;

	if (!hd)
		return -1;

    if (ioctl(ehd->fd, OIC_GET_SDH_SET_BOARD_STAT_TYPE, &val) != 0) {
        return -1;
    }
    return 0;
}

int sdhlib_set_board_stat_pkts(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int val = SDH_BOARD_STAT_TYPE_PKTS;

	if (!hd)
		return -1;

    if (ioctl(ehd->fd, OIC_GET_SDH_SET_BOARD_STAT_TYPE, &val) != 0) {
        return -1;
    }
    return 0;
}

int sdhlib_set_board_stat_bytes(void *hd)
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;
    int val = SDH_BOARD_STAT_TYPE_BYTES;

	if (!hd)
		return -1;

    if (ioctl(ehd->fd, OIC_GET_SDH_SET_BOARD_STAT_TYPE, &val) != 0) {
        return -1;
    }
    return 0;
}

void *sgfplib_open(int fpgadev)
{
	struct sgfplib_hd *hd = NULL;
	char fname[32];

	hd = (struct sgfplib_hd *)malloc(sizeof(struct sgfplib_hd));
	if (!hd) {
		PRINTERR(hd, "malloc hd failed!\n");
		return NULL;
	}
	memset(hd, 0, sizeof(*hd));

	hd->std_out = stdout;
	memset(fname, 0, sizeof(fname));
    if (fpgadev == 0) {
        sprintf(fname, "/dev/%s", OIC_NAME);
    } else {
        sprintf(fname, "/dev/%s%d", OIC_NAME, fpgadev);
    }

	hd->fd = open(fname, O_RDWR);
	if (hd->fd < 0) {
		PRINTERR(hd, "open %s device failed!\n", fname);
		goto sgfplib_free_exit;
	}
	
	return hd;

sgfplib_free_exit:
	free(hd);
	return NULL;
}

int sgfplib_close(void *hd) 
{
	struct sgfplib_hd *ehd = (struct sgfplib_hd *)hd;

	if (!hd)
		return -1;

	if (ehd->fd)
		close(ehd->fd);
	free(ehd);

	return 0;
}
