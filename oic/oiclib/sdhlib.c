/*
 * (C) Copyright 2018
 * liye <ye.li@raycores.com>
 *
 * sdhlib.c - A description goes here.
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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "oiclib.h" 
#include "sdhlib.h" 
#include "sgfplib.h" 
#include "oic.h" 
#include "cloves.h"
#include "cconfig.h"
#include "bcm-opt.h"

#define DEBUG

FILE *logfd = NULL;
#if defined(DEBUG)
#define SDH_LOGERROR(fmt, args...)		fprintf(logfd, "[ERROR] " fmt "\n", ##args);fflush(logfd);
#define SDH_LOGINFO(fmt, args...)		fprintf(logfd, "[INFO] " fmt "\n", ##args);fflush(logfd);
#define SDH_LOGDEBUG(fmt, args...)		fprintf(logfd, "[DEBUG] " fmt "\n", ##args);fflush(logfd);
#else
#define SDH_LOGERROR(fmt, args...)		do { } while (0)
#define SDH_LOGINFO(fmt, args...)		do { } while (0)
#define SDH_LOGDEBUG(fmt, args...)		do { } while (0)
#endif

#define FPGA0_DEV_NAME      "/dev/oicdev1"
#define FPGA1_DEV_NAME      "/dev/oicdev2"
#define FPGA2_DEV_NAME      "/dev/oicdev3"
#define FPGA3_DEV_NAME      "/dev/oicdev4"

#define SDHLIB_LOGFILE  "/tmp/sdhlib.log"
#define SDHLIB_LOGLEVEL LGWRLEVELINFO
#define SDHLIB_LOGSIZE  1024 

#define COMPILER_64_SET(dst, src_hi, src_lo)                \
    ((dst) = (((uint64_t) ((uint32_t)(src_hi))) << 32) | ((uint64_t) ((uint32_t)(src_lo)))

#define COMPILER_64_SUB_64(dst, src)    ((dst) -= (src))


struct sdhlib_stat {
    uint64_t cnt_inner;
    uint64_t cnt_local;
    uint64_t cnt_global;
    uint64_t cnt_2muser;
    uint64_t cnt_linkmap;
    uint64_t cnt_64kppp;
    uint64_t cnt_gfp;
};

struct sdhlib_sdh_hd {
    int fd0;
    int fd1;
    int fd2;
    int fd3;
    int swfd;
    struct board_info bdinfo;
    struct sdhlib_stat stat;
};

struct sdhlib_sdh_hd *sdh_hd = NULL;
int cur_fd = -1;
char logfile[512];

int port_conv_fpgaid(uint8_t hp_port, uint8_t *fpgaid)
{
    switch(hp_port) {
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
        case 8:
            *fpgaid = 1;
            break;

        case 3:
        case 4:
        case 9:
        case 10:
        case 11:
        case 12:
            *fpgaid = 2;
            break;

        case 13:
        case 14:
        case 17:
        case 18:
        case 19:
        case 20:
            *fpgaid = 3;
            break;

        case 15:
        case 16:
        case 21:
        case 22:
        case 23:
        case 24:
            *fpgaid = 4;
            break;

        default:
            return -1;
    }
    return 0;
}

int fpgaid_fd_map(uint8_t fpgaid)
{
    if (!sdh_hd)
        return -1;

    cur_fd = -1;
    switch(fpgaid) {
        case 1:
            if (sdh_hd->fd0 < 0) {
                SDH_LOGERROR( "Failed to open %s \n", FPGA0_DEV_NAME);
                return -1;
            }
            cur_fd = sdh_hd->fd0;
            break;

        case 2:
            if (sdh_hd->fd1 < 0) {
                SDH_LOGERROR( "Failed to open %s \n", FPGA1_DEV_NAME);
                return -1;
            }
            cur_fd = sdh_hd->fd1;
            break;

        case 3:
            if (sdh_hd->fd2 < 0) {
                SDH_LOGERROR( "Failed to open %s \n", FPGA2_DEV_NAME);
                return -1;
            }
            cur_fd = sdh_hd->fd2;
            break;

        case 4:
            if (sdh_hd->fd3 < 0) {
                SDH_LOGERROR( "Failed to open %s \n", FPGA3_DEV_NAME);
                return -1;
            }
            cur_fd = sdh_hd->fd3;
            break;

        default:
            cur_fd = -1;
            SDH_LOGERROR( "fpgaid %d err.\n", fpgaid);
            return -1;
    }

    return 0;
}

int set_inner_rule(uint8_t enable, uint8_t hp_port, uint8_t hp_chan, uint8_t sel, uint8_t lp_chan)
{
    uint8_t fpgaid = 0;
    struct sdh_inner_rule rule;

    if (port_conv_fpgaid(hp_port, &fpgaid) != 0) {
        SDH_LOGERROR( "hp port %d err.\n", hp_port);
        return -1;
    }

    if (sdh_hd->bdinfo.fpgas[fpgaid-1].type != FPGA_TYPE_HP) {
        SDH_LOGERROR("fpgaid %d is not hp\n", fpgaid);
        return -1;
    }

    if (fpgaid_fd_map(fpgaid) != 0) {
        SDH_LOGERROR("fpgaid %d map err\n", fpgaid);
        return -1;
    }

    memset(&rule, 0, sizeof(&rule));

    switch (hp_port)
    {
        case 1:
        case 3:
        case 13:
        case 15:
            hp_port = 1;
            break;

        case 2:
        case 4:
        case 14:
        case 16:
            hp_port = 2;
            break;

        default:
            SDH_LOGERROR("hp port %d  err\n", hp_port);
            return -1;
    }
    rule.src_port = (hp_port - 1);
    rule.src_channel = (hp_chan - 1);
    rule.enable = enable;
    rule.dst_sel = sel;
    rule.dst_channel = (lp_chan - 1);

    if (ioctl(cur_fd, OIC_SET_SDH_INNER_RULE, &rule) != 0) {
        SDH_LOGERROR( "Failed ioctl set inner rule .\n");
        return -1;
    }

    sdh_hd->stat.cnt_inner++;
    SDH_LOGDEBUG("[%llu]inner src %d-%d dst-sel[%d] %d enable[%d]", \
             sdh_hd->stat.cnt_inner, rule.src_port, rule.src_channel, \
             rule.dst_sel, rule.dst_channel, rule.enable);

    return 0;
}

int sdhlib_add_inner_sw_rule(uint8_t hp_port, uint8_t hp_chan, uint8_t sel, uint8_t lp_chan)
{
    if (!sdh_hd)
        return -1;

    if (set_inner_rule(1, hp_port, hp_chan, sel, lp_chan) != 0) {
        SDH_LOGERROR( "Failed to set inner rule enable. \n");
        return -1;    
    }
    return 0;
}

int sdhlib_get_sdh_status(void)
{
    if (!sdh_hd)
        return -1;

    return 0;
}

int sdhlib_get_board_stat(struct board_stat *stat)
{
    struct sdh_fpga_stat fstat;
	uint8_t i = 0;

    if (!sdh_hd)
        return -1;

    memset(stat, 0, sizeof(struct board_stat));
    stat->fpgas_num = BOARD_FPGA_MAXNUM;
	for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {
        memset(&fstat, 0, sizeof(&fstat));

		if (fpgaid_fd_map((i + 1)) != 0)
			return -1;

		if (cur_fd < 0)
			return 0;

		if (ioctl(cur_fd, OIC_SET_SDH_GET_BOARD_STAT, &fstat) != 0) {
			SDH_LOGERROR("ioctl get sdh get board stat failed.");
			return -1;
		}

        stat->fpgas[i].id = (i + 1);
        stat->fpgas[i].pos_rpkts = fstat.ppp_cnt;
        stat->fpgas[i].gfp_rpkts = fstat.gfp_cnt;
        stat->fpgas[i].laps_rpkts = fstat.laps_cnt;
        stat->fpgas[i].atm_rpkts = fstat.atm_cnt;
        stat->fpgas[i].tug_rpkts = fstat.hp2lp_cnt;
        stat->fpgas[i].private_tpkts = fstat.hp2lp_cnt;
        stat->fpgas[i].e1_rpkts = fstat.e1user_cnt;
        stat->fpgas[i].ether_tpkts = fstat.sw_eth0 + fstat.sw_eth1 + fstat.sw_eth2 + fstat.sw_eth3;
        stat->fpgas[i].hdlc_rpkts = 0;
	}
	return 0;
}

int sdhlib_set_local_rule(struct proto_local_rule *rule)
{
    struct sdh_local_rule lrule;
    uint8_t i = 0;


    if (!sdh_hd)
        return -1;

    if (fpgaid_fd_map(rule->fpgaid) != 0) {
        return -1;
    }

    memset(&lrule, 0,sizeof(&lrule));
    lrule.proto = rule->protocol;

    for (i = 0; i < 4; i++) {
        if (rule->pbmp & (1 << i)) {
            lrule.sel[lrule.valid_num] = i;
            lrule.valid_num++;
        }
    }
    if (ioctl(cur_fd, OIC_SET_SDH_LOCAL_RULE, &lrule) != 0) {
        SDH_LOGERROR( "Failed ioctl set local rule .\n");
        return -1;
    }

    sdh_hd->stat.cnt_local++;
    SDH_LOGDEBUG("[%llu]local : fpgaid %d proto %d sels_pbmp 0x%x valid_num %d sel %d %d %d %d", \
             sdh_hd->stat.cnt_local, rule->fpgaid, rule->protocol, rule->pbmp, \
             lrule.valid_num, lrule.sel[0], lrule.sel[1], lrule.sel[2], lrule.sel[3]);

    return 0;
}

int sdhlib_set_global_rule(struct proto_global_rule *rule)
{
    struct sdh_global_rule grule;
    uint8_t valid_num = 0;
    uint8_t i = 0;

    if (!sdh_hd)
        return -1;
    
    if (fpgaid_fd_map(rule->fpgaid) != 0) {
        return -1;
    }

    memset(&grule, 0, sizeof(&grule));
    grule.proto = rule->protocol;
    for (i = 0; i < GLOBAL_SEL_MAXNUM; i++) {
        if (rule->sels_bmp & (1 << i)) {
            grule.sel[valid_num] = rule->sels[i];
            valid_num++;
        }
    }

    grule.valid_num = valid_num;

    if (ioctl(cur_fd, OIC_SET_SDH_GLOBAL_RULE, &grule) != 0) {
        SDH_LOGERROR( "Failed ioctl set global rule .\n");
        return -1;
    }

    sdh_hd->stat.cnt_global++;
    SDH_LOGDEBUG("[%llu] global : fpgaid %d proto %d sels_bmp 0x%x valid_num %d", \
             sdh_hd->stat.cnt_global, rule->fpgaid, grule.proto, rule->sels_bmp, grule.valid_num);

    for (i = 0; i < grule.valid_num; i++) 
        SDH_LOGDEBUG("sel[%d]: %d", i, grule.sel[i]);

    return 0;
}


int sdhlib_del_inner_sw_rule(uint8_t hp_port, uint8_t hp_chan)
{
    if (!sdh_hd)
        return -1;

    if (set_inner_rule(0, hp_port, hp_chan, 0, 0) != 0) {
        SDH_LOGERROR( "Failed to set inner rule disable. \n");
        return -1;    
    }
    return 0;
}

int get_cpld_info(void)
{
	int fd;
	char fname[32];
	int val = 0;

	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", CLOVES_NAME);

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		return -1;
	}

	if (ioctl(fd, CLOVES_GET_AMC_PRESENT , &val) == 0) {
        close(fd);
        return val;
	}

    close(fd);
    return -1;
}

static int check_boardtype(char *boardname)
{
    char str1[16];
    char str2[16];

    memset(str1, 0, sizeof(str1));
    memset(str2, 0, sizeof(str2));

    sprintf(str2, "XSCB_V2");
    memcpy(str1, boardname, strlen(str2));
    if (!strcmp(str1, str2)) {
        //SDH_LOGDEBUG("board is XSCB_V2.");
        return BOARDTYPE_XSCB;
    }
 
    sprintf(str2, "MACB_V3");
    memcpy(str1, boardname, strlen(str2));
    if (!strcmp(str1, str2)) {
        //SDH_LOGDEBUG("board is MACB_V3.");
        return BOARDTYPE_MACB;
    }

    sprintf(str2, "OI_AMC_V3");
    memcpy(str1, boardname, strlen(str2));
    if (!strcmp(str1, str2)) {
        //SDH_LOGDEBUG("board is OI_AMC_V3.");
        return BOARDTYPE_OI_AMC;
    }
    SDH_LOGERROR("Unkown board type %s ", str1);
    return 0;
}

static int get_bdinfo_from_cfgfile(struct board_info *bdinfo)
{
    char devfile[] = "/etc/board.cfg";
    char buf[64];
    FILE *fp = NULL;
    char *p = NULL;
    char *result = NULL;

    if (access(devfile, 0)) {
        SDH_LOGERROR( "%s is not exisits.", devfile);
        return 0;
    }

    fp = fopen(devfile, "r");
    if (!fp){
        PRINT(NULL, "failed open %s\n", devfile);
        return -1;
    }
    while(fgets(buf, 64, fp) != NULL) {
        p = strstr(buf, "boardtype");
        if (p && (!bdinfo->boardtype)) {
            result = strtok(p, "=");
            if (result) {
                result = strtok(NULL, "=");
                bdinfo->boardtype = check_boardtype(result);
            } else {
                SDH_LOGERROR( "boardtype is null, in %s\n", devfile); 
                bdinfo->boardtype = 0;
            }
        }

        p = strstr(buf, "slot");
        if (p && (!bdinfo->slot)) {
            result = strtok(p, "=");
            if (result) {
                result = strtok(NULL, "=");
                bdinfo->slot = atoi(result); 
            } else {
                SDH_LOGERROR( "slot is null, in %s\n", devfile); 
                bdinfo->slot = 0;
            }
        }
    }
    fclose(fp);
    return 0;
}

int sdhlib_get_borad_info(struct board_info *bdinfo)
{
    uint8_t  i = 0;
    int amc_status = 0;
    struct sdh_fpga_info finfo;

    memset(bdinfo, 0, sizeof(struct board_info));
    bdinfo->hw_version = 0;
    bdinfo->sw_version = 0;
 
    if (get_bdinfo_from_cfgfile(bdinfo) != 0) {
        SDH_LOGERROR( "get board info from cfgfile.\n");
        return -1;
    }
    amc_status = get_cpld_info();
    if (amc_status < 0) {
        SDH_LOGERROR( "get amc status from cpld err.\n"); 
    }

    for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {
        if (fpgaid_fd_map((i + 1)) != 0) {
            return -1;
        }

        memset(&finfo, 0, sizeof(&finfo));
        if (ioctl(cur_fd, OIC_SET_SDH_GET_BOARD_INFO, &finfo) != 0) {
            return -1; 
        }

        if (i < 2) {
            if (amc_status & 0x01){
                bdinfo->fpgas[i].present = 1;
            } else {
                bdinfo->fpgas[i].present = 0;
            }
        } else {
            if (amc_status & 0x02){
                bdinfo->fpgas[i].present = 1;
            } else {
                bdinfo->fpgas[i].present = 0;
            }
        }
        bdinfo->fpgas[i].type = finfo.type;
        if (finfo.type == FPGA_TYPE_64KPPP) {
            bdinfo->fpgas[i].subtype = 0;
        } else {
            bdinfo->fpgas[i].subtype = (finfo.subtype + 1);
        }
        bdinfo->fpgas[i].version = finfo.version;
        memcpy(bdinfo->fpgas[i].date, finfo.date, sizeof(finfo.date));
        bdinfo->fpgas[i].status = finfo.status;
        bdinfo->chassisid = finfo.chassisid;
        //SDH_LOGDEBUG("fpgaid[%d] type %d subtype %d status %d", 
        //      i, bdinfo->fpgas[i].type, bdinfo->fpgas[i].subtype, bdinfo->fpgas[i].status);
    }
    return 0;
}

int sdhlib_set_borad_info(struct board_cfg_info *bdinfo)
{
    struct sdh_fpga_info finfo;
	int i, type;

	for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {
		if (fpgaid_fd_map((i + 1)) != 0) {
			return -1;
		}

        //board type is hp or lp , can set board info.
        type = sdh_hd->bdinfo.fpgas[i].type;
        if ((type == FPGA_TYPE_HP) && (type == FPGA_TYPE_LP)) {
            SDH_LOGERROR("fpgaid %d is not hp or lp\n", i);
            continue;
        }

		memset(&finfo, 0, sizeof(struct sdh_fpga_info));
		finfo.fpgaid = (i + 1);
        finfo.physlot = bdinfo->physlot;
        finfo.chassisid = bdinfo->chassisid;

		if (ioctl(cur_fd, OIC_SET_SDH_SET_BOARD_INFO, &finfo) != 0) {
			return -1;
		}
	}

	return 0;
}

int sdhlib_set_ratio(uint8_t sel, uint8_t ratio)
{
    return 0;
}

int sdhlib_set_e1user_map(struct proto_e1user_map *map)
{
    struct sdh_e1user_map smap;
    uint8_t type = 0;
    uint8_t fpgaid = 0;

    fpgaid = map->fpgaid;
    if (fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

    type = sdh_hd->bdinfo.fpgas[fpgaid-1].type;
    if (type != FPGA_TYPE_LP)
        return -1;

    memset(&smap, 0, sizeof(struct sdh_e1user_map));
    smap.au4 = (map->channel - 1);
    smap.e1 = (map->e1 - 1);
    smap.enable = map->enable;
    smap.user_chassisid = map->user_chassisid;
    smap.user_slot = map->user_slot;
    smap.user_port = map->user_port;
    smap.user_e1 = (map->user_e1 - 1);
    if ((map->rule != 1) && (map->rule != 2)) {
        SDH_LOGERROR("Failed set fpga[%d] 2muser  : src %d-%d dst-sel[%d]rule[%d] user %d-%d-%d-%d enable[%d]", \
                 fpgaid, smap.au4, smap.e1, smap.sel, smap.rule, smap.user_chassisid, \
                 smap.user_slot, smap.user_port, smap.user_e1, smap.enable);
        return -1;
    }
    smap.rule = map->rule;
    smap.sel = map->sel;

    sdh_hd->stat.cnt_2muser++;
    SDH_LOGDEBUG("[%llu]2muser : src %d-%d dst-sel[%d]rule[%d] user %d-%d-%d-%d enable[%d]", \
             sdh_hd->stat.cnt_2muser, smap.au4, smap.e1, smap.sel, smap.rule, smap.user_chassisid, \
             smap.user_slot, smap.user_port, smap.user_e1, smap.enable);

    if (ioctl(cur_fd, OIC_SET_SDH_SET_E1USER, &smap) != 0) {
        return -1;
    }

    return 0;
}

int sdhlib_set_e1linkmap_map(struct proto_e1linkmap_map *map)
{
    struct sdh_e1linkmap_map lmp_map;
    uint8_t type = 0;
    uint8_t fpgaid = 0;

    fpgaid = map->fpgaid;
    if (fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

    type = sdh_hd->bdinfo.fpgas[fpgaid-1].type;
    if (type != FPGA_TYPE_LP)
        return -1;
    
    memset(&lmp_map, 0, sizeof(struct sdh_e1linkmap_map));
    lmp_map.lmp_sel  = map->lmp_sel;
    lmp_map.map.au4 = (map->channel - 1);
    lmp_map.map.e1 = (map->e1 - 1);
    lmp_map.map.enable = map->enable;
    lmp_map.map.user_chassisid = map->lmp_chassisid;
    lmp_map.map.user_slot = map->lmp_slot;
    lmp_map.map.user_port = map->lmp_port;
    lmp_map.map.user_e1 = (map->lmp_e1 - 1);

    if ((map->rule != 1) && (map->rule != 2)) {
        SDH_LOGERROR("Unkown e1 rule %d ", map->rule);
        return -1;
    }
    lmp_map.map.rule = map->rule;
    lmp_map.map.sel = map->sel;

    if (ioctl(cur_fd, OIC_SET_SDH_SET_E1LINKMAP, &lmp_map) != 0) {
        SDH_LOGERROR("Failed set fpga[%d] linkmap : src %d-%d dst-sel[%d] rule[%d]lm-sel[%d] \
                 user %d-%d-%d-%d enable[%d]",  fpgaid, lmp_map.map.au4, lmp_map.map.e1, \
                 lmp_map.map.sel, lmp_map.map.rule, lmp_map.lmp_sel, lmp_map.map.user_chassisid, \
                 lmp_map.map.user_slot, lmp_map.map.user_port, lmp_map.map.user_e1, lmp_map.map.enable);

        return -1; 
    }

    sdh_hd->stat.cnt_linkmap++;
    SDH_LOGDEBUG("[%llu]linkmp : src %d-%d dst-sel[%d] rule[%d]lm-sel[%d] user %d-%d-%d-%d enable[%d]", \
             sdh_hd->stat.cnt_linkmap, lmp_map.map.au4, lmp_map.map.e1, lmp_map.map.sel, lmp_map.map.rule, \
             lmp_map.lmp_sel, lmp_map.map.user_chassisid, lmp_map.map.user_slot, \
             lmp_map.map.user_port, lmp_map.map.user_e1, lmp_map.map.enable);

    return 0;
}

int sdhlib_set_2m64kppp_map(struct proto_2m64kppp_map *map)
{
    struct sdh_e164k_map e164k_map;
    uint8_t type = 0;
    uint8_t fpgaid = 0;

    fpgaid = map->fpgaid;
    if (fpgaid_fd_map(fpgaid) != 0) {
        SDH_LOGERROR("fpgaid %d failed", fpgaid);
        return -1;
    }

    type = sdh_hd->bdinfo.fpgas[fpgaid-1].type;
    if (type != FPGA_TYPE_LP) {
        SDH_LOGERROR("fpgaid %d is not lp", fpgaid);
        return -1;
    }

    memset(&e164k_map, 0, sizeof(struct sdh_e164k_map));
    e164k_map.e164k_sel = map->ppp_sel;
    e164k_map.map.au4 = (map->channel - 1);
    e164k_map.map.e1 = (map->e1 - 1);
    e164k_map.map.enable = map->enable;
    e164k_map.map.user_chassisid = map->ppp_chassisid;
    e164k_map.map.user_slot = map->ppp_slot;
    e164k_map.map.user_port = map->ppp_port;
    e164k_map.map.user_e1 = (map->ppp_e1 - 1);

    if ((map->rule != 1) && (map->rule != 2)) {
        SDH_LOGERROR("Unkown e1 rule %d ", map->rule);
        return -1;
    }
    e164k_map.map.rule = map->rule;
    e164k_map.map.sel = map->sel;

    if (ioctl(cur_fd, OIC_SET_SDH_SET_E164K, &e164k_map) != 0) {
        SDH_LOGERROR("Failed set fpga[%d] 64kppp src %d-%d dst-sel[%d]rule[%d] e164k-sel[%d] \
                 user %d-%d-%d-%d enable[%d]", fpgaid, e164k_map.map.au4, e164k_map.map.e1, e164k_map.map.sel, \
                 e164k_map.map.rule, e164k_map.e164k_sel, e164k_map.map.user_chassisid, e164k_map.map.user_slot, \
                 e164k_map.map.user_port, e164k_map.map.user_e1, e164k_map.map.enable);
        return -1;
    }

    sdh_hd->stat.cnt_64kppp++;
    SDH_LOGDEBUG("[%llu]64kppp : src %d-%d dst-sel[%d]rule[%d] e164k-sel[%d] user %d-%d-%d-%d enable[%d]", \
             sdh_hd->stat.cnt_64kppp, e164k_map.map.au4, e164k_map.map.e1, e164k_map.map.sel, \
             e164k_map.map.rule, e164k_map.e164k_sel, e164k_map.map.user_chassisid, e164k_map.map.user_slot, \
             e164k_map.map.user_port, e164k_map.map.user_e1, e164k_map.map.enable);

    return 0;
}

int sdhlib_set_2mgfp_map(struct proto_2mgfp_map *map)
{
    struct sdh_e1gfp_map e1gfp_map;
    uint8_t fpgaid = 0;
    uint8_t type = 0;

    fpgaid = map->fpgaid;
    if (fpgaid_fd_map(fpgaid) != 0) {
        SDH_LOGERROR("fpgaid %d failed", fpgaid);
        return -1;
    }

    type = sdh_hd->bdinfo.fpgas[fpgaid-1].type;
    if (type != FPGA_TYPE_LP) {
        SDH_LOGERROR("fpgaid %d is not lp", fpgaid);
        return -1;
    }

    memset(&e1gfp_map, 0, sizeof(struct sdh_e1gfp_map));
    e1gfp_map.e1gfp_sel = map->gfp_sel;
    e1gfp_map.map.au4 = (map->channel - 1);
    e1gfp_map.map.e1 = (map->e1 - 1);
    e1gfp_map.map.enable = map->enable;
    e1gfp_map.map.user_chassisid = map->gfp_chassisid;
    e1gfp_map.map.user_slot = map->gfp_slot;
    e1gfp_map.map.user_port = map->gfp_port;
    e1gfp_map.map.user_e1 = (map->gfp_e1 - 1);

    if ((map->rule != E1_RULE_LOCAL) && (map->rule != E1_RULE_GLOBAL)) {
        SDH_LOGERROR("Unkown e1 rule %d ", map->rule);
        return -1;
    }

    e1gfp_map.map.rule = map->rule;
    e1gfp_map.map.sel = map->sel;

#if 1
    if (ioctl(cur_fd, OIC_SET_SDH_SET_E1GFP, &e1gfp_map) != 0) {
        SDH_LOGERROR("Failed set fpga[%d] gfp src %d-%d dst-sel[%d]rule[%d] e1gfp-sel[%d] \
                 user %d-%d-%d-%d enable[%d]", fpgaid, e1gfp_map.map.au4, e1gfp_map.map.e1, e1gfp_map.map.sel, \
                 e1gfp_map.map.rule, e1gfp_map.e1gfp_sel, e1gfp_map.map.user_chassisid, e1gfp_map.map.user_slot, \
                 e1gfp_map.map.user_port, e1gfp_map.map.user_e1, e1gfp_map.map.enable);
        return -1;
    }
#endif

    sdh_hd->stat.cnt_gfp++;
    SDH_LOGDEBUG("[%llu]gfp : src %d-%d dst-sel[%d]rule[%d] e1gfp-sel[%d] user %d-%d-%d-%d enable[%d]", \
             sdh_hd->stat.cnt_gfp, e1gfp_map.map.au4, e1gfp_map.map.e1, e1gfp_map.map.sel, \
             e1gfp_map.map.rule, e1gfp_map.e1gfp_sel, e1gfp_map.map.user_chassisid, e1gfp_map.map.user_slot, \
             e1gfp_map.map.user_port, e1gfp_map.map.user_e1, e1gfp_map.map.enable);

    return 0;
}

int sdhlib_get_soh_hpinfo(uint8_t port, struct soh_hpinfo *info)
{
    struct fpga_board_runinfo_port_ex pinfo;
    uint8_t fpgaid = 0;
    uint8_t hport = 0;
    uint8_t i = 0;

    memset(info, 0, sizeof(struct soh_hpinfo));
    if (port_conv_fpgaid(port, &fpgaid) != 0) {
        return -1;
    }

    if (sdh_hd->bdinfo.fpgas[fpgaid-1].type != FPGA_TYPE_HP) {
        return -1; 
    }

    if(fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_START, NULL) != 0) {
        return -1; 
    }

	if ((port % 2) == 0)
		hport = 1;

    for (i = 0; i < SDHMUX_STM1_MAXNUM; i++) {
        memset(&pinfo, 0, sizeof(struct fpga_board_runinfo_port_ex));
        pinfo.fiber = ((hport & 0x03) << 6) | i;
        if (ioctl(cur_fd, OIC_GET_FPGA_SLINK_INFO_HP, &pinfo) != 0) {
            goto err;
        }

        if (i == 0) {
            info->port = port;
            info->speed = pinfo.phy_type;
            if (info->speed == PAYLOAD_HPSPEED_10000M) {
                info->stm1s_num = SDHMUX_STM1_MAXNUM;
            } else if (info->speed == PAYLOAD_HPSPEED_2500M) {
                info->stm1s_num = 16;
            } else if (info->speed == PAYLOAD_HPSPEED_622M) {
                info->stm1s_num = 4;
            } else if (info->speed == PAYLOAD_HPSPEED_155M) {
                info->stm1s_num = 1;
            } else {
                info->stm1s_num = 0;
                SDH_LOGERROR( "Unkown port speed %d\n", info->speed);
                goto err;
            }
        }
        //SDH_LOGDEBUG("[%d] los %d b1 %d b2 %d b3 %d c2 %d j1_0 %d j1_1 %d j1_2 %d j1_3 %d\n", 
        //        i, pinfo.los, pinfo.b1_cnt, pinfo.b2_cnt, pinfo.b3_cnt, pinfo.c2_val,
        //      pinfo.j1_0, pinfo.j1_1, pinfo.j1_2, pinfo.j1_3);

        info->stm1s[i].b1 = pinfo.b1_cnt;
        info->stm1s[i].b2 = pinfo.b2_cnt;
        info->stm1s[i].b3 = pinfo.b3_cnt;
        info->stm1s[i].c2 = pinfo.c2_val;
        info->stm1s[i].j1_0 = pinfo.j1_0;
        info->stm1s[i].j1_1 = pinfo.j1_1;
        info->stm1s[i].j1_2 = pinfo.j1_2;
        info->stm1s[i].j1_3 = pinfo.j1_3;
    }

	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        return -1; 
    }
    return 0;

err: 
	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        return -1; 
    }
    return -1;
}

int sdhlib_get_soh_lpinfo(uint8_t fpgaid, uint8_t port, struct soh_lpinfo *info)
{
    struct slink_info link;
    struct fpga_board_runinfo_ex rinfo;
    uint8_t i = 0;

    if (sdh_hd->bdinfo.fpgas[fpgaid-1].type != FPGA_TYPE_LP) {
        return -1; 
    }

    if(fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

    memset(&link, 0, sizeof(&link));
    memset(&rinfo, 0, sizeof(&rinfo));
    memset(info, 0, sizeof(struct soh_lpinfo));

    rinfo.start_port = (port - 1);
	if (ioctl(cur_fd, OIC_GET_FPGA_BD_RUNINFO_EX, &rinfo) != 0 ) {
        return -1;
    }

    info->id = fpgaid;
    info->c2 = rinfo.ports[0].c2_val;
    info->channels_num = SDHMUX_CHANNEL_MAXNUM;

	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_START, NULL) != 0) {
        return -1; 
    }
    for (i = 0; i < SDHMUX_CHANNEL_MAXNUM; i++) {
        link.fiber = (port - 1);
        link.channel = i;
        if (ioctl(cur_fd, OIC_GET_FPGA_SLINK_INFO, &link) != 0) {
            goto err;
        }
        info->channels[i].v5 = link.v5_val;
        if (link.e1_sync) {
            info->channels[i].e1valid = 1;
            info->channels[i].e1type = 1;
        } else if (link.nfm_e1_sync) {
            info->channels[i].e1valid = 1;
            info->channels[i].e1type = 2;
        } else {
            info->channels[i].e1valid = 0;

            if (link.svc_type == 1)
                info->channels[i].e1type = 3;
        }
    }

	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        return -1;
    }
    return 0;

err:
	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        return -1;
    }
    return -1;
}

int get_payload_hp(struct payload_hpinfo *info)
{
    uint8_t i = 0;
    struct fpga_board_runinfo_port_ex pinfo;
    uint8_t hport = 0;

    if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_START, NULL) != 0) {
        return -1;
    }
    if ((info->port % 2) == 0)
        hport = 1;

    for (i = 0; i < SDHMUX_STM1_MAXNUM; i++) {
        memset(&pinfo, 0, sizeof(struct fpga_board_runinfo_port_ex));
        pinfo.fiber = ((hport & 0x03) << 6) | i;
        if (ioctl(cur_fd, OIC_GET_FPGA_SLINK_INFO_HP, &pinfo) != 0) {
            goto err;
        }

        /* if have los,lof, info is 0 */
        if (i == 0) {
            if ((pinfo.los) || (pinfo.stm1_synced)) {
                info->stm1s_num = 0;
                break;
            }

            info->speed = pinfo.phy_type;
            switch (info->speed) {
                case PAYLOAD_HPSPEED_155M:
                    info->stm1s_num = 1;
                    break;

                case PAYLOAD_HPSPEED_622M:
                    info->stm1s_num = 4;
                    break;

                case PAYLOAD_HPSPEED_2500M:
                    info->stm1s_num = 16;
                    break;

                case PAYLOAD_HPSPEED_10000M:
                    info->stm1s_num = SDHMUX_STM1_MAXNUM;
                    break;

                default:
                    return -1;
            }
        }

        info->stm1s[i].id = (i + 1);

        if (pinfo.c2_val == 2) {
            info->stm1s[i].type = PROTOCOL_TUG;
		} else if (pinfo.c2_val == 0) {
            info->stm1s[i].type = PROTOCOL_UEQ;
        } else {
            info->stm1s[i].type = pinfo.svc_type;
        }
    }

    if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        return -1;
    }
    return 0;
err:
	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        return -1;
    }
    return -1;
}

int get_payload_lp(struct payload_lpinfo *info)
{
    struct fpga_board_runinfo_port_ex pinfo;
    struct slink_info *slink = NULL;
    struct payload_tug3 *tug3 = NULL;
    struct payload_tug2 *tug2 = NULL;
    struct payload_c1x *c1x = NULL;
    uint8_t tug2_num = 0;
    uint8_t c1x_num = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t k = 0;

    slink = (struct slink_info *)malloc(sizeof(struct slink_info) * SDHMUX_CHANNEL_MAXNUM);
    if (!slink) {
        return -1; 
    }
    memset(slink, 0, (sizeof(struct slink_info) * SDHMUX_CHANNEL_MAXNUM));
    memset(&pinfo, 0, sizeof(struct fpga_board_runinfo_port_ex));

    pinfo.fiber = (info->id - 1);
    if (ioctl(cur_fd, OIC_GET_FPGA_SLINK_INFO_HP, &pinfo) != 0) {
        goto err;
    }

    info->tug3s_num = 0;
    if (pinfo.c2_val == 0x00) {
        info->type = PROTOCOL_UEQ; 
        goto ok_exit;
    } else if (pinfo.c2_val == 0x02) {
        info->type = PROTOCOL_TUG; 
        info->tug3s_num = SDHMUX_TUG3_MAXNUM;
    } else {
        PRINT(NULL,"payload lp info port[%d] unkown c2 type %d\n", info->id, pinfo.c2_val);
        goto ok_exit;
    }

    if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_START, NULL) != 0) {
        goto err;
    }
    for (i = 0; i < SDHMUX_CHANNEL_MAXNUM; i++) {
        slink[i].fiber = (info->id - 1);
        slink[i].channel = i;
        if (ioctl(cur_fd, OIC_GET_FPGA_SLINK_INFO, &slink[i]) != 0) {
            goto err;
        }
    }

    for (i = 0; i < info->tug3s_num; i++) {
        tug3 = &info->tug3s[i];
        tug3->id = (i + 1);
        tug3->type = PROTOCOL_TUG;
        tug3->tug2s_num = SDHMUX_TUG2_MAXNUM;

        for (j = 0; j < tug3->tug2s_num; j++) {
            tug2 = &info->tug3s[i].tug2s[j];
            tug2_num = (i * SDHMUX_TUG3_MAXNUM) + j;
            tug2->id = (j + 1);
            tug2->type = slink[tug2_num].channel_rate;
            tug2->c1xs_num = SDHMUX_C1x_MAXNUM;

            for (k = 0; k < tug2->c1xs_num; k++) {
                c1x = &info->tug3s[i].tug2s[j].c1xs[k];
                c1x_num = i + (j * 3)  + (k * 21);
                c1x->id = (k + 1);
                c1x->type = slink[c1x_num].svc_type;
                c1x->tss_num = 0;

            }
        }

    }

	if (ioctl(cur_fd, OIC_SET_FPGA_SLINK_INFO_END, NULL) != 0) {
        goto err;
    }

ok_exit:
    free(slink);
    return 0;
err:
    free(slink);
    return -1;
}

int sdhlib_get_payload_info(uint8_t port, uint8_t fpgaid, struct payload_info *pinfo)
{
    pinfo->type = sdh_hd->bdinfo.fpgas[fpgaid-1].type;

    if (fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

    if (pinfo->type == FPGA_TYPE_HP) {
        //SDH_LOGDEBUG("sdhlib_get_payload_info : hp .");
        pinfo->info.hp.port = port;
        if (get_payload_hp(&pinfo->info.hp) != 0) {
            return -1;
        }
    } else if (pinfo->type == FPGA_TYPE_LP) {
        //SDH_LOGDEBUG("sdhlib_get_payload_info : lp .");
        pinfo->info.lp.id = port;
        if (get_payload_lp(&pinfo->info.lp) != 0) {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

int sdhlib_get_gfp_info(uint8_t port,uint8_t fpgaid, struct gfp_info *info)
{
    char section[] = {"Sgfp Group Info"};
    char id[] = {"link"};
    unsigned long cfghd = 0ul;
    uint8_t port_group = 0;
    uint8_t secnum = 0;
    uint8_t idnum = 0;
    uint8_t groupid = 0;
    uint8_t channels_num = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    char result_name[64];
    char idval[64];
    int tmpval[4];

    memset(info, 0, sizeof(struct gfp_info));
    if (fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

    info->type = sdh_hd->bdinfo.fpgas[fpgaid-1].type;
    if (info->type == FPGA_TYPE_HP) {
        if (port > 4) {
            SDH_LOGERROR( "HP port %d err\n", port);
            return -1;
        }
    } else if (info->type == FPGA_TYPE_LP) {
        if (port > 16) {
            SDH_LOGERROR( "LP port %d err\n", port);
            return -1;
        }
    } else {
        return -1;
    }

    port_group = ((port - 1) >> 2) & 0x03;
    memset(&result_name, 0, sizeof(result_name));
    sprintf(result_name, "/tmp/fpga%d/sgfp%d.cfg", (fpgaid - 1), port_group);

    if (access(result_name, 0) != 0) {
        //SDH_LOGINFO("file %s is not exisits.", result_name);
        info->groups_num = 0;
        return 0;
    }

    cfghd = CfgInitialize(result_name);
    if (cfghd == 0ul) {
        printf("Failed to CfgInitialize %s.\n", result_name);
        return -1;
    }

    secnum = CfgGetCount(cfghd, section, NULL, 0);
    groupid = 0;

    for (i = 0; i < secnum; i++) {
        idnum = CfgGetCount(cfghd, section, id, (i+1));
        info->groups[i].speed = 0;
        memset(info->groups[i].channels, 0, sizeof(info->groups[i].channels));
        for (j = 0; j < idnum; j++) {
            if (CfgGetValue(cfghd, section, id, idval, (j+1), (i + 1)) < 0) {
                break;
            }
            memset(tmpval, 0, sizeof(tmpval));
            sscanf(idval, "%d,%d,%d,%d", &tmpval[0], &tmpval[1], &tmpval[2], &tmpval[3]);
            if ((j == 0) && (tmpval[0] != (port - 1))) {
                break;
            }
            if (j == 0) {
                groupid++;
                channels_num = 0;
            }

            info->groups[groupid - 1].speed = tmpval[3];
            info->groups[groupid - 1].channels[channels_num] = (tmpval[2] + 1);
            channels_num++;
        }
        info->groups[groupid - 1].channels_num = channels_num;
    }
	CfgInvalidate(cfghd);
    info->groups_num = groupid;
    return 0;
}

int sdhlib_get_E1_info(uint8_t fpgaid, struct e1_info *info)
{
    if (fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }
    if (ioctl(cur_fd, OIC_SET_SDH_GET_E1_INFO, info) != 0) {
        SDH_LOGERROR( "sdhlib_get_E1-info failed.\n");
        return -1;
    }

    return 0;
}

int sdhlib_set_sel(uint8_t fpgaid, uint8_t sel)
{
    if (fpgaid_fd_map(fpgaid) != 0) {
        return -1;
    }

	if (ioctl(cur_fd, OIC_SET_GFP_FPGA_SPU_SELNUM, &sel) != 0) {
        SDH_LOGERROR( "sdhlib_set_sel failed.\n");
        return -1;
    }

    return 0;
}

/*
 * port: '1-4' '13-16' sdh ports,get status from fpga. '5-12' '17-24' ether ports.get status from fpga.
* '25-40'; fpga(1-16)-switch ports. '41-42': fab(1-2)-switch ports. '43-50': rear(1-8)-switch ports.
* */
typedef struct sdh_switch_port_map {
    uint8_t swportname[8];
    uint8_t sw_portnum;
    char usrportname[16];
    uint8_t usrportnum;
    uint8_t usrporttype;
} sdh_switch_port_map_t;

sdh_switch_port_map_t sdh_sw_map_tbl[] = {
    {"xe0",  1, "fpga2-0", 33, PORT_TYPE_LAN},
    {"xe1",  2, "fpga2-1", 34, PORT_TYPE_LAN},
    {"xe2",  3, "fpga2-2", 35, PORT_TYPE_LAN},
    {"xe3",  4, "fpga2-3", 36, PORT_TYPE_LAN},
    {"xe4",  5, "fpga3-0", 37, PORT_TYPE_LAN},
    {"xe5",  6, "fpga3-1", 38, PORT_TYPE_LAN},
    {"xe6",  7, "fpga3-2", 39, PORT_TYPE_LAN},
    {"xe7",  8, "fpga3-3", 40, PORT_TYPE_LAN},
    {"xe8",  9,  "fab0",   41, PORT_TYPE_LAN},
    {"xe9",  10, "fab1",   42, PORT_TYPE_LAN},
    {"xe10", 11, "rear-1", 43, PORT_TYPE_LAN},
    {"xe11", 12, "rear-2", 44, PORT_TYPE_LAN},
    {"xe12", 13, "rear-3", 45, PORT_TYPE_LAN},
    {"xe13", 14, "rear-4", 46, PORT_TYPE_LAN},
    {"xe14", 15, "rear-5", 47, PORT_TYPE_LAN},
    {"xe15", 16, "rear-6", 48, PORT_TYPE_LAN},
    {"xe16", 17, "rear-7", 49, PORT_TYPE_LAN},
    {"xe17", 18, "rear-8", 50, PORT_TYPE_LAN},
    {"xe18", 19, "fpga0-0", 25, PORT_TYPE_LAN},
    {"xe19", 20, "fpga0-1", 26, PORT_TYPE_LAN},
    {"xe20", 21, "fpga0-2", 27, PORT_TYPE_LAN},
    {"xe21", 22, "fpga0-3", 28, PORT_TYPE_LAN},
    {"xe22", 23, "fpga1-0", 29, PORT_TYPE_LAN},
    {"xe23", 24, "fpga1-1", 30, PORT_TYPE_LAN},
    {"xe24", 25, "fpga1-2", 31, PORT_TYPE_LAN},
    {"xe25", 26, "fpga1-3", 32, PORT_TYPE_LAN},
};

typedef struct pca9548_channel2sfp_map {
    uint8_t pca9548addr;
    uint8_t channelid;
    char i2cdevname[16];
    char usrportname[16];
    uint8_t usrportnum;
    uint8_t usrporttype;
} pca9548_channel2sfp_map_t;

pca9548_channel2sfp_map_t sfp_map_tbl[] = {
    {0x70, 1, "/dev/i2c-2", "sdh-1", 1, PORT_TYPE_SDH},
    {0x70, 3, "/dev/i2c-3", "sdh-2", 2, PORT_TYPE_SDH},
    {0x70, 5, "/dev/i2c-4", "sdh-3", 3, PORT_TYPE_SDH},
    {0x70, 7, "/dev/i2c-5", "sdh-4", 4, PORT_TYPE_SDH},
    /* 0x70 channelid 5-8 reserved */
    {0x71, 1, "/dev/i2c-10", "front-eth-1", 5, PORT_TYPE_LAN},
    {0x71, 2, "/dev/i2c-11", "front-eth-2", 6, PORT_TYPE_LAN},
    {0x71, 3, "/dev/i2c-12", "front-eth-3", 7, PORT_TYPE_LAN},
    {0x71, 4, "/dev/i2c-13", "front-eth-4", 8, PORT_TYPE_LAN},
    {0x71, 5, "/dev/i2c-14", "front-eth-5", 9, PORT_TYPE_LAN},
    {0x71, 6, "/dev/i2c-15", "front-eth-6", 10, PORT_TYPE_LAN},
    {0x71, 7, "/dev/i2c-16", "front-eth-7", 11, PORT_TYPE_LAN},
    {0x71, 8, "/dev/i2c-17", "front-eth-8", 12, PORT_TYPE_LAN},
    {0x72, 1, "/dev/i2c-18", "sdh-5", 13, PORT_TYPE_SDH},
    {0x72, 3, "/dev/i2c-19", "sdh-6", 14, PORT_TYPE_SDH},
    {0x72, 5, "/dev/i2c-20", "sdh-7", 15, PORT_TYPE_SDH},
    {0x72, 7, "/dev/i2c-21", "sdh-8", 16, PORT_TYPE_SDH},
    /* 0x72 channelid 5-8 reserved */
    {0x73, 1, "/dev/i2c-26", "front-eth-9",  17, PORT_TYPE_LAN},
    {0x73, 2, "/dev/i2c-27", "front-eth-10", 18, PORT_TYPE_LAN},
    {0x73, 3, "/dev/i2c-28", "front-eth-11", 19, PORT_TYPE_LAN},
    {0x73, 4, "/dev/i2c-29", "front-eth-12", 20, PORT_TYPE_LAN},
    {0x73, 5, "/dev/i2c-30", "front-eth-13", 21, PORT_TYPE_LAN},
    {0x73, 6, "/dev/i2c-31", "front-eth-14", 22, PORT_TYPE_LAN},
    {0x73, 7, "/dev/i2c-32", "front-eth-15", 23, PORT_TYPE_LAN},
    {0x73, 8, "/dev/i2c-33", "front-eth-16", 24, PORT_TYPE_LAN},
};

#define SDH_PORT_MAXNUM  50
#define I2C_READ_DATA    1
#define I2C_WRITE_DATA   2

static int i2c_rw_data(uint8_t rw, uint8_t i2cfd, uint8_t i2caddr, uint8_t offset, uint8_t *val)
{
    struct i2c_rdwr_ioctl_data *data = NULL;

    if ((data = (struct i2c_rdwr_ioctl_data *)malloc(sizeof(struct i2c_rdwr_ioctl_data))) == NULL) {
        goto exit1;
    }
    data->nmsgs = 2;
    if ((data->msgs = (struct i2c_msg *)malloc(sizeof(struct i2c_msg) * data->nmsgs)) == NULL) {
        goto exit2;
    }
    if ((data->msgs[0].buf = (unsigned char *)malloc(sizeof(unsigned char))) == NULL) {
        goto exit3;
    }
    if ((data->msgs[1].buf = (unsigned char *)malloc(sizeof(unsigned char))) == NULL) {
        goto exit4;
    }

    data->msgs[0].addr = i2caddr;
    data->msgs[0].flags = 0;
    data->msgs[0].len  = 1;
    data->msgs[0].buf[0] = offset;

    if (rw == I2C_READ_DATA) {
        data->msgs[1].addr = i2caddr;
        data->msgs[1].flags = I2C_M_RD;
        data->msgs[1].len  = 1;
        data->msgs[1].buf[0] = 0;
    } else {
        data->msgs[1].addr = i2caddr;
        data->msgs[1].flags &= ~I2C_M_RD;
        data->msgs[1].len  = 1;
        data->msgs[1].buf[0] = *val;
    }

    if (ioctl(i2cfd, I2C_RDWR, data) < 0) {
        //SDH_LOGDEBUG("i2c %s data failed. addr 0x%x\n", (rw == I2C_READ_DATA) ? "read":"write", i2caddr);
        goto exit5;
	}

    if (rw == I2C_READ_DATA) {
        *val = data->msgs[1].buf[0];
        //SDH_LOGDEBUG("i2c read data complete. addr 0x%x val 0x%x\n", i2caddr, *val);
    }

    free(data->msgs[1].buf);
    free(data->msgs[0].buf);
    free(data->msgs);
    free(data);
    return 0;

exit5:
        free(data->msgs[1].buf);
exit4:
        free(data->msgs[0].buf);
exit3:
        free(data->msgs);
exit2:
        free(data);
exit1:
    return -1;
}

static int get_port_status_from_pca9548(uint8_t usrportnum, struct port_status *status)
{
    struct fpga_board_runinfo_port_ex pinfo;
    uint8_t channels_num = 24; 
    uint8_t pca9548addr = 0;
    uint8_t channelid = 0;
    uint8_t tmpval = 0;
    int i2cfd = 0;
    uint8_t id = 0;
    uint8_t fpgaid = 0;


    while (id < channels_num) {
        if (sfp_map_tbl[id].usrportnum == usrportnum) {
            pca9548addr = sfp_map_tbl[id].pca9548addr;
            channelid = sfp_map_tbl[id].channelid - 1;
            status->type = sfp_map_tbl[id].usrporttype;
            break;
        }
        id++;
    }

    if (id >= channels_num) {
        SDH_LOGERROR("get port status from pca9548. usrportnum %d is not found.", usrportnum);
        return -1;
    }

    i2cfd = open(sfp_map_tbl[id].i2cdevname, O_RDWR);
    if (i2cfd < 0) {
        SDH_LOGERROR("failed to open %s", sfp_map_tbl[id].i2cdevname);
        return -1;
    }

    /* pca9548 addr enable */
    tmpval = (1 << channelid);
    i2c_rw_data(I2C_WRITE_DATA, i2cfd, pca9548addr, 0, &tmpval);

    /* read sfp(0x50) reg info */
    tmpval = 0;
    i2c_rw_data(I2C_READ_DATA, i2cfd, 0x51, 110, &tmpval);

    status->link_status = (tmpval & 0x02) ? 1 : 0;
    status->work_enable = PORT_WORK_ENABLE;
    status->rate = PORT_RATE_10G;
    status->fiber_type = PORT_FIBERTYPE_SINGLE;

    i2c_rw_data(I2C_READ_DATA, i2cfd, 0x51, 104, &tmpval);
    status->rx_power = (tmpval & 0x0f) << 8;
    i2c_rw_data(I2C_READ_DATA, i2cfd, 0x51, 105, &tmpval);
    status->rx_power |= (tmpval & 0x0f) << 0;

    i2c_rw_data(I2C_READ_DATA, i2cfd, 0x51, 102, &tmpval);
    status->tx_power = (tmpval & 0x0f) << 8;

    i2c_rw_data(I2C_READ_DATA, i2cfd, 0x51, 103, &tmpval);
    status->tx_power |= (tmpval & 0x0f) << 0;

    /* pca9548 addr disable */
    tmpval = 0;
    i2c_rw_data(I2C_WRITE_DATA, i2cfd, pca9548addr, 0, &tmpval);
    close(i2cfd);

    status->b1_code = 0;
    status->b2_code = 0;
    status->b3_code = 0;
    status->mtu = 4000;

    memset(&pinfo, 0, sizeof(&pinfo));
	switch (usrportnum) {
		case 1:
		case 3:
		case 13:
		case 15:
			pinfo.fiber = 0;
			break;

		case 2:
		case 4:
		case 14:
		case 16:
			pinfo.fiber = (1 << 6);
			break;

		default:
			return 0;
	}
    /* sdh port get b1_code/b2_code/b3_code from fpga */
    port_conv_fpgaid(usrportnum, &fpgaid);
    fpgaid_fd_map(fpgaid);

    if (ioctl(cur_fd, OIC_GET_SDH_GET_AU4_STATUS, &pinfo) != 0) {
        SDH_LOGERROR("ioctl get sdh au4 status failed.");
        return -1;
    }

    status->link_status = (pinfo.los & 0x01) | ((pinfo.stm1_synced & 0x01) << 1);
    status->b1_code = pinfo.b1_cnt;
    status->b2_code = pinfo.b2_cnt;
    status->b3_code = pinfo.b3_cnt;

	switch (pinfo.phy_type) {
		case PAYLOAD_HPSPEED_10000M:
			status->rate = PORT_RATE_10G;
			break;

		case PAYLOAD_HPSPEED_2500M:
			status->rate = PORT_RATE_2500M;
			break;

		case PAYLOAD_HPSPEED_622M:
			status->rate = PORT_RATE_622M;
			break;

		case PAYLOAD_HPSPEED_155M:
			status->rate = PORT_RATE_155M;
			break;

		default:
			return -1;
	}

    return 0;
}

static int get_port_status_from_switch(uint8_t usrportnum, struct port_status *status)
{
	bcm_get_ports_t gptr;
    int port_max_num = 26;
    int id = 0;

    memset(&gptr, 0, sizeof(gptr));
    while (id < port_max_num) {
        if (sdh_sw_map_tbl[id].usrportnum == usrportnum) {
            status->type = sdh_sw_map_tbl[id].usrporttype;
            gptr.port = sdh_sw_map_tbl[id].sw_portnum;
            break;
        }
        id++;
    }

    if ((id >= port_max_num) || (gptr.port > 26)) {
        SDH_LOGERROR("get port status from switch usrportnum %d is not found.", usrportnum);
        return -1;
    }

	if (ioctl(sdh_hd->swfd, BCM_SELF_GET_PORT_STATUS, &gptr) != 0) {
		SDH_LOGERROR("sw get a port status err.\n");
		return -1;
	}

    status->link_status = gptr.linkstatus;
    status->work_enable = gptr.linkswitch;
    status->mtu = gptr.frame_max;
    
    switch (gptr.speed) {
        case 40000:
            status->rate = PORT_RATE_40G;
            break;

        case 10000:
            status->rate = PORT_RATE_10G;
            break;

        case 1000:
            status->rate = PORT_RATE_1000M;
            break;

        default:
            return -1;
    }

    status->b1_code = 0;
    status->b2_code = 0;
    status->b3_code = 0;
    status->rx_power = 0;
    status->tx_power = 0;

    if ((gptr.linkscan == BCM_LINKSCAN_MODE_NONE) && (gptr.loopback == BCM_PORT_LOOPBACK_PHY) && \
        (gptr.discard == BCM_PORT_DISCARD_ALL)) {

        status->fiber_type = PORT_FIBERTYPE_SINGLE;
    } else if ((gptr.linkscan == BCM_LINKSCAN_MODE_SW) && (gptr.loopback == BCM_PORT_LOOPBACK_NONE) && \
        (gptr.discard == BCM_PORT_DISCARD_NONE)) {

        status->fiber_type = PORT_FIBERTYPE_DOUBLE;
    } else {

        status->fiber_type = 0;
    }

    return 0;
}

int sdhlib_get_port_status(uint8_t usrportnum, struct port_status *status)
{
    if ((usrportnum < 1) || (usrportnum > SDH_PORT_MAXNUM))
        return -1;

    status->id = usrportnum;

    memset(status, 0, sizeof(struct port_status));

    if (usrportnum <= 24)
        get_port_status_from_pca9548(usrportnum, status);
    else
        get_port_status_from_switch(usrportnum, status);
	
    return 0;
}

static int get_port_stat_from_switch(uint8_t usrportnum, struct port_stat *pstat)
{
	bcm_get_ports_statistics gptr;
    int port_max_num = 26; 
    int id = 0;

	memset(&gptr, 0, sizeof(gptr));
    while (id < port_max_num) {
        if (sdh_sw_map_tbl[id].usrportnum == usrportnum) {
            gptr.port = sdh_sw_map_tbl[id].sw_portnum;
            break;
        }
        id++;
    }

    if ((id >= port_max_num) || (gptr.port > 26)) {
        SDH_LOGERROR("get port stat from switch port %d is not found.", usrportnum);
        return -1;
    }

    if (ioctl(sdh_hd->swfd, BCM_SELF_GET_PORT_STATISTICS, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port port stat failed.");
		return -1;
	}

	pstat->rx_pkts = gptr.bcmIfInUcastPkts +
		     gptr.bcmIfInBroadcastPkts +
		     gptr.bcmIfInMulticastPkts;
    pstat->rx_pps = gptr.bcmIfInPps;
    pstat->rx_bps = gptr.bcmIfInBps;
	pstat->rx_drops = gptr.bcmIfInDiscards;
	pstat->rx_errors = gptr.bcmIfInErrors;

    pstat->tx_pkts = gptr.bcmIfOutUcastPkts +
		     gptr.bcmIfOutBroadcastPkts +
		     gptr.bcmIfOutMulticastPkts;
    pstat->tx_pps = gptr.bcmIfOutPps;
    pstat->tx_bps = gptr.bcmIfOutBps;
	pstat->tx_drops = gptr.bcmIfOutDiscards;
	pstat->tx_errors = gptr.bcmIfOutErrors;

    return 0;
}

/* get port statistics :
* port : 5-12 fpga0-fpga1, 17-24 fpga2-fpga3 ; 25-50 switch port 
* */
int sdhlib_get_port_stat(uint8_t port, struct port_stat *pstat)
{
    uint8_t fpgaid = 0;
    struct sdh_fpga_stat fstat;

    memset(pstat, 0, sizeof(struct port_stat));

    pstat->id = port;
    switch (port) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 13:
        case 14:
        case 15:
        case 16:
            return 0;
    }

    if (port > 24) {
        get_port_stat_from_switch(port, pstat);
        return 0;
    }

    port_conv_fpgaid(port, &fpgaid);
    fpgaid_fd_map(fpgaid);

    memset(&fstat, 0, sizeof(struct sdh_fpga_stat));
    if (ioctl(cur_fd, OIC_SET_SDH_GET_BOARD_STAT, &fstat) != 0) {
		SDH_LOGERROR("ioctl set");
        return -1;
    }

    switch (port) {
        case 5:
        case 9:
            pstat->tx_pkts = fstat.local_eth0;
            break;

        case 6:
        case 10:
            pstat->tx_pkts = fstat.local_eth1;
        break;

        case 7:
        case 11:
            pstat->tx_pkts = fstat.local_eth2;
        break;

        case 8:
        case 12:
            pstat->tx_pkts = fstat.local_eth3;
        break;
    }

    return 0;
}

int sdhlib_set_port_fiber(uint8_t port, uint8_t fiber_type)
{
	bcm_set_ports_t gptr;
    uint8_t port_max_num = 26;
    int id = 0;

    if (port < 43) {
        SDH_LOGINFO("port %d is < 43, return.", port);
        return 0;
    }

    memset(&gptr, 0, sizeof(gptr));
    while (id < port_max_num) {
        if (sdh_sw_map_tbl[id].usrportnum == port) {
            gptr.port = sdh_sw_map_tbl[id].sw_portnum;
            break;
        }
        id++;
    }

    if ((id >= port_max_num) || (gptr.port > 26)) {
        SDH_LOGERROR("set fiber type port %d not find.", port);
        return -1;
    }

    if (fiber_type == PORT_FIBERTYPE_SINGLE) {
        /* set port linkscan=none, loopback=phy, discard=all */
        gptr.linkscan = BCM_LINKSCAN_MODE_NONE;
        gptr.loopback = BCM_PORT_LOOPBACK_PHY;
        gptr.discard = BCM_PORT_DISCARD_ALL;
    } else if (fiber_type == PORT_FIBERTYPE_DOUBLE) {
        /* set port linkscan=SW, loopback=none, discard=none*/
        gptr.linkscan = BCM_LINKSCAN_MODE_SW;
        gptr.loopback = BCM_PORT_LOOPBACK_NONE;
        gptr.discard = BCM_PORT_DISCARD_NONE;
    } else {
        SDH_LOGERROR("Unkown fiber type %d.", fiber_type);
        return -1;
    }

    if (ioctl(sdh_hd->swfd, BCM_SELF_SET_PORT_LINKSCAN, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port port linkscan failed.");
        return -1;
    }

    if (ioctl(sdh_hd->swfd, BCM_SELF_SET_PORT_LOOPBACK, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port port loopback failed.");
        return -1;
    }

    if (ioctl(sdh_hd->swfd, BCM_SELF_SET_PORT_DISCARD, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port port discard failed.");
        return -1;
    }

    return 0;
}

int sdhlib_set_port_mtu(uint8_t port, uint32_t mtu)
{
    bcm_set_ports_t gptr;
    uint8_t port_max_num = 26;
    int id = 0;

    if (port <= 24) {
        SDH_LOGINFO("set mut port %d <= 24", port);
        return 0;
    }

    memset(&gptr, 0, sizeof(gptr));
    while (id < port_max_num) {
        if (sdh_sw_map_tbl[id].usrportnum == port) {
            gptr.port = sdh_sw_map_tbl[id].sw_portnum;
            break;
        }
        id++;
    }

    if ((id >= port_max_num) || (gptr.port > 26)) {
        SDH_LOGERROR("set mtu port %d not find.", port);
        return -1;
    }

    gptr.frame_max = mtu;
    if (ioctl(sdh_hd->swfd, BCM_SELF_SET_PORT_FRAME_MAX, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port port frame max failed.");
        return -1;
    }

    return 0;
}

int sdhlib_set_port_enable(uint8_t port, uint8_t enable)
{
    bcm_set_ports_t gptr;
    uint8_t port_max_num = 26;
    int id = 0;

    if (port <= 24) {
        SDH_LOGINFO("set enable port %d <= 24", port);
        return 0;
    }

    memset(&gptr, 0, sizeof(gptr));
    while (id < port_max_num) {
        if (sdh_sw_map_tbl[id].usrportnum == port) {
            gptr.port = sdh_sw_map_tbl[id].sw_portnum;
            break;
        }
        id++;
    }

    if ((id >= port_max_num) || (gptr.port > 26)) {
        SDH_LOGERROR("set enable port %d not find.", port);
        return -1;
    }

    gptr.linkswitch = enable;
    if (ioctl(sdh_hd->swfd, BCM_SELF_SET_PORT_SWITCH, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port switch work enable|disable failed.");
        return -1;
    }

    return 0;
}

int sdhlib_set_port_clear(uint8_t port)
{
    bcm_set_ports_t gptr;
    uint8_t port_max_num = 26;
    int id = 0;

    if (port <= 24) {
        SDH_LOGINFO("set enable port %d <= 24", port);
        return 0;
    }

    memset(&gptr, 0, sizeof(gptr));
    while (id < port_max_num) {
        if (sdh_sw_map_tbl[id].usrportnum == port) {
            gptr.port = sdh_sw_map_tbl[id].sw_portnum;
            break;
        }
        id++;
    }

    if ((id >= port_max_num) || (gptr.port > 26)) {
        SDH_LOGERROR("set clear port %d not find.", port);
        return -1;
    }

    if (ioctl(sdh_hd->swfd, BCM_SELF_SET_PORT_STAT_CLEAR, &gptr) != 0) {
        SDH_LOGERROR("ioctl get port stat clear failed.");
        return -1;
    }

    return 0;
}

int sdhlib_get_gfp_result(uint8_t fpgaid, struct gfp_result *result)
{
    //struct gfp_result *result = NULL;
    char result_file[128];
    unsigned long cfghd = 0ul;
    int secnum = 0;
    int linknum = 0;
    int i = 0;
    int j = 0;
    char value[128];
    int tmpval[8];

    if (fpgaid < 1 || fpgaid > 4) {
        SDH_LOGERROR("line[%d] %s : fpgaid %d err.", __LINE__, __func__, fpgaid);
        return -1;
    }
    memset(result_file, 0, sizeof(result_file));
    memset(result, 0, sizeof(struct gfp_result));

    sprintf(result_file, "/tmp/fpga%d/gfp.result", (fpgaid-1));

    if (access(result_file, 0)) {
        SDH_LOGERROR("%s is not exisits.", result_file);
        return 0;
    }

    cfghd = CfgInitialize(result_file);
    if (cfghd == 0ul) {
        SDH_LOGERROR("line[%d] file[%s] Initialize is failed .", __LINE__, result_file);
        goto err_exit;
    }

    secnum = CfgGetCount(cfghd, "Group", NULL, 0);
    result->groups_num = secnum;

    for (i = 0; i < secnum; i++) {
        linknum = CfgGetCount(cfghd, "Group", "slink", (i+1));
        result->groups[i].channels_num = linknum;

        for (j = 0; j < linknum; j++) {
            memset(value, 0, sizeof(value));
            if (CfgGetValue(cfghd, "Group", "slink", value, (j+1), (i+1)) < 0) {
                SDH_LOGERROR("line[%d] CfgGetValue section[%d]id[%d] is failed.", __LINE__, i, j);
                continue;
            }
            memset(tmpval, 0, sizeof(tmpval));
            sscanf(value, "%d,%d,%d,%d,%d", &tmpval[0], &tmpval[1], &tmpval[2], &tmpval[3], &tmpval[4]);
            result->groups[i].channels[j].chassisid = tmpval[0];
            result->groups[i].channels[j].slot = tmpval[1];
            result->groups[i].channels[j].port = tmpval[2];
            result->groups[i].channels[j].au4 = tmpval[3];
            result->groups[i].channels[j].channel = tmpval[4];
            SDH_LOGINFO("line[%d] group[%d-%d]link[%d-%d]-%d,%d,%d,%d,%d", __LINE__, result->groups_num, \
                       i, result->groups[i].channels_num, j, result->groups[i].channels[j].chassisid, \
                       result->groups[i].channels[j].slot, result->groups[i].channels[j].port, \
                       result->groups[i].channels[j].au4, result->groups[i].channels[j].channel);
        }
    }

    CfgInvalidate(cfghd);
    return 0;

err_exit:
    return -1;
}

int sdhlib_init(void)
{
    memset(logfile, 0, sizeof(logfile));
    sprintf(logfile, "%s", SDHLIB_LOGFILE);
    logfd = fopen(logfile, "a+");
    if (logfd == NULL) {
        PRINT("sdhlib_init : fopen %s failed.", logfile);
        return -1;
    }

    sdh_hd = (struct sdhlib_sdh_hd *)malloc(sizeof(struct sdhlib_sdh_hd));
	if (!sdh_hd) {
        SDH_LOGERROR( "malloc failed.\n");
		return -1;
	}
	memset(sdh_hd, 0, sizeof(struct sdhlib_sdh_hd));
	memset(&sdh_hd->stat, 0, sizeof(struct sdhlib_stat));

    sdh_hd->fd0 = open(FPGA0_DEV_NAME, O_RDWR);
    sdh_hd->fd1 = open(FPGA1_DEV_NAME, O_RDWR);
    sdh_hd->fd2 = open(FPGA2_DEV_NAME, O_RDWR);
    sdh_hd->fd3 = open(FPGA3_DEV_NAME, O_RDWR);

    sdh_hd->swfd = open(SW_MODULE_NAME, O_RDWR);
    if (!sdh_hd->swfd) {
        SDH_LOGERROR("Failed to open swfd.");
        return -1;
    }

    if (sdhlib_get_borad_info(&sdh_hd->bdinfo) != 0) {
        return -1;
    }

	//LGWROPEN(logfile, SDHLIB_LOGLEVEL, SDHLIB_LOGSIZE);
    return 0;
}

int sdhlib_exit(void)
{
    if (sdh_hd->fd0) {
        close(sdh_hd->fd0);
    }
    if (sdh_hd->fd1) {
        close(sdh_hd->fd1);
    }
    if (sdh_hd->fd2) {
        close(sdh_hd->fd2);
    }
    if (sdh_hd->fd3) {
        close(sdh_hd->fd3);
    }
    if (sdh_hd->swfd) {
        close(sdh_hd->swfd); 
    }
    if (logfd) {
        fclose(logfd); 
    }
    free(sdh_hd);

    return 0;
}
