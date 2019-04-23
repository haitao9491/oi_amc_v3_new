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

#define PRINT(hd, format, ...)        fprintf(oiclib_get_hd_std_out(hd), format, ##__VA_ARGS__)
#define PRINTINFO(hd, format, ...)    fprintf(oiclib_get_hd_std_out(hd), "FUN:%s LINE:%d[INFO]"format"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define PRINTWARN(hd, format, ...)    fprintf(oiclib_get_hd_std_out(hd), "FUN:%s LINE:%d[WARN]"format"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#ifdef OICLIB_DBG
#define PRINTDBG(hd, format, ...)     fprintf(oiclib_get_hd_std_out(hd), "FUN:%s LINE:%d[DBG]"format"", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PRINTDBG(hd, format, ...)     do{}while(0)
#endif
#define PRINTERR(hd,  format, ...)    fprintf(oiclib_get_hd_std_out(hd), "FUN:%s LINE:%d[ERR]"format"", __FUNCTION__, __LINE__, ##__VA_ARGS__)

void *oiclib_open();
struct fpga_verinfo     *oiclib_get_fpga_version(void *hd);
int oiclib_set_fpga_online(void *hd, int value);
int oiclib_get_fpga_online(void *hd, int *value);
int oiclib_get_fpga_ddr_status(void *hd, int *value);
int oiclib_set_fpga_bd_startup(void *hd, int value);
int oiclib_get_fpga_bd_startup(void *hd, int *value);
int oiclib_set_fpga_bd_cfginfo_orig(void *hd, char *dmac, char *smac,
		char *etype, char *slot, char *subslot);
int oiclib_get_fpga_bd_cfginfo_orig(void *hd, char *dmac, char *smac,
		char *etype, char *slot, char *subslot);
int oiclib_set_fpga_bd_cfginfo_ex(void *hd, char *dmac, char *smac, 
		char *dip, char *sip, char *dport, char *sport, char *devid, char *cardid);
int oiclib_get_fpga_bd_cfginfo_ex(void *hd, char *dmac, char *smac, 
		char *dip, char *sip, char *dport, char *sport, char *devid, char *cardid);
int oiclib_get_fpga_bd_runinfo(void *hd, struct fpga_board_runinfo *rinfo);
int oiclib_set_fpga_64k_ch_tran_start(void *hd, unsigned char link, unsigned char ts);
int oiclib_set_fpga_64k_ch_tran_stop(void *hd, unsigned char link, unsigned char ts);
int oiclib_set_fpga_2m_ch_tran_start(void *hd, unsigned char channel);
int oiclib_set_fpga_2m_ch_tran_stop(void *hd, unsigned char channel);
int oiclib_set_fpga_synctime(void *hd, unsigned int sec, unsigned int usec);
int oiclib_set_fpga_2m_ch_valid_start(void *hd, unsigned char link);
int oiclib_set_fpga_2m_ch_valid_stop(void *hd, unsigned char link);
int oiclib_get_fpga_2m_ch_valid(void *hd, unsigned char link, int *valid);
int oiclib_set_fpga_pl_cfgch_start(void *hd, unsigned int index, unsigned char ts,unsigned char ctl);
int oiclib_set_fpga_pl_cfgch_stop(void *hd, unsigned int index, unsigned char ts,unsigned char ctl);
int oiclib_set_fpga_pl_reset(void *hd);
int oiclib_get_fpga_pl_stat(void *hd);
int oiclib_set_fpga_pl_slot(void *hd,unsigned char *Psl);
int oiclib_set_fpga_pl_mode(void *hd,unsigned char mode);
int oiclib_set_fpga_pl_age_channel(void *hd,unsigned char flag);
int oiclib_get_fpga_in_stat(void *hd, struct fpga_in_stat *stat);
int oiclib_get_fpga_is_sigch(void *hd, struct fpga_is_sigch *sig);
FILE *oiclib_get_hd_std_out(void *hd);
void oiclib_set_hd_std_out(void *hd, FILE *fd);
int oiclib_set_fpga_pl_tbl_info(void *hd, unsigned char slot, unsigned char subslot);
int oiclib_set_fpga_silence_timeout(void *hd, unsigned char tm);
int oiclib_get_fpga_silence_timeout(void *hd, unsigned char *tm);
int oiclib_set_fpga_silence_range(void *hd, unsigned int range);
int oiclib_get_fpga_silence_range(void *hd, unsigned int *range);
int oiclib_get_fpga_silence_result(void *hd, unsigned char link, unsigned char timeslot, unsigned char *stat);
int oiclib_close(void *hd);
#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_OICLIB_7CC3465F_4503494E_610F3394_H */
