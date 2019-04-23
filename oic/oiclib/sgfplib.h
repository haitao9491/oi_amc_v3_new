/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oiclib.h - A description goes here.
 *
 */

#ifndef _HEAD_OICLIB_7CC3465F_4503494E_610F3394_H
#define _HEAD_OICLIB_7CC3465F_4503494E_610F3394_H

#include "oic.h"

#if defined(__cplusplus)
extern "C" {
#endif

void *sgfplib_open(int fpgadev);
int sgfplib_get_fpga_bd_runinfo_ex(void *hd, struct fpga_board_runinfo_ex *rinfo);
int sgfplib_set_linkinfo_start(void *hd);
int sgfplib_get_linkinfo(void *hd, struct slink_info *link);
int sgfplib_get_hp_linkinfo(void *hd, struct fpga_board_runinfo_port_ex  *link);
int sgfplib_set_linkinfo_end(void *hd);
int sgfplib_set_try_group(void *hd, struct sgroup_all_info *gsinfo);
int sgfplib_set_group(void *hd, struct sgroup_all_info *gsinfo);
int sgfplib_get_try_group(void *hd, struct sgroup_all_info *gsinfo);
int sgfplib_get_group(void *hd, struct sgroup_all_info *gsinfo);
#define GET_CHSTAT_MASK			0x01
#define GET_CHSTAT_LOCAL_MASK	0x02
#define GET_CHSTAT_CFG_MASK		0x04
#define GET_CHSTAT_DB_MASK      0x08
int sgfplib_get_hdlc_chstat(void *hd, int mask, struct hdlc_port_stat *chstat);
int sgfplib_set_hdlc_cfg(void *hd, struct hdlc_cfg *hcfg);
int sgfplib_get_hdlc_ge_stat(void *hd, struct bd_ge_stat *gstat);
int sgfplib_set_spu_selnum(void *hd, unsigned char selnum);
int sgfplib_get_spu_selnum(void *hd, unsigned char *value);
int sgfplib_set_spu_forward_enable(void *hd);
int sgfplib_set_spu_forward_disable(void *hd);
int sgfplib_set_spu_forward_rule(void *hd, struct spu_forward_rule rule);
int sgfplib_set_spu_channel_forward(void *hd, struct chan_flag chan);
int sdhlib_get_sdh_fpga_stat(void *hd, struct sdh_fpga_stat *stat);
int sdhlib_set_board_stat_clear(void *hd);
int sdhlib_set_board_stat_bytes(void *hd);
int sdhlib_set_board_stat_pkts(void *hd);
int sgfplib_close(void *hd);
int sdhlib_get_E1_info(unsigned char fpgaid, struct e1_info *info);
#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_OICLIB_7CC3465F_4503494E_610F3394_H */
