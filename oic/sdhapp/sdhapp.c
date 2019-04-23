/*
 * (C) Copyright 2018
 * liye <ye.li@raycores.com>
 *
 * sdhapp.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "oic.h"
#include "sdhlib.h"
#include "sgfplib.h"
#include "common.h"
#include "aplog.h"
#include "cconfig.h"

#define CHANNEL_MAX 64

void *__lgwr__handle = NULL;

static char *phy_type[] = {"155M", "622M", "2.5G", "10G"};
static char *ch_rate[] = {"C4", "C12", "C11"};
static char *svc_type[] = {"OTH", "GFP", "LAPS","PPP","ATM"};

void sdh_usage(void)
{
    printf("            sdhapp show :\n");
    printf("                hp-pinfo [fpga0|fpga1|fpga2|fpga3] port <1-2>\n");
    printf("                                                   : show info of hp port.\n");
    printf("                hp-slink  [fpga0|fpga1|fpga2|fpga3] port <1-2> <clear>\n");
    printf("                                                   : show sinfo of hp port or clear Bcode.\n");
    printf("                lp-pinfo [fpga0|fpga1|fpga2|fpga3] <clear> \n");
    printf("                                                   : show info of lp port or clear Bcode.\n");
    printf("                lp-slink  [fpga0|fpga1|fpga2|fpga3] port <1-16>\n");
    printf("                                                   : show sinfo of lp port .\n");
    printf("                lp-chan  [fpga0|fpga1|fpga2|fpga3] port <1-16>\n");
    printf("                                                   : show channel info of lp port .\n");
    printf("                hp-soh  port <1-4|13-16>\n");
    printf("                                                   : show sinfo of hp port .\n");
    printf("                lp-soh  [fpga0|fpga1|fpga2|fpga3] port <1-16>\n");               
    printf("                                                   : show sinfo of lp port .\n");
    printf("                payload  [fpga0|fpga1|fpga2|fpga3] port <1-16>\n");               
    printf("                                                   : show sinfo of payload .\n");
    printf("                inner                              : show inner rules.\n");
    printf("                stat <debug> <clear|bps>           : show fpga stats.\n");
    printf("                info                               : show board info.\n");
    printf("                gfp-info <fpga0|fpga1|fpga2|fpga3> : show gfp info by fpga.\n");
    printf("                gfp-result <fpga0|fpga1|fpga2|fpga3> : show gfp info by fpga.\n");
    printf("                E1-info                            : show E1 info.\n");
    printf("                port-status <1-50>                 : show port status.\n");
    printf("                port-stat <1-50>                   : show port stat.\n");
    printf("            sdhapp cfg :\n");
    printf("                inner <1-4|13-16> <1-64> [sel] <1-16>\n");
    printf("                                                   : cfg enable inner rule.\n");
    printf("                inner dis <1-4|13-16> <1-64>       : cfg dis inner rule.\n");
    printf("                sel [fpga0|fpga1|fpga2|fpga3] <0-255>\n");
    printf("                                                   : cfg sel .\n");
    printf("                local [fpga0|fpga1|fpga2|fpga3] <gfp|laps|ppp|atm|hdlc> [port-group 1-4]\n");
    printf("                                                   : cfg one protocal local rule. port-group. eg: 1,3 output to port1,3. \n");
    printf("                local [fpga0|fpga1|fpga2|fpga3] <gfp|laps|ppp|atm|hdlc> <dis>\n");
    printf("                                                   : cfg one protocal local rule is disable . \n");
    printf("                global [fpga0|fpga1|fpga2|fpga3] <gfp|laps|ppp|atm|hdlc> [sel 0-255]\n");
    printf("                                                   : cfg one protocal global rule. port-group. eg: 1,3 output to port1,3. \n");
    printf("                global [fpga0|fpga1|fpga2|fpga3] <gfp|laps|ppp|atm|hdlc> <dis>\n");
    printf("                                                   : cfg one protocal global rule is disable . \n");
    printf("                e1user [fpga0|fpga1|fpga2|fpga3] [local|global] [cfgfile|dis]\n");
    printf("                                                   : cfg e1user. \n");
    printf("                e1linkmap [fpga0|fpga1|fpga2|fpga3] [local|global] [cfgfile|dis]\n");
    printf("                                                   : cfg e1linkmap. \n");
    printf("                e164k [fpga0|fpga1|fpga2|fpga3] [local|global] [cfgfile|dis]\n");
    printf("                                                   : cfg e164kppp. \n");
    printf("                e1gfp [fpga0|fpga1|fpga2|fpga3] [local|global] [cfgfile|dis]\n");
    printf("                                                   : cfg e1gfp. \n");
    printf("                info chassisid <1-15> \n");
    printf("                                                   : cfg chassisid,slot,fpga-id. \n");
    printf("                clear <1-50>                       : clear port stat. 1-24: front ports, 25-50: switch ports.\n");
    printf("                fiber <1-50> <single|double>       : cfg port fiber type is single or double.\n");
    printf("                mtu <1-50> [1-16356]               : cfg port mtu.\n");
    printf("                work <1-50> <enable|disable>       : cfg port enable|disable.\n");
    return;
}

static void convert_format_uint64_decimal(char *buf, uint64_t n)
{
    int                 pw;
    char                *obuf;
    uint64_t            p10;
    int                 digit;
	int					comma = 46;

    static const struct pow10_s {
        uint32_t          hi, lo;
    } pow10[] = {
        { 0U,           1U              },
        { 0U,           10U             },
        { 0U,           100U            },
        { 0U,           1000U           },
        { 0U,           10000U          },
        { 0U,           100000U         },
        { 0U,           1000000U        },
        { 0U,           10000000U       },
        { 0U,           100000000U      },
        { 0U,           1000000000U     },
        { 2U,           1410065408U     },
        { 23U,          1215752192U     },
        { 232U,         3567587328U     },
        { 2328U,        1316134912U     },
        { 23283U,       276447232U      },
        { 232830U,      2764472320U     },
        { 2328306U,     1874919424U     },
        { 23283064U,    1569325056U     },
        { 232830643U,   2808348672U     },
        { 2328306436U,  2313682944U     },
    };

    obuf = buf;

    for (pw = 19; pw >= 0; pw--) {

        p10 = (((uint64_t) ((uint32_t)(pow10[pw].hi))) << 32) | ((uint64_t) ((uint32_t)(pow10[pw].lo)));

        for (digit = '0'; n >= p10; digit++) {
            n -= p10;
        }

        if (buf > obuf || digit != '0') {
            *buf++ = digit;
            if (comma != 0 && (pw % 3) == 0 && pw > 0) {
                *buf++ = comma;
            }
        }
    }

    if (buf == obuf) {
        *buf++ = '0';
    }

    *buf = 0;
}


int sdh_env_init(void)
{
    if (sdhlib_init() != 0) {
        printf("sdhlib_init failed.\n"); 
        return -1;
    }
    return 0;
}

int sdh_env_exit(void)
{
    if (sdhlib_exit() != 0) {
        printf("sdhlib_exit failed.\n"); 
        return -1;
    }
    return 0;
}

int sdh_cfg_inner_rule(char *argv[])
{
    uint8_t hp_port = 0;
    uint8_t hp_chan = 0;
    uint8_t sel = 0;
    uint8_t lp_chan = 0;
    uint8_t enable = 0;

    if (!strcmp(argv[0], "dis")) {
        hp_port = atoi(argv[1]);
        hp_chan = atoi(argv[2]);
    } else {
        enable = 1;
        hp_port = atoi(argv[0]);
        hp_chan = atoi(argv[1]);
        sel = atoi(argv[2]);
        lp_chan = atoi(argv[3]);
    }

    switch (hp_port) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 13:
        case 14:
        case 15:
        case 16:
            break;
        default:
            printf("hp port %d err.\n", hp_port);
            return -1;
    }

    if ((hp_chan < 1) || (hp_chan > 64)) {
        printf("hp chann %d err.\n", hp_chan);
        return -1;
    }

    if (!enable) {
        if (sdhlib_del_inner_sw_rule(hp_port, hp_chan) != 0) {
            return -1;
        }
        return 0;
    }

    if ((lp_chan > 16) || (lp_chan < 1)) {
        printf("lp chan %d err !!\n", lp_chan);
        return -1;
    }

    if (sdhlib_add_inner_sw_rule(hp_port, hp_chan, sel, lp_chan) != 0) {
        printf("add inner sw rule err !!\n");
        return -1;
    }
    return 0;
}

static int8_t conv_fpgaid(char **argv)
{
    int8_t id = 0;

    if (!strcmp(argv[0], "fpga0")) {
        id = 1;
    } else if (!strcmp(argv[0], "fpga1")) {
        id = 2;
    } else if (!strcmp(argv[0], "fpga2")) {
        id = 3;
    } else if (!strcmp(argv[0], "fpga3")) {
        id = 4;
    } else {
        id = 255;
        printf("unkown fpga name %s\n", argv[0]);
    }
    return id;
}

static void *sgfp_open(int fpgaid)
{
    void *sgfpfd = NULL;

    sgfpfd = sgfplib_open(fpgaid);
    if (!sgfpfd) {
        printf("Failed to open sgfpfd.\n"); 
    }
    return sgfpfd; 
}

int sdh_show_lp_pinfo(void *sgfpfd, char **argv, uint8_t fpgaid)
{
    struct board_info bd_info;
	struct fpga_board_runinfo_ex pinfo;
    uint8_t i = 0;
    uint8_t j = 0;
	char buff[1028];
	char tmp_phy_type[8];
	char tmp_ch_rate[8];
    int maxnum = 4;

    memset(buff, 0, sizeof(buff));
    memset(&pinfo, 0, sizeof(&pinfo));

    pinfo.start_port = 0;
    pinfo.clear = 0;
    if (argv[0] && (!strcmp(argv[0], "clear"))) {
        pinfo.clear = 1;
    }

    memset(&bd_info, 0, sizeof(&bd_info));
    if (sdhlib_get_borad_info(&bd_info) != 0) {
        printf("show lp-pinfo : sdhlib_get_borad_info failed \n");
        return -1;
    }
    if (bd_info.fpgas[fpgaid-1].type == 4) {
        maxnum = 8;
    }

    printf("port los stm1  phy    b1    b2    b3  auptr 0110rev 0110no " \
           "1001rev 1001no   c2 ch_rate  e1  nfm_e1\n");

    for (j = 0; j < maxnum; j++) {
        if  (sgfplib_get_fpga_bd_runinfo_ex(sgfpfd, &pinfo) != 0) {
            goto err; 
        }

        for (i = 0; i < FPGA_OI_PORTNUM; i++) {
            if (pinfo.ports[i].phy_type <= 3) {
                strcpy(tmp_phy_type, phy_type[pinfo.ports[i].phy_type]);
            } else {
                strcpy(tmp_phy_type, "0");
            }

            if (pinfo.ports[i].stm1_synced == 0)
                pinfo.ports[i].stm1_synced = 1;
            else if(pinfo.ports[i].stm1_synced == 1)
                pinfo.ports[i].stm1_synced = 0;

            if ((pinfo.ports[i].chan_rate >= 1) && (pinfo.ports[i].chan_rate <= 3)) {
                strcpy(tmp_ch_rate, ch_rate[pinfo.ports[i].chan_rate - 1]);
            }
            else {
                sprintf(tmp_ch_rate, "%u", pinfo.ports[i].chan_rate);
            }

            sprintf(buff, "%3d  %3u  %3u %4s %5u %5u %5u 0x%04x     %3u    %3u" \
                    "     %3u    %3u 0x%02x     %3s %2u %2u",
                    ((j*4)+i+1), pinfo.ports[i].los, pinfo.ports[i].stm1_synced,
                    tmp_phy_type, pinfo.ports[i].b1_cnt,
                    pinfo.ports[i].b2_cnt, pinfo.ports[i].b3_cnt, 
                    pinfo.ports[i].auptr_val, pinfo.ports[i].auptr_0110_rev,
                    pinfo.ports[i].auptr_0110_no, pinfo.ports[i].auptr_1001_rev,
                    pinfo.ports[i].auptr_1001_no,
                    pinfo.ports[i].c2_val, tmp_ch_rate,
                    pinfo.ports[i].e1_cnt, pinfo.ports[i].nfm_e1_cnt);

            printf("%s\n", buff);
        }
        pinfo.start_port += FPGA_OI_PORTNUM;
    }
    sgfplib_close(sgfpfd);
    return 0;
err:
    sgfplib_close(sgfpfd);
    return -1;
}

int sdh_show_hp_pinfo(void *sgfpfd, char **argv)
{
	struct fpga_board_runinfo_ex pinfo;
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t port = 0;
	char buff[1028];
	char tmp_phy_type[8];
	char tmp_ch_rate[8];

    if (strcmp(argv[0], "port")) {
        return -1;
    }

    port = atoi(argv[1]); 
    if ((port != 1) && (port != 2)) {
        return -1;
    }

    memset(buff, 0, sizeof(buff));
    memset(&pinfo, 0, sizeof(&pinfo));
    pinfo.clear = 0;
    if (argv[2] && (!strcmp(argv[2], "clear"))) {
        pinfo.clear = 1;
    }

    pinfo.start_port = (port - 1) * 64;
    printf("port los stm1  phy    b1    b2    b3  auptr 0110rev 0110no " \
           "1001rev 1001no   c2 ch_rate j1\n");
    for (j = 0; j < 16; j++) {
        if  (sgfplib_get_fpga_bd_runinfo_ex(sgfpfd, &pinfo) != 0) {
            goto err; 
        }

        for (i = 0; i < FPGA_OI_PORTNUM; i++) {
            if (pinfo.ports[i].phy_type <= 3) {
                strcpy(tmp_phy_type, phy_type[pinfo.ports[i].phy_type]);
            } else {
                strcpy(tmp_phy_type, "0");
            }

            if (pinfo.ports[i].stm1_synced == 0)
                pinfo.ports[i].stm1_synced = 1;
            else if(pinfo.ports[i].stm1_synced == 1)
                pinfo.ports[i].stm1_synced = 0;

            if ((pinfo.ports[i].chan_rate >= 1) && (pinfo.ports[i].chan_rate <= 3)) {
                strcpy(tmp_ch_rate, ch_rate[pinfo.ports[i].chan_rate - 1]);
            }
            else {
                sprintf(tmp_ch_rate, "%u", pinfo.ports[i].chan_rate);
            }

            sprintf(buff, "%3d  %3u  %3u %4s %5u %5u %5u 0x%04x     %3u    %3u" \
                    "     %3u    %3u 0x%02x     %3s 0x%08x%08x%08x%08x",
                    ((j*4)+i+1), pinfo.ports[i].los, pinfo.ports[i].stm1_synced,
                    tmp_phy_type, pinfo.ports[i].b1_cnt,
                    pinfo.ports[i].b2_cnt, pinfo.ports[i].b3_cnt, 
                    pinfo.ports[i].auptr_val, pinfo.ports[i].auptr_0110_rev,
                    pinfo.ports[i].auptr_0110_no, pinfo.ports[i].auptr_1001_rev,
                    pinfo.ports[i].auptr_1001_no,
                    pinfo.ports[i].c2_val, tmp_ch_rate,
                    pinfo.ports[i].j1_3, pinfo.ports[i].j1_2, 
                    pinfo.ports[i].j1_1, pinfo.ports[i].j1_0);

            printf("%s\n", buff);

            switch (pinfo.ports[i].phy_type) {
                case 0:
                    if (((j*4)+i+1) == 1)
                        goto ok_exit;
                    break;

                case 1:
                    if (((j*4)+i+1) == 4)
                        goto ok_exit;
                    break;

                case 2:
                    if (((j*4)+i+1) == 16)
                        goto ok_exit;
                    break;

                case 3:
                    break;

                default:
                    break;
            } 
        }
        pinfo.start_port += 4;
    }

ok_exit:
    sgfplib_close(sgfpfd);
    return 0;
err:
    sgfplib_close(sgfpfd);
    return -1;
}

static void print_linkinfo_lp(struct slink_info* info)
{
	char buff[1028] = "";
	char tmp_p_rate[8] = "";
	char tmp_ch_rate[8] = "";
	char tmp_svc_type[8] = "";
	char channel_num[8] = "";
    char v5err[8] = "";
    char k4err[8] = "";
    char sqerr[8] = "";
    char mfierr[8] = "";

    char tuptr[8] = "";
    char v5[8] = "";
    char v5cnt[8] = "";
    char rev0[8] = "";
    char no0[8] = "";
    char rev1[8] = "";
    char no1[8] = "";

	char ce1_sync[8] = "";
	char ce1_err[8] = "";
	char cnfr_e1_sync[8] = "";
	char cnfr_e1_err[8] = "";

	if (info->channel_rate == 1) {
		strcpy(channel_num, "*");
		strcpy(v5err, "*");
		strcpy(k4err, "*");
		strcpy(sqerr, "*");
		strcpy(mfierr, "*");
		strcpy(tuptr, "*");
		strcpy(v5, "*");
		strcpy(v5cnt, "*");
		strcpy(rev0, "*");
		strcpy(no0, "*");
		strcpy(rev1, "*");
		strcpy(no1, "*");
		strcpy(ce1_sync, "*");
		strcpy(ce1_err, "*");
		strcpy(cnfr_e1_sync, "*");
		strcpy(cnfr_e1_err, "*");
	}
	else {
		sprintf(channel_num, "%u", (info->channel + 1));
		sprintf(v5err, "%u", info->v5_sync_cnt);
		sprintf(k4err, "%u", info->k4_sync_cnt);
		sprintf(sqerr, "%u", info->sq_cnt);
		sprintf(mfierr, "%u", info->mfi_cnt);
		sprintf(tuptr, "%u", info->tuptr_val);
		sprintf(v5, "%u", info->v5_val);
		sprintf(v5cnt, "0x%04x", info->v5_cnt);
		sprintf(rev0, "%u", info->tuptr_0110_rev);
		sprintf(no0, "%u", info->tuptr_0110_no);
		sprintf(rev1, "%u", info->tuptr_1001_rev);
		sprintf(no1, "%u", info->tuptr_1001_no);
		sprintf(ce1_sync, "%u", info->e1_sync);
		sprintf(ce1_err, "%u", info->e1_sync_err);
		sprintf(cnfr_e1_sync, "%u", info->nfm_e1_sync);
		sprintf(cnfr_e1_err, "%u", info->nfm_e1_sync_err);
	}

	if (info->fiber_rate <= 3) {
		strcpy(tmp_p_rate, phy_type[info->fiber_rate]);
	}
	else {
		strcpy(tmp_p_rate, "0");
	}
	
	if (info->channel_rate >= 1 && info->channel_rate <= 3) {
		strcpy(tmp_ch_rate, ch_rate[info->channel_rate - 1]);
	}
	else {
		sprintf(tmp_ch_rate, "%u", info->channel_rate);
	}
	
	if (info->svc_type >= 0 && info->svc_type <= 4) {
		strcpy(tmp_svc_type, svc_type[info->svc_type]);
	}
	else {
		sprintf(tmp_svc_type, "%u", info->svc_type);
	}
    sprintf(buff, "%3u  %4s  %3s   %4s       %3u  %3u %3u  %3u  %5u %3u  0x%08x 0x%08x %4s    " \
            "%3s    %3s    %3s    %3s     %3s  %-1u  %-4u      %-1u      %-4u", (info->fiber + 1), tmp_p_rate,
            channel_num, tmp_ch_rate, info->vc_valid, info->is_lcas, info->is_member,
            info->is_last_member, info->mfi,info->sq, info->pre_gid, info->cur_gid, tmp_svc_type,
            v5err, k4err, "*",sqerr, mfierr,  info->e1_sync, info->e1_sync_err,
            info->nfm_e1_sync, info->nfm_e1_sync_err);

    printf("%s\n", buff);
}

static void print_linkinfo_hp(struct fpga_board_runinfo_port_ex *info)
{
	char buff[1028] = "";
	char tmp_p_rate[8] = "";
	char tmp_ch_rate[8] = "";
	char tmp_svc_type[8] = "";
	char channel_num[8] = "";
    char syncerr[8] = "";
    char sqerr[8] = "";
    char mfierr[8] = "";


    sprintf(channel_num, "%u", (info->channel + 1));
    sprintf(sqerr, "%u", info->sq_cnt);
    sprintf(mfierr, "%u", info->mfi_cnt);
    sprintf(syncerr, "%u", info->sync_cnt);

	if (info->phy_type <= 3) {
		strcpy(tmp_p_rate, phy_type[info->phy_type]);
	}
	else {
		strcpy(tmp_p_rate, "0");
	}
	
	if (info->chan_rate >= 1 && info->chan_rate <= 3) {
		strcpy(tmp_ch_rate, ch_rate[info->chan_rate - 1]);
	}
	else {
		sprintf(tmp_ch_rate, "%u", info->chan_rate);
	}
	
	if (info->svc_type >= 0 && info->svc_type <= 4) {
		strcpy(tmp_svc_type, svc_type[info->svc_type]);
	}
	else {
		sprintf(tmp_svc_type, "%u", info->svc_type);
	}
    sprintf(buff, "%3u  %4s  %3s   %4s       %3u  %3u %3u  %3u  %5u %3u  0x%08x  0x%08x    %4s    " \
            "%3s    %4s    %3s", (info->fiber + 1), tmp_p_rate,
            channel_num, tmp_ch_rate, info->vc_valid, info->is_lcas, info->is_member,
            info->is_last_member, info->mfi,info->sq, info->pre_gid, info->cur_gid, tmp_svc_type,
            syncerr, sqerr, mfierr);

    printf("%s\n", buff);
}

void print_linkinfo_chan(struct slink_info *info)
{
    char buff[1028] = "";
	char tmp_p_rate[8] = "";
	char tmp_ch_rate[8] = "";
	char channel_num[8] = "";
    char v5err[8] = "";
    char k4err[8] = "";
//  char c4err[8] = "";
    char sqerr[8] = "";
    char mfierr[8] = "";

    char tuptr[8] = "";
    char v5[8] = "";
    char v5cnt[8] = "";
    char rev0[8] = "";
    char no0[8] = "";
    char rev1[8] = "";
    char no1[8] = "";

	char ce1_sync[8] = "";
	char ce1_err[8] = "";
	char cnfr_e1_sync[8] = "";
	char cnfr_e1_err[8] = "";

    sprintf(channel_num, "%u", (info->channel + 1));
    sprintf(v5err, "%u", info->v5_sync_cnt);
    sprintf(k4err, "%u", info->k4_sync_cnt);
    sprintf(sqerr, "%u", info->sq_cnt);
    sprintf(mfierr, "%u", info->mfi_cnt);
    sprintf(tuptr, "%u", info->tuptr_val);
    sprintf(v5, "%u", info->v5_val);
    sprintf(v5cnt, "0x%04x", info->v5_cnt);
    sprintf(rev0, "%u", info->tuptr_0110_rev);
    sprintf(no0, "%u", info->tuptr_0110_no);
    sprintf(rev1, "%u", info->tuptr_1001_rev);
    sprintf(no1, "%u", info->tuptr_1001_no);
    sprintf(ce1_sync, "%u", info->e1_sync);
    sprintf(ce1_err, "%u", info->e1_sync_err);
    sprintf(cnfr_e1_sync, "%u", info->nfm_e1_sync);
    sprintf(cnfr_e1_err, "%u", info->nfm_e1_sync_err);
    sprintf(tmp_ch_rate, "%s", ch_rate[info->channel_rate - 1]);

    sprintf(buff, "%3u  %4s  %3s   %4s   %3s %3s %6s     %3s    %3s     %3s    %3s"\
            "    %3s    %3s         %3s        %3s", (info->fiber + 1), tmp_p_rate,
            channel_num, tmp_ch_rate, tuptr, v5, v5cnt, rev0, no0, rev1, no1,
            ce1_sync, ce1_err, cnfr_e1_sync, cnfr_e1_err);

	printf("%s\n", buff);
}

int sdh_show_lp_slink(void *gfpfd, char **argv)
{
    struct slink_info *g_link = NULL;
    uint8_t i = 0;
    uint8_t port = 0;
    
    if (strcmp(argv[0], "port")) {
        return -1;
    }
    port = atoi(argv[1]);
    if ((port < 1) && (port > 16)) {
        return -1;
    }

    g_link = (struct slink_info*)malloc(sizeof(struct slink_info));
	if (g_link == NULL) {
		return -1;
	}
    
    if (sgfplib_set_linkinfo_start(gfpfd) != 0)
        goto err;

    printf("  p p_rate  ch ch_rate  vc_sync lcas mem l_mem   mfi  sq  pre_gid    cur_gid ch_type" \
           " v5_err k4_err c4_err sq_err mfi_err e1 e1_err nfr_e1 nfr_e1_err\n");

    g_link->fiber = (port - 1);
    for (i = 0; i < CHANNEL_MAX; i++) {
        g_link->channel = i;
        if (sgfplib_get_linkinfo(gfpfd, g_link) != 0)
            goto err;

        print_linkinfo_lp(g_link);
        //c4 only one channel
        if (g_link->channel_rate == 1) {
            break;
        }
    }

    if (sgfplib_set_linkinfo_end(gfpfd) != 0)
        goto err;

    free(g_link);
    return 0;

err:
    free(g_link);
    return -1;
}


int sdh_show_lp_chan(void *sgfpfd, char **argv)
{
    struct slink_info *info = NULL;
    uint8_t i = 0;
    uint8_t port = 0;
    
    if (strcmp(argv[0], "port")) {
        return -1;
    }
    port = atoi(argv[1]);
    if ((port < 1) && (port > 16)) {
        return -1;
    }

    info = (struct slink_info*)malloc(sizeof(struct slink_info));
	if (info == NULL) {
		return -1;
	}

    if (sgfplib_set_linkinfo_start(sgfpfd) != 0)
        goto err;

    printf("  p p_rate  ch ch_rate Tuptr  V5 v5_err " \
           "0110rev 0110no 1001rev 1001no"\
           " e1_sync e1_err nfr_e1_sync nfr_e1_err\n");

    for (i = 0; i < CHANNEL_MAX; i++) {
        memset(info, 0, sizeof(info));
        info->fiber = (port - 1);
        info->channel = i;
        if (sgfplib_get_linkinfo(sgfpfd, info) != 0)
            goto err;

        print_linkinfo_chan(info);
    }

    if (sgfplib_set_linkinfo_end(sgfpfd) != 0)
        goto err;

    free(info);
    return 0;

err:
    free(info);
    return -1;
}

int sdh_show_hp_slink(void *sgfpfd, char **argv)
{
	struct fpga_board_runinfo_port_ex pinfo;
    uint8_t i = 0;
    uint8_t port = 0;
    
    if (strcmp(argv[0], "port")) {
        return -1;
    }
    port = atoi(argv[1]);
    if ((port < 1) && (port > 2)) {
        return -1;
    }

    if (sgfplib_set_linkinfo_start(sgfpfd) != 0)
        goto err;

    printf("  p p_rate  ch ch_rate  vc_sync lcas mem l_mem   mfi  sq  pre_gid     cur_gid        ch_type" \
           " syncerr  sq_err mfi_err\n");

    for (i = 0; i < CHANNEL_MAX; i++) {
        memset(&pinfo, 0, sizeof(&pinfo));
        pinfo.fiber = (((port - 1) & 0x03) << 6) | i;
        pinfo.channel = 0;
        if (sgfplib_get_hp_linkinfo(sgfpfd, &pinfo) != 0)
            goto err;

        print_linkinfo_hp(&pinfo);

        switch (pinfo.phy_type) {
                case 0:
                    if ((i+1) == 1)
                        goto ok_exit;
                    break;

                case 1:
                    if ((i+1) == 4)
                        goto ok_exit;
                    break;

                case 2:
                    if ((i+1) == 16)
                        goto ok_exit;
                    break;

                case 3:
                    break;

                default:
                    break;
        }
    }

ok_exit:
    if (sgfplib_set_linkinfo_end(sgfpfd) != 0)
        goto err;

    return 0;

err:
    return -1;
}


int sdh_cfg_sel(char **argv)
{
    unsigned char sel = -1;
    uint8_t fpgaid = 0;
    void *fd = NULL;

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff) {
        return -1;
    }

    sel = atoi(argv[1]);

    fd = sgfp_open(fpgaid);
    if (!fd) {
        return -1;
    }

    if (sgfplib_set_spu_selnum(fd, sel) != 0) {
        sgfplib_close(fd);
        return -1;
    }

    sgfplib_close(fd);
    return 0;
}

int protocal_map(char **argv, uint8_t *id)
{

    if (!strcmp(argv[0], "gfp")) {
        *id = PROTOCOL_GFP;
    } else if (!strcmp(argv[0], "laps")) {
        *id = PROTOCOL_LAPS;
    } else if (!strcmp(argv[0], "ppp")) {
        *id = PROTOCOL_PPP_POS;
    } else if (!strcmp(argv[0], "atm")) {
        *id = PROTOCOL_ATM;
    } else if (!strcmp(argv[0], "hdlc")) {
        *id = PROTOCOL_HDLC;
    //} else if (!strcmp(argv[0], "e1usr")) {
    //    *id = PROTOCOL_E1USER;
    //} else if (!strcmp(argv[0], "e1linkmap")) {
    //    *id = PROTOCOL_E1LINKMAP;
    //} else if (!strcmp(argv[0], "2m64kppp")) {
    //    *id = PROTOCOL_PPP_2M64K;
    } else {
        return -1;
    }

    return 0;
}

int sdh_cfg_local(char **argv)
{
    uint8_t outp[4];
    char *port_str = NULL;
    struct proto_local_rule rule;
    uint8_t i = 0;

    if (!argv[0] || !argv[1] || !argv[2])
        return -1;

    memset(&rule, 0, sizeof(struct proto_local_rule));

    rule.fpgaid = conv_fpgaid(&argv[0]);
    if (rule.fpgaid == 0xff)
        return -1;

    if (protocal_map(&argv[1], &rule.protocol) != 0) {
        return -1;
    }

    memset(outp, 0, sizeof(outp));
    
    rule.pbmp = 0;
    if (!strcmp(argv[2], "dis")) {
        goto set_rule;
    }

    port_str = strtok (argv[2], ",");
    outp[0] = atoi(port_str) - 1;
    if ((outp[0] <= 4) && (outp > 0)) {
        rule.pbmp = (1 << outp[0]);
    } else {
        return -1; 
    }

    if (port_str != NULL) {
        for (i = 1; i < 4; i++) {
            port_str = strtok (NULL, ",");
            if (port_str != NULL) {
                outp[i] = atoi(port_str) - 1;
                if ((outp[0] <= 4) && (outp > 0)) {
                    rule.pbmp |= (1 << outp[i]);
                } else {
                    return -1; 
                }
            }
        }
    }

    printf("local output port %d %d %d %d\n", outp[0], outp[1], outp[2], outp[3]);

set_rule:
    if (sdhlib_set_local_rule(&rule) != 0) {
        return -1;
    }

    return 0;
}


int sdh_cfg_global(char **argv)
{
    int8_t outs[GLOBAL_SEL_MAXNUM];
    char *port_str = NULL;
    struct proto_global_rule rule;
    uint8_t i = 0;

    if (!argv[0] || !argv[1] || !argv[2])
        return -1;

    memset(&rule, 0, sizeof(struct proto_global_rule));

    rule.fpgaid = conv_fpgaid(&argv[0]);
    if (rule.fpgaid == 0xff)
        return -1;

    if (protocal_map(&argv[1], &rule.protocol) != 0) {
        return -1;
    }

    memset(outs, 0, sizeof(outs));
    if (!strcmp(argv[2], "dis")) {
        rule.sels_bmp = 0;
        memset(rule.sels, 0,sizeof(rule.sels));
        goto set_rule;
    }

    port_str = strtok (argv[2], ",");
    if (port_str == NULL)
        return -1;

    outs[0] = atoi(port_str);
    rule.sels[0] = outs[0];

    rule.sels_bmp = 1; 
    if (port_str != NULL) {
        for (i = 1; i < GLOBAL_SEL_MAXNUM; i++) {
            port_str = strtok (NULL, ",");
            if (port_str != NULL) {
                outs[i] = atoi(port_str);
                rule.sels_bmp |= (1 << i); 
                rule.sels[i] = outs[i];
            }
        }
    }

    printf("global bmp 0x%x rule.selsut port %d %d %d %d\n", rule.sels_bmp, rule.sels[0], rule.sels[1], rule.sels[2], rule.sels[3]);

set_rule:
    if (sdhlib_set_global_rule(&rule) != 0) {
        return -1;
    }

    return 0;
}

int sdh_get_board_info(void)
{
    struct board_info bd_info;
    char *fpga_type[] = {"OTH", "HP", "LP", "64KPPP", "GFP"};
    char *fpga_subtype[] = {"OTH", "10g","2.5g", "622m", "155m"};
    char *boardname[] = {"OTH", "XSCB", "MACB", "OI_AMC"};
    uint8_t i = 0;

    memset(&bd_info , 0, sizeof(&bd_info));
    if (sdhlib_get_borad_info(&bd_info) != 0) {
        return -1; 
    }

    printf("sdh board info : \n");
    printf("slot %d \n", bd_info.slot);
    printf("hardware version %d \n", bd_info.hw_version);
    printf("software version %d \n", bd_info.sw_version);
    printf("board type       %s \n", boardname[bd_info.boardtype]);
    for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {
        printf("fpga[%d] present [%s] type [%s] subtype [%s] status [%s] date [%x-%x-%x] ver [D%x] \n", \
               (i+1), bd_info.fpgas[i].present ? "exisit": "not exisit", \
               fpga_type[bd_info.fpgas[i].type], \
               (bd_info.fpgas[i].subtype)<5 ? fpga_subtype[bd_info.fpgas[i].subtype]:"OTH" ,\
               bd_info.fpgas[i].status ? "ok":"err", bd_info.fpgas[i].date[DATE_IDX_YEAR], bd_info.fpgas[i].date[DATE_IDX_MONTH], \
              bd_info.fpgas[i].date[DATE_IDX_DAY], bd_info.fpgas[i].version);
    }
    return 0;
}

int sdh_get_board_stat(int argc, char **argv)
{
    struct sdh_fpga_stat stat;
    struct board_stat bd_stat;
    void *fd = NULL;
    uint8_t i = 0;
    uint8_t debug = 0;
    uint8_t type = 0;
    struct sdh_fpga_stat *all_stat, *one_stat;
    unsigned long fpgas_info[8];
    int id = 0;

    if (argc > 0) {
        if (argv[0]) {
            if (!strcmp(argv[0], "debug")) {
                debug = 1;
                type = SDH_BOARD_STAT_TYPE_PKTS;
            } else if (!strcmp(argv[0], "clear")) {
                type = SDH_BOARD_STAT_TYPE_CLEAR;
            } else if (!strcmp(argv[0], "bps")) {
                type = SDH_BOARD_STAT_TYPE_BYTES;
            } else {
                return -1;
            }
        }

        if (argv[1]) {
            if (!strcmp(argv[1], "debug")) {
                debug = 1;
            } else if (!strcmp(argv[1], "clear")) {
                type = SDH_BOARD_STAT_TYPE_CLEAR;
            } else if (!strcmp(argv[1], "bps")) {
                type = SDH_BOARD_STAT_TYPE_BYTES;
            } else {
                return -1;
            }
        }
    }

    if(debug) {
        all_stat = (struct sdh_fpga_stat *)malloc(sizeof(struct sdh_fpga_stat) * BOARD_FPGA_MAXNUM);
        if (!all_stat) {
            printf("malloc err. \n");
            return -1;
        }

        memset(fpgas_info, 0, sizeof(fpgas_info));
        memset(all_stat, 0, sizeof(struct sdh_fpga_stat) * BOARD_FPGA_MAXNUM);

        for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {
            memset(&stat, 0, sizeof(&stat));
            fd = sgfp_open((i + 1));
            if (!fd) {
                free(all_stat);
                return -1;
            }

            if (type == SDH_BOARD_STAT_TYPE_CLEAR) {
                if (sdhlib_set_board_stat_clear(fd) != 0) {
                    free(all_stat);
                    return -1;
                }
            } else if (type == SDH_BOARD_STAT_TYPE_BYTES) {
                if (sdhlib_set_board_stat_bytes(fd) != 0) {
                    free(all_stat);
                    return -1;
                }
            } else if (type == SDH_BOARD_STAT_TYPE_PKTS) {
                if (sdhlib_set_board_stat_pkts(fd) != 0) {
                    free(all_stat);
                    return -1;
                }
            } else {
                printf("Unkown type %d \n", type);
                free(all_stat);
                return -1;
            }

            one_stat = all_stat + (sizeof(struct sdh_fpga_stat) * i);
            if (sdhlib_get_sdh_fpga_stat(fd, one_stat) != 0) {
                printf("get board fpga %d stat failed.\n", i); 
                continue;
            }

            id = i * 2;
            fpgas_info[id] = one_stat->task_p;
            fpgas_info[id + 1] = one_stat->ch_err;

            sgfplib_close(fd);
        }

        printf("\ntask_p_0   ch_err_0   task_p_1   ch_err_1   task_p_2   ch_err_2   task_p_3   ch_err_3\n");
        printf("%-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu \n", \
              fpgas_info[0], fpgas_info[1],fpgas_info[2],fpgas_info[3],fpgas_info[4],fpgas_info[5], \
              fpgas_info[6],fpgas_info[7]);

        for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {

            one_stat = all_stat + (sizeof(struct sdh_fpga_stat) * i);

            printf("\nid gfp        atm        pos        e1usr      e1lm       64kppp      tug        laps       gfp-tx    rx-los\n");
            printf("%2d %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu\n",
                   (i+1), one_stat->gfp_cnt, one_stat->atm_cnt, one_stat->ppp_cnt, one_stat->e1user_cnt, one_stat->e1lm_cnt, \
                   one_stat->e164k_ppp_cnt, one_stat->hp2lp_cnt, one_stat->laps_cnt, one_stat->gfp_tx_cnt, one_stat->rx_los_cnt);

            printf("id gfp_head   gfp_fisu   atm_head   atm_fisu   ppp_head   ppp_fisu   laps_head  laps_fisu  atm-tx      cx_rx_cnt\n");

            printf("%2d %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu\n",
                   (i + 1), one_stat->gfp_ok, one_stat->gfp_fisu, one_stat->atm_ok, one_stat->atm_fisu, one_stat->ppp_ok, \
                   one_stat->ppp_fisu, one_stat->laps_ok, one_stat->laps_fisu, one_stat->atm_tx_cnt, one_stat->cx_rx_cnt);

            printf("id gfp_we_err gfp_los    atm_we_err atm_los    ppp_we_err ppp_los    laps_we_err laps_los   ppp-tx     lap-tx\n");
            printf("%2d %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu %-10lu  %-10lu %-10lu %-10lu\n",
                   (i + 1), one_stat->gfp_we_err, one_stat->gfp_los, one_stat->atm_we_err, one_stat->atm_los, one_stat->ppp_we_err,
                    one_stat->ppp_los, one_stat->laps_we_err, one_stat->laps_los, one_stat->ppp_tx_cnt, one_stat->lap_tx_cnt);

            printf("id local_eth0 local_eth1 local_eth2 local_eth3 global_eth0 global_eth1 global_eth2 global_eth3 e3-tx     e3-rx\n");
            printf("%2d %-10lu %-10lu %-10lu %-10lu %-10lu  %-10lu  %-10lu  %-10lu  %-10lu %-10lu\n",
                   (i + 1), one_stat->local_eth0, one_stat->local_eth1, one_stat->local_eth2, one_stat->local_eth3, one_stat->sw_eth0,
                   one_stat->sw_eth1, one_stat->sw_eth2, one_stat->sw_eth3, one_stat->e3_tx_cnt, one_stat->e3_rx_cnt);

        }
        free(all_stat);

    } else {
        memset(&bd_stat, 0,sizeof(bd_stat));
        if (sdhlib_get_board_stat(&bd_stat) != 0) {
            return -1;
        }

        printf("id gfp        atm        pos        e1usr      e1lm       64kppp      tug         laps\n");
        for (i = 0; i < bd_stat.fpgas_num ; i++) {
            printf("%2d %-10llu %-10llu %-10llu %-10llu %-10llu %-10llu %-10llu %-10llu\n",
                   (i+1), bd_stat.fpgas[i].gfp_rpkts, bd_stat.fpgas[i].atm_rpkts,
                   bd_stat.fpgas[i].pos_rpkts, bd_stat.fpgas[i].e1_rpkts, 0ull,
                   0ull, bd_stat.fpgas[i].tug_rpkts, bd_stat.fpgas[i].laps_rpkts);
        }
    }
    return 0;
}

int sdh_get_soh_lpinfo(int argc, char **argv)
{
    uint8_t fpgaid = 0;
    uint8_t port = 0;
    uint8_t i = 0;
    struct soh_lpinfo info;

    if (argc != 3) {
        printf("%d err: %s %s \n",argc, argv[0], argv[1]);
        return -1; 
    }

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    if (!strcmp(argv[1], "port")) {
        port = atoi(argv[2]);
        if ((port < 1) || (port > 16)) {
            printf("port err %d \n", port);
            return -1;
        }
    }

    memset(&info, 0, sizeof(struct soh_lpinfo));
    if (sdhlib_get_soh_lpinfo(fpgaid, port, &info) != 0) {
        return -1;
    }
    printf("id c2 chan v5 e1valid e1type\n");
    for (i = 0; i < 63; i++) {
        printf("%2d %2d %2d 0x%04x %-6d %d \n", 
               fpgaid, info.c2, (i+1), info.channels[i].v5, info.channels[i].e1valid, info.channels[i].e1type);
    }

    return 0;
}

int sdh_get_soh_hpinfo(int argc, char **argv)
{
    uint8_t port = 0;
    uint8_t i = 0;
    struct soh_hpinfo info;

    if (argc != 2) {
        printf("err: %s %s \n", argv[0], argv[1]);
        return -1; 
    }

    if (!strcmp(argv[0], "port")) {
        port = atoi(argv[1]);
    }

    memset(&info, 0, sizeof(struct soh_hpinfo));
    if (sdhlib_get_soh_hpinfo(port, &info) != 0) {
        return -1; 
    }
    printf("port speed stm1 b1   b2   b3   c2 j1-0 j1-1 j1-2 j1-3\n");
    for (i = 0; i < info.stm1s_num; i++) {
        printf("%2d   %2d    %2d %4d %4d %4d %2d 0x%x 0x%x 0x%x 0x%x\n" , port , info.speed, (i+1), \
                info.stm1s[i].b1, info.stm1s[i].b2, info.stm1s[i].b3, info.stm1s[i].c2,\
              info.stm1s[i].j1_0, info.stm1s[i].j1_1, info.stm1s[i].j1_2, info.stm1s[i].j1_3);
    }
    return 0;
}

int sdh_get_payload(int argc, char **argv)
{
    struct payload_info pinfo;
    struct payload_hpinfo *hpp = NULL;
    struct payload_lpinfo *lpp = NULL;
    struct payload_tug3 *tug3 = NULL;
    struct payload_tug2 *tug2 = NULL;
    struct payload_c1x *c1x = NULL;
    uint8_t fpgaid = 0;
    uint8_t port = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t k = 0;

    if (argc != 3) {
        printf("argc != 3, err: %s %s %s\n", argv[0], argv[1], argv[2]);
        return -1; 
    }
    memset(&pinfo, 0, sizeof(struct payload_info));

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    if (!strcmp(argv[1], "port")) {
        port = atoi(argv[2]);
        if ((port < 1) || (port > 16)) {
            printf("port err %d \n", port);
            return -1;
        }
    }

    if (sdhlib_get_payload_info(port, fpgaid, &pinfo) != 0) {
        return -1;
    }

    if (pinfo.type == FPGA_TYPE_HP) {
        hpp = &pinfo.info.hp;
        printf("port speed stm1 stm1-type \n");
        for (i = 0; i < hpp->stm1s_num; i++) {
            printf("%2d   %2d    %2d   %2d\n", hpp->port, hpp->speed,\
                  hpp->stm1s[i].id,  hpp->stm1s[i].type);
        }
    } else if (pinfo.type == FPGA_TYPE_LP) {
        lpp = &pinfo.info.lp;
        printf("port type tug3 type tug2 type c1x type\n");
        for (i = 0; i < lpp->tug3s_num; i++) {
            tug3 = &lpp->tug3s[i];
        
            for (j = 0; j < tug3->tug2s_num; j++) {
                tug2 = &tug3->tug2s[j];

                for (k = 0; k < tug2->c1xs_num; k++) {
                    c1x = &tug2->c1xs[k];
                    printf("%2d   %2d   %2d   %2d   %2d   %2d   %2d  %2d \n",
                           lpp->id, lpp->type, tug3->id, tug3->type, 
                           tug2->id, tug2->type, c1x->id, c1x->type);

                }
            }
        }
    } else {
        return -1;
    }

    return 0;
}

int cfg_e1map(char *filename, char *section, void *map)
{
    struct proto_e1linkmap_map *e1lmp_map = NULL;
    struct proto_e1user_map *e1user_map = NULL;
    struct proto_2m64kppp_map *e164k_map = NULL;
    struct proto_2mgfp_map *gfp_map = NULL;
    unsigned long  cfghd;
    char id[] = {"e1"};
    uint8_t secnum = 0;
    uint8_t idnum = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    char idval[64];
    int tmpval[8];

    cfghd = CfgInitialize(filename);
    if (cfghd == 0ul) {
        printf("Failed to CfgInitialize %s.\n", filename);
        return -1;
    }

    secnum = CfgGetCount(cfghd, section, NULL, 0);
    if (secnum < 1) {
        printf("section[%s] count is %d in %s .\n", section, secnum, filename);
        CfgInvalidate(cfghd);
        return -1;
    }

    for (j = 0; j < secnum; j++) {

        idnum = CfgGetCount(cfghd, section, id, (j+1));
        if (!idnum) {
            printf("section[%s] idnum[e1] is %d in %s .\n", section, idnum, filename);
            continue;
        }

        for (i = 0; i < idnum; i++) {
            memset(idval, 0, sizeof(idval));
            if (CfgGetValue(cfghd, section, id, idval, (i+1), (j+1)) < 0) {
                break;
            }

            memset(tmpval, 0, sizeof(tmpval));
            if (!strcmp(section, "LOCAL_E1USER") || !strcmp(section, "GLOBAL_E1USER")) {

                sscanf(idval, "%d,%d-%d-%d,%d,%d,%d", &tmpval[0], &tmpval[1], &tmpval[2], &tmpval[3], 
                       &tmpval[4],&tmpval[5],&tmpval[6]);

                e1user_map = map;
                e1user_map->channel = tmpval[0];
                e1user_map->e1 = tmpval[1];
                e1user_map->sel = tmpval[2];
                e1user_map->user_chassisid = tmpval[3];
                e1user_map->user_slot = tmpval[4];
                e1user_map->user_port = tmpval[5];
                e1user_map->user_e1 = tmpval[6];
                e1user_map->enable = 1;

                if (sdhlib_set_e1user_map(map) != 0) {
                    printf("sdhlib_set_e1user_map:  %s err.\n", idval);
                }

            } else if (!strcmp(section, "LOCAL_E1LINKMAP") || !strcmp(section, "GLOBAL_E1LINKMAP")) {

                sscanf(idval, "%d,%d-%d-%d-%d,%d,%d,%d", &tmpval[0], &tmpval[1], &tmpval[2], &tmpval[3], 
                       &tmpval[4],&tmpval[5], &tmpval[6], &tmpval[7]);

                e1lmp_map = map;
                e1lmp_map->channel = tmpval[0];
                e1lmp_map->e1 = tmpval[1];
                e1lmp_map->sel = tmpval[2];

                e1lmp_map->lmp_sel = tmpval[3];
                e1lmp_map->lmp_chassisid = tmpval[4];
                e1lmp_map->lmp_slot = tmpval[5];
                e1lmp_map->lmp_port = tmpval[6];
                e1lmp_map->lmp_e1 = tmpval[7];
                e1lmp_map->enable = 1;

                if (sdhlib_set_e1linkmap_map(map) != 0) {
                    printf("sdhlib_set_e1linkmap_map:  %s err.\n", idval);
                }

            } else if (!strcmp(section, "LOCAL_E164K") || !strcmp(section, "GLOBAL_E164K")) {

                sscanf(idval, "%d,%d-%d-%d-%d,%d,%d,%d", &tmpval[0], &tmpval[1], &tmpval[2], &tmpval[3],
                       &tmpval[4],&tmpval[5], &tmpval[6], &tmpval[7]);

                e164k_map = map;
                e164k_map->channel = tmpval[0];
                e164k_map->e1 = tmpval[1];
                e164k_map->sel = tmpval[2];

                e164k_map->ppp_sel = tmpval[3];
                e164k_map->ppp_chassisid = tmpval[4];
                e164k_map->ppp_slot = tmpval[5];
                e164k_map->ppp_port = tmpval[6];
                e164k_map->ppp_e1 = tmpval[7];
                e164k_map->enable = 1;

                if (sdhlib_set_2m64kppp_map(map) != 0) {
                    printf("sdhlib_set_2m64kppp_map:  %s err.\n", idval);
                }

            } else if (!strcmp(section, "LOCAL_E1GFP") || !strcmp(section, "GLOBAL_E1GFP")) {

                sscanf(idval, "%d,%d-%d-%d-%d,%d,%d,%d", &tmpval[0], &tmpval[1], &tmpval[2], &tmpval[3],
                       &tmpval[4],&tmpval[5], &tmpval[6], &tmpval[7]);

                gfp_map = map;
                gfp_map->channel = tmpval[0];
                gfp_map->e1 = tmpval[1];
                gfp_map->sel = tmpval[2];

                gfp_map->gfp_sel = tmpval[3];
                gfp_map->gfp_chassisid = tmpval[4];
                gfp_map->gfp_slot = tmpval[5];
                gfp_map->gfp_port = tmpval[6];
                gfp_map->gfp_e1 = tmpval[7];
                gfp_map->enable = 1;

                if (sdhlib_set_2mgfp_map(map) != 0) {
                    printf("sdhlib_set_2mgfp_map:  %s err.\n", idval);
                }

            } else {
                CfgInvalidate(cfghd);
                return -1;
            }
        }
    }

	CfgInvalidate(cfghd);
    return 0;
}

static int cfg_e1map_dis(char *proto, void *map)
{
    struct proto_e1linkmap_map *e1lmp_map = NULL;
    struct proto_e1user_map *e1user_map = NULL;
    struct proto_2m64kppp_map *e164k_map = NULL;
    struct proto_2mgfp_map *e1gfp_map = NULL;
    int i, j, k;

    if (!strcmp(proto, "e1user")) {
        e1user_map = map;

        for (i = 1; i <= 4; i++) {
            e1user_map->fpgaid = i;

            for (j = 1; j <= 16; j++) {
                e1user_map->channel = j;

                for (k = 1; k <= 63; k++) {
                    e1user_map->e1 = k;
                    e1user_map->sel = 0;
                    e1user_map->user_chassisid = 0;
                    e1user_map->user_slot = 0;
                    e1user_map->user_port = 0;
                    e1user_map->user_e1 = 0;
                    e1user_map->enable = 0;

                    if (sdhlib_set_e1user_map(map) != 0) {
                        printf("Failed sdhlib_set_e1user_map.\n");
                    }
                }
            }
        }

    } else if (!strcmp(proto, "e1linkmap")) {
        e1lmp_map = map;

        for (i = 1; i <= 4; i++) {
            e1lmp_map->fpgaid = i;

            for (j = 1; j <= 16; j++) {
                e1lmp_map->channel = j;

                for (k = 1; k <= 63; k++) {
                    e1lmp_map->e1 = k;
                    e1lmp_map->sel = 0;
                    e1lmp_map->lmp_chassisid = 0;
                    e1lmp_map->lmp_slot = 0;
                    e1lmp_map->lmp_port = 0;
                    e1lmp_map->lmp_e1 = 0;
                    e1lmp_map->enable = 0;

                    if (sdhlib_set_e1linkmap_map(map) != 0) {
                        printf("Failed sdhlib_set_e1linkmap_map.\n");
                    }
                }
            }
        }


    } else if (!strcmp(proto, "e164k")) {
        e164k_map = map;

        for (i = 1; i <= 4; i++) {
            e164k_map->fpgaid = i;

            for (j = 1; j <= 16; j++) {
                e164k_map->channel = j;

                for (k = 1; k <= 63; k++) {
                    e164k_map->e1 = k;
                    e164k_map->sel = 0;
                    e164k_map->ppp_chassisid = 0;
                    e164k_map->ppp_slot = 0;
                    e164k_map->ppp_port = 0;
                    e164k_map->ppp_e1 = 0;
                    e164k_map->enable = 0;

                    if (sdhlib_set_2m64kppp_map(map) != 0) {
                        printf("Failed sdhlib_set_2m64kppp_map.\n");
                    }
                }
            }
        }

    } else if (!strcmp(proto, "e1gfp")) {
        e1gfp_map = map;

        for (i = 1; i <= 4; i++) {
            e1gfp_map->fpgaid = i;

            for (j = 1; j <= 16; j++) {
                e1gfp_map->channel = j;

                for (k = 1; k <= 63; k++) {
                    e1gfp_map->e1 = k;
                    e1gfp_map->sel = 0;
                    e1gfp_map->gfp_chassisid = 0;
                    e1gfp_map->gfp_slot = 0;
                    e1gfp_map->gfp_port = 0;
                    e1gfp_map->gfp_e1 = 0;
                    e1gfp_map->enable = 0;

                    if (sdhlib_set_2mgfp_map(map) != 0) {
                        printf("Failed sdhlib_set_2mgfp_map.\n");
                    }
                }
            }
        }

    } else {
        printf("Unkown protocal %s \n", proto);
        return -1;
    }

    return 0;
}

int sdh_cfg_e1linkmap(int argc, char **argv)
{
    struct proto_e1linkmap_map map;
    uint8_t fpgaid = 0;
    char *section = NULL;

    if (argc != 3) {
        return -1;
    }

    memset(&map, 0, sizeof(struct proto_e1linkmap_map));

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    map.fpgaid = fpgaid;

    /* argv[1]: rule argv[2]: cfgfile */
    if ( (!argv[1]) || (!argv[2]) )
        return -1;


    if ( !strcmp(argv[1], "local") ) {
        map.rule = E1_RULE_LOCAL;
        section = "LOCAL_E1LINKMAP";
    } else if ( !strcmp(argv[1], "global") ) {
        map.rule = E1_RULE_GLOBAL;
        section = "GLOBAL_E1LINKMAP";
        
    } else {
        printf("Unkown rule %s.", argv[1]);
        return -1; 
    }

    if (!strcmp(argv[2], "dis")) {
        if (cfg_e1map_dis("e1linkmap", &map) != 0) {
            return -1;
        }
    } else {
        if (cfg_e1map(argv[2], section, &map) != 0) {
            return -1;
        }
    }

    return 0;
}

int sdh_cfg_e1user(int argc, char **argv)
{
    struct proto_e1user_map map;
    uint8_t fpgaid = 0;
    char *section = NULL;

    if (argc != 3){
        return -1;
    }

    memset(&map, 0, sizeof(struct proto_e1user_map));

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    map.fpgaid = fpgaid;

    /* argv[1]: rule argv[2]: cfgfile */
    if ( (!argv[1]) || (!argv[2]) )
        return -1;

    if ( !strcmp(argv[1], "local") ) {
        map.rule = E1_RULE_LOCAL;
        section = "LOCAL_E1USER";
    } else if ( !strcmp(argv[1], "global") ) {
        map.rule = E1_RULE_GLOBAL;
        section = "GLOBAL_E1USER";
        
    } else {
        printf("Unkown rule %s.", argv[1]);
        return -1; 
    }

    if (!strcmp(argv[2], "dis")) {
        if (cfg_e1map_dis("e1user", &map) != 0) {
            return -1;
        }
    } else {
        if (cfg_e1map(argv[2], section, &map) != 0) {
            return -1;
        }
    }

    return 0;
}

int sdh_cfg_e164k(int argc, char **argv)
{
    struct proto_2m64kppp_map map;
    uint8_t fpgaid = 0;
    char *section = NULL;

    if (argc != 3){
        return -1;
    }

    memset(&map, 0, sizeof(struct proto_2m64kppp_map));

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    map.fpgaid = fpgaid;

    /* argv[1]: rule argv[2]: cfgfile */
    if ( (!argv[1]) || (!argv[2]) )
        return -1;

    if ( !strcmp(argv[1], "local") ) {
        map.rule = E1_RULE_LOCAL;
        section = "LOCAL_E164K";
    } else if ( !strcmp(argv[1], "global") ) {
        map.rule = E1_RULE_GLOBAL;
        section = "GLOBAL_E164K";
        
    } else {
        printf("Unkown rule %s.", argv[1]);
        return -1; 
    }

    if (!strcmp(argv[2], "dis")) {
        if (cfg_e1map_dis("e164k", &map) != 0) {
            return -1;
        }
    } else {
        if (cfg_e1map(argv[2], section, &map) != 0) {
            return -1;
        }
    }

    return 0;
}

int sdh_cfg_e1gfp(int argc, char **argv)
{
    struct proto_2mgfp_map map;
    uint8_t fpgaid = 0;
    char *section = NULL;

    if (argc != 3){
        return -1;
    }

    memset(&map, 0, sizeof(struct proto_2mgfp_map));

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    map.fpgaid = fpgaid;

    /* argv[1]: rule argv[2]: cfgfile */
    if ( (!argv[1]) || (!argv[2]) )
        return -1;

    if (!strcmp(argv[1], "local") ) {
        map.rule = E1_RULE_LOCAL;
        section = "LOCAL_E1GFP";
    } else if (!strcmp(argv[1], "global") ) {
        map.rule = E1_RULE_GLOBAL;
        section = "GLOBAL_E1GFP";
    } else {
        printf("Unkown rule %s.", argv[1]);
        return -1; 
    }

    if (!strcmp(argv[2], "dis")) {
        if (cfg_e1map_dis("e1gfp", &map) != 0) {
            return -1;
        }
    } else {
        if (cfg_e1map(argv[2], section, &map) != 0) {
            return -1;
        }
    }

    return 0;
}


int sdh_get_gfp_info(int argc, char **argv)
{
    uint8_t fpgaid = 0;
    uint8_t port = 0;
    uint8_t i = 0;
    uint8_t j = 0;
    uint8_t k = 0;
    struct gfp_info ginfo;

    if (argc != 1) {
        return -1;
    }

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    printf("fpgaid type port group speed chans chan\n");

    for (k = 0; k < 16; k++) {
        memset(&ginfo, 0, sizeof(struct gfp_info));
        port = (k+1);
        if (sdhlib_get_gfp_info(port, fpgaid, &ginfo) != 0) {
            continue;
        }

        if (!ginfo.groups_num) {
            continue;
        }

        for (i = 0; i < ginfo.groups_num; i++) {
            printf("%2d     %2d   %2d   %2d    %2d   %2d    ", \
                   fpgaid, ginfo.type, port, (i+1), ginfo.groups[i].speed, ginfo.groups[i].channels_num);
            printf("[");
            for (j = 0; j < ginfo.groups[i].channels_num; j++) {
                printf("%d ", ginfo.groups[i].channels[j]);
            }
            printf("]\n");
        }
    }

    return 0;
}

int sdh_get_gfp_result(int argc, char **argv)
{
    struct gfp_result  *result = NULL;
    int i, j;
    uint8_t fpgaid = 0;

    if (argc != 1) {
        return -1;
    }

    fpgaid = conv_fpgaid(&argv[0]);
    if (fpgaid == 0xff)
        return -1;

    result = (struct gfp_result *)malloc(sizeof(struct gfp_result));
    if (result == NULL) {
        printf("malloc failed\n"); 
        return -1;
    }

    memset(result, 0, sizeof(struct gfp_result));

    if (sdhlib_get_gfp_result(fpgaid, result) != 0) {
        free(result);
        return -1;
    }

    printf("fpgaid groupid channel\n");
    for (i = 0; i < result->groups_num; i++) {
        printf("%2d     %2d      %2d-%2d-%2d-%2d-%2d \n", fpgaid, (i+1), 
              result->groups[i].channels[0].chassisid, result->groups[i].channels[0].slot, \
              result->groups[i].channels[0].port, result->groups[i].channels[0].au4, \
              result->groups[i].channels[0].channel);

        for (j = 1; j < result->groups[i].channels_num; j++) {
            printf("               %2d-%2d-%2d-%2d-%2d \n", \
                   result->groups[i].channels[j].chassisid, result->groups[i].channels[j].slot, \
                   result->groups[i].channels[j].port, result->groups[i].channels[j].au4, \
                   result->groups[i].channels[j].channel);
        }
    }

    free(result);
    return 0;
}

int sdh_get_E1_info()
{
    unsigned char i = 0;
    struct e1_info e1_info;

    printf("fpgaid e1_cnt m64k_cnt m2m_cnt m2m_nfm_cnt\n");

    for (i = 0; i < BOARD_FPGA_MAXNUM; i++) {        
        memset(&e1_info, 0, sizeof(struct e1_info));
        if (sdhlib_get_E1_info(i + 1, &e1_info) != 0) {
            return -1;
        }
        printf("%6d %6d %8d %7d %11d\n", \
            i + 1, e1_info.e1_cnt, e1_info.m64k_cnt, e1_info.m2m_cnt, e1_info.m2m_nfm_cnt);
    }
    return 0;
}

int sdh_get_port_status(char *argv)
{
    int port = 0;
    struct port_status status;
    char *rate_string[] = {"OTH", "40G","10G","2.5G", "1M", "622M", "155M"};
    char *fiber_string[] = {"Unkown", "single", "double"};

    port = atoi(argv);
    if ((port < 1) || (port > 50)) {
        return -1;
    }

    memset(&status, 0, sizeof(struct port_status));
    if (sdhlib_get_port_status(port, &status) != 0) {
        return -1;
    }

    printf("port type link enable rate  fiber  rx_power tx_power b1_code b2_code b3_code mtu \n");

    printf("%2d   %3s  %-2d  %3s    %5s %6s %8d %8d %6d  %6d  %6d  %5d \n", status.id, (status.type == PORT_TYPE_SDH) ? "sdh" : "lan", \
           status.link_status, status.work_enable == PORT_WORK_ENABLE? "en":"dis", rate_string[status.rate], \
          fiber_string[status.fiber_type], status.rx_power, status.tx_power, \
          status.b1_code, status.b2_code, status.b3_code, status.mtu);

    return 0;
}


int sdh_get_port_stat(char *argv)
{
	unsigned char port = 0;
	struct port_stat pstat;
	char buf_rate[32];

	port = atoi(argv);
    if ((port < 1) || (port > 50)) {
        return -1;
    }

	memset(&pstat, 0, sizeof(struct port_stat));
	if (sdhlib_get_port_stat(port, &pstat) != 0) {
		return -1;
	}

    printf("Port %d \n", pstat.id);
	printf("tx_pkts : %llu ", pstat.tx_pkts);
	printf("tx_drops: %llu ", pstat.tx_drops);
	printf("tx_errors : %llu ", pstat.tx_errors);
    printf("\n");
	convert_format_uint64_decimal(buf_rate, pstat.tx_pps);
	printf("tx_pps : %16s/s ", buf_rate);
	convert_format_uint64_decimal(buf_rate, pstat.tx_bps);
	printf("tx_bps : %16s/s ", buf_rate);
    printf("\n");

	printf("rx_pkts : %llu ", pstat.rx_pkts);
	printf("rx_drops: %llu ", pstat.rx_drops);
	printf("rx_errors : %llu ", pstat.rx_errors);
    printf("\n");
	convert_format_uint64_decimal(buf_rate, pstat.rx_pps);
	printf("rx_pps : %16s/s ", buf_rate);
	convert_format_uint64_decimal(buf_rate, pstat.rx_bps);
	printf("rx_bps : %16s/s ", buf_rate);
    printf("\n");

	return 0;
}

int sdh_cfg_info(int argc, char **argv)
{
    unsigned char id = 0;
    unsigned char physlot = 0;
    struct board_cfg_info bd_info;

    if (argc != 4) {
        return -1;
    }

    if (strcmp(argv[0], "chassisid")) {
        return -1;
    }
    id = atoi(argv[1]);
    if ((id < 1) || (id > 15)) {
        return -1; 
    }

    if (strcmp(argv[2], "physlot")) {
        return -1;
    }

    physlot = atoi(argv[3]);
    if ((physlot < 1) || (physlot > 15)) {
        return -1;
    }
    memset(&bd_info, 0, sizeof(&bd_info));
    bd_info.chassisid = id;
    bd_info.physlot = physlot;
    printf("chassisid %d physlot %d \n", id, physlot);
    if (sdhlib_set_borad_info(&bd_info) != 0) {
        printf("Failed sdhlib_set_board_info \n");
        return -1;
    }

    return 0;
}

int sdh_cfg_mtu(int port, int mtu)
{
    if (mtu == 0) {
        printf("mtu is 0 error.\n");
        return -1;
    }

    if (sdhlib_set_port_mtu(port, mtu) != 0) {
        printf("sdlib_set_port_mtu failed \n");
        return -1;
    }

    return 0;
}

int sdh_cfg_fiber(int port, char *fiber)
{
    unsigned char fiber_type = 0;

    printf("fiber is %s \n", fiber);

    if (!strcmp(fiber, "single")) {
        fiber_type = PORT_FIBERTYPE_SINGLE;
    } else if (!strcmp(fiber, "double")) {
        fiber_type = PORT_FIBERTYPE_DOUBLE;
    } else {
        printf("unkown fiber type %s \n", fiber);
        return -1;
    }

    if (sdhlib_set_port_fiber(port, fiber_type) != 0) {
        printf("sdlib_set_port_fiber failed \n");
        return -1;
    }

    return 0;
}

int sdh_cfg_stat_clear(int port)
{
    if (port < 1) {
        printf("port is %d error.\n", port); 
        return -1;
    }

    if (sdhlib_set_port_clear(port) != 0) {
        printf("sdlib_set_port_stat_clear failed \n");
        return -1;
    }

    return 0;
}

#if 0
int sdh_cfg_port_discard(int port, char *discard)
{
    unsigned char port_discard = 0;

    printf("port discard is %s \n", discard);

    if (!strcmp(discard, "none")) {
        port_discard = BCM_PORT_DISCARD_NONE;
    } else if (!strcmp(discard, "all")) {
        port_discard = BCM_PORT_DISCARD_ALL;
    } else if (!strcmp(discard, "tag")) {
        port_discard = BCM_PORT_DISCARD_TAG;
    } else if (!strcmp(discard, "untag")) {
        port_discard = BCM_PORT_DISCARD_UNTAG;
    } else {
        printf("Unkown discard %s \n", discard);
        return -1;
    }

    if (sdhlib_set_port_discard(port, port_discard) != 0) {
        printf("sdlib_set_port_discard failed \n");
        return -1;
    }

    return 0;
}
#endif

int sdh_cfg_work_enable(int port, char *enable)
{
    unsigned char work_enable = 0;

    printf("enable is %s \n", enable);

    if (!strcmp(enable, "enable")) {
        work_enable = PORT_WORK_ENABLE;
    } else if (!strcmp(enable, "disable")) {
        work_enable = PORT_WORK_DISABLE;
    } else {
        printf("unkown work enable %s \n", enable);
        return -1;
    }

    if (sdhlib_set_port_enable(port, work_enable) != 0) {
        printf("sdhlib_set_port_enable failed \n");
        return -1;
    }

    return 0;
}

int sdh_parse_args(int argc, char **argv)
{
    uint8_t fpgaid = 0;
    void *fd = NULL;


    if (argc <= 2)
        return -1;

    if (!strcmp(argv[1], "show")) {
        if ( (!strcmp(argv[2], "hp-pinfo")) || (!strcmp(argv[2], "hp-slink")) ||
             (!strcmp(argv[2], "lp-pinfo")) || (!strcmp(argv[2], "lp-slink")) || 
            (!strcmp(argv[2], "lp-chan"))) {
                fpgaid = conv_fpgaid(&argv[3]);
                if (fpgaid == 0xff)
                return -1;

                fd = sgfp_open(fpgaid); 
                if (!fd)
                return -1;

                if (!strcmp(argv[2], "hp-pinfo")) { 
                    if (sdh_show_hp_pinfo(fd, &argv[4]) != 0) {
                        sgfplib_close(fd); 
                        return -1;
                    }
                } else if (!strcmp(argv[2], "hp-slink")) {
                    if (sdh_show_hp_slink(fd, &argv[4]) != 0) {
                        sgfplib_close(fd); 
                        return -1;
                    }
                } else if (!strcmp(argv[2], "lp-pinfo")) {
                    if (sdh_show_lp_pinfo(fd, &argv[4], fpgaid) != 0) {
                        sgfplib_close(fd); 
                        return -1;
                    }
                } else if (!strcmp(argv[2], "lp-slink")) {
                    if (sdh_show_lp_slink(fd, &argv[4]) != 0) {
                        sgfplib_close(fd); 
                        return -1;
                    }
                } else if (!strcmp(argv[2], "lp-chan")) {
                    if (sdh_show_lp_chan(fd, &argv[4]) != 0) {
                        return -1;
                    }
                } else {
                    return -1;
                }

        } else if (!strcmp(argv[2], "stat")) {
            if (sdh_get_board_stat((argc - 3), &argv[3]) != 0) {
                return -1; 
            }
        } else if (!strcmp(argv[2], "info")) {
            if (sdh_get_board_info() != 0) {
                return -1;
            }
        } else if (!strcmp(argv[2], "lp-soh")) {
            if (sdh_get_soh_lpinfo((argc - 3), &argv[3]) != 0) {
                return -1;
            }
        } else if (!strcmp(argv[2], "hp-soh")) {
            if (sdh_get_soh_hpinfo((argc - 3), &argv[3]) != 0) {
                return -1;
            }
        } else if (!strcmp(argv[2], "payload")) {
            if (sdh_get_payload((argc - 3), &argv[3]) != 0) {
                return -1;
            }
        } else if (!strcmp(argv[2], "gfp-info")) {
            if (sdh_get_gfp_info((argc - 3), &argv[3]) != 0) {
                return -1; 
            }
        } else if (!strcmp(argv[2], "gfp-result")) {
            if (sdh_get_gfp_result((argc - 3), &argv[3]) != 0) {
                return -1; 
            }
        } else if (!strcmp(argv[2], "E1-info")) {
            if (sdh_get_E1_info() != 0) {
                return -1; 
            }
        } else if (!strcmp(argv[2], "port-status")) {
            if (sdh_get_port_status(argv[3]) != 0) {
                return -1; 
            }
        } else if (!strcmp(argv[2], "port-stat")) {
            if (sdh_get_port_stat(argv[3]) != 0) {
                return -1; 
            }
        } else {
            return -1; 
        }
    } else if (!strcmp(argv[1], "cfg")) {
        if (!strcmp(argv[2], "inner") && ((argc == 7) || (argc == 6))) {
           if (sdh_cfg_inner_rule(&argv[3]) != 0) {
               printf("sdh_cfg_inner_rule err. \n");
               return -1;
           }

        } else if (!strcmp(argv[2], "sel")) {
           if (sdh_cfg_sel(&argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "local")) {
           if (sdh_cfg_local(&argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "global")) {
           if (sdh_cfg_global(&argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "e1linkmap")) {
           if (sdh_cfg_e1linkmap((argc - 3), &argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "e1user")) {
           if (sdh_cfg_e1user((argc - 3), &argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "e164k")) {
           if (sdh_cfg_e164k((argc - 3), &argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "e1gfp")) {
           if (sdh_cfg_e1gfp((argc - 3), &argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "info")) {
           if (sdh_cfg_info((argc - 3), &argv[3]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "mtu")) {
            if (!argv[3] || !argv[4])
                return -1;

            if (sdh_cfg_mtu(atoi(argv[3]), atoi(argv[4])) != 0)
                return -1;

        } else if (!strcmp(argv[2], "fiber")) {
            if (!argv[3] || !argv[4])
                return -1;

           if (sdh_cfg_fiber(atoi(argv[3]), argv[4]) != 0)
               return -1;

        } else if (!strcmp(argv[2], "clear")) {
           if (sdh_cfg_stat_clear(atoi(argv[3])) != 0)
               return -1;

        } else if (!strcmp(argv[2], "work")) {
           if (sdh_cfg_work_enable(atoi(argv[3]), argv[4]) != 0)
               return -1;

        } else {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (sdh_env_init() != 0) {
        printf("sdh env init failed.\n"); 
        return -1;
    }
    if (sdh_parse_args(argc, argv) != 0) {
		printf("sdh parse args failed.\n");
        sdh_usage();
        sdh_env_exit();
        return -1;
    }
    if (sdh_env_exit() != 0) {
        printf("sdh env exit failed.\n"); 
        return -1;
    }
    return 0;
}
