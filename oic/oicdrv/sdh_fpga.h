/*
 * (C) Copyright 2018
 * liye <ye.li@raycores.com>
 *
 * sdh_fpga.h - A description goes here.
 *
 */

#ifndef _HEAD_SDH_FPGA_4E9756DE_7F9651C7_136268F8_H
#define _HEAD_SDH_FPGA_4E9756DE_7F9651C7_136268F8_H


typedef struct {
	unsigned char	vrr;		/* version register		*/
	unsigned char	dar[3];		/* date register		*/
	unsigned char	sub_vrr;	/* sub version register	*/
	unsigned char	reserved0;
} ver_t;

typedef struct {
	uint8_t	dsr;		/* ddr status register		*/
	uint8_t	bcr;		/* board control register	*/
    uint8_t   dstop;      /* ddr stop reset           */
} sdh_bdstartup_t;

typedef struct {
    uint8_t  inner_hw_no_h;
    uint8_t  inner_hw_no_l;
    uint8_t  user_hw_no_h;
    uint8_t  user_hw_no_l;
} devno_t;

typedef struct {
    uint8_t   list_cfg_ext_reg;
    uint8_t   list_cfg_en;
    uint8_t   list_cfg_ch;
    uint8_t   list_cfg_info_h;
    uint8_t   list_cfg_info_l;
    uint8_t   list_cfg_type;
} set_list_t;

typedef struct {
    uint8_t  list_cap_ext_reg;
    uint8_t  mif_list_indx;
    uint8_t  mif_list_sel;
    uint8_t  mif_list_cap_en;
    uint8_t  mif_list_cap_va_3;
    uint8_t  mif_list_cap_va_2;
    uint8_t  mif_list_cap_va_1;
    uint8_t  mif_list_cap_va_0;
} get_list_t;

typedef struct {
    uint8_t  mis_cnt_clr;
    uint8_t  mis_cnt_sel;
    uint8_t  mis_cnt_val_3;
    uint8_t  mis_cnt_val_2;
    uint8_t  mis_cnt_val_1;
    uint8_t  mis_cnt_val_0;
} mis_cnt_t;

typedef struct {
    uint8_t  sw_ch_sel;
    uint8_t  sw_e1_sel;
    uint8_t  sw_ch_en;
    uint8_t  sw_cfg_en;
} ch_en_t;

typedef struct {
	uint8_t	 fpgaid;
	uint8_t	 chassisid_slot;
	uint8_t	 user_fpgaid;
	uint8_t	 user_chassisid_slot;
} sdh_devinfo_t;

typedef struct {
    uint8_t	 cfg_refer;
    uint8_t	 read0_2m;
    uint8_t	 read1_2m;
    uint8_t	 read2_2m;
    uint8_t	 read3_2m;
} refer_E1;

typedef struct {
	ver_t				verinfo;
    sdh_bdstartup_t		startup;
	sdh_devinfo_t		devinfo;
	uint8_t				reserved1[0x1e - 0x0c];
    uint8_t				selnum;
	uint8_t				reserved2[0x3f - 0x1f];
    set_list_t			set_list;
	uint8_t				reserved3[0x4f - 0x45];
    get_list_t			get_list;
	uint8_t				reserved4[0x63 - 0x57];
    refer_E1            E1info;
	uint8_t				reserved5[0xff - 0x68];
    mis_cnt_t			mis_cnt;
	uint8_t				reserved6[0x10f - 0x105];
    ch_en_t				ch_en;
} sdh_fpga_reg;


typedef struct {
	uint8_t				reserved0[0x1ff];
	uint8_t				cpld_status;
} sdh_cpld_reg;

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SDH_FPGA_4E9756DE_7F9651C7_136268F8_H */
