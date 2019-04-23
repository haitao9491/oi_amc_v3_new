/*
 * (C) Copyright 2017
 * liye <ye.li@raycores.com>
 *
 * gfp_fpga.h - A description goes here.
 *
 */

#ifndef _HEAD_GFP_FPGA_2447371E_2B424591_3F013253_H
#define _HEAD_GFP_FPGA_2447371E_2B424591_3F013253_H

typedef struct {
	unsigned char	vrr;		/* version register		*/
	unsigned char	dar[4];		/* date register		*/
} version_t;

typedef struct {
	unsigned char	dsr;		/* ddr status register		*/
	unsigned char	bcr;		/* board control register	*/
    unsigned char   dstop;      /* ddr stop reset           */
} gfp_bdstartup_t;

typedef struct {
    unsigned char   select_port;
    unsigned char   select_chan;
    unsigned char   clear_bcode;
    unsigned char   refresh_en;
    unsigned char   select_info;
    unsigned char   get_val_3;
    unsigned char   get_val_2;
    unsigned char   get_val_1;
    unsigned char   get_val_0;
} get_info_t;

typedef struct {
	unsigned char ge;
} get_ge_stat_t;

typedef struct {
    unsigned char   mif_cfg_op;
    unsigned char   mif_cfg_mix;
    unsigned char   mif_cfg_ram_addr;
#if defined(IN_X86)
    unsigned char   mif_cfg_ram_data_4;
#endif
    unsigned char   mif_cfg_ram_data_3;
    unsigned char   mif_cfg_ram_data_2;
    unsigned char   mif_cfg_ram_data_1;
    unsigned char   mif_cfg_ram_data_0;
#if defined(IN_X86)
    unsigned char   mif_cfg_ram_addr1;
#else
    unsigned char   mif_cfg_ram_group;
#endif
} set_info_t;

typedef struct {
    unsigned char   scan_status;
    unsigned char   group_id;
    unsigned char   scan_res;
#if defined(IN_X86)
    unsigned char   group_id1;
#endif
} scan_t ;

typedef struct {
	unsigned char task_sel;
	unsigned char cfg_op;
	unsigned char cfg_data_3;
	unsigned char cfg_data_2;
	unsigned char cfg_data_1;
	unsigned char cfg_data_0;
} set_svc_t;

typedef struct {
	unsigned char mis_cnt_clr;
} set_clr_t;

typedef struct {
	unsigned char mif_cfg_op;
	unsigned char mif_cfg_addr_h;
	unsigned char mif_cfg_addr_l;
	unsigned char mif_cfg_data_5;
	unsigned char mif_cfg_data_4;
	unsigned char mif_cfg_data_3;
	unsigned char mif_cfg_data_2;
	unsigned char mif_cfg_data_1;
	unsigned char mif_cfg_data_0;
} hdlc_scan_t;

typedef struct
{
    unsigned char mif_list_cfg_en;      
    unsigned char mif_list_cfg_ch;      
    unsigned char mif_list_cfg_info_h;  
    unsigned char mif_list_cfg_info_l;  
}rule_t;

typedef struct
{
        unsigned char   reg_sel;
        unsigned char   reg_val[4];
        unsigned char   reg_cfg_en;
}chan_forward_t;

typedef struct {
	version_t		verinfo;
	unsigned char	reserved0;
    gfp_bdstartup_t startup;
	unsigned char	reserved1[0x1e - 0x08];
        unsigned char   selnum;
	unsigned char	reserved2[0x40 - 0x1f];
        rule_t          rule;
        unsigned char   forward_en;
	unsigned char	reserved3[0x5f - 0x45];
    get_info_t      get_info;
	get_ge_stat_t   get_gstat;
	unsigned char	reserved4[0x6f - 0x69];
    set_info_t      set_info;
#if defined(IN_X86)
	unsigned char	reserved5[0x7f - 0x78];
#else
	unsigned char	reserved5[0x7f - 0x77];
#endif
    scan_t          scan;
#if defined(IN_X86)
	unsigned char   reserved6[0x8f - 0x83];
#else
	unsigned char   reserved6[0x8f - 0x82];
#endif
	set_svc_t		set_svc;
	unsigned char   reserved7[0xaf - 0x95];
	hdlc_scan_t		hdlc_scan;
	unsigned char   reserved8[0xff - 0xb8];
	set_clr_t       set_clr;
        unsigned char   reserved9[0x10f - 0x100];
        chan_forward_t  chan;
} gfp_fpga_reg;

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_GFP_FPGA_2447371E_2B424591_3F013253_H */
