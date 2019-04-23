/*
 * (C) Copyright 2015
 * <www.raycores.com>
 *
 */

#ifndef _HEAD_FPGADRV_CTL_H
#define _HEAD_FPGADRV_CTL_H


#if defined(__cplusplus)
extern "C" {
#endif

int fpgadrvctl_read_version(unsigned long arg);
int fpgadrvctl_read_ddr_init_stat(unsigned long arg);
int fpgadrvctl_read_board_start_opt(unsigned long arg);
int fpgadrvctl_write_board_start_opt(unsigned long arg);
int fpgadrvctl_read_board_cfg_info(unsigned long arg);
int fpgadrvctl_write_board_cfg_info(unsigned long arg);
int fpgadrvctl_read_voice_cfg(unsigned long arg);
int fpgadrvctl_write_voice_cfg(unsigned long arg);
int fpgadrvctl_read_pl_scan_rst(unsigned long arg);
int fpgadrvctl_write_pl_scan_rst(unsigned long arg);
int fpagdrvctl_read_cfg_result(unsigned long arg);
int fpgadrvctl_read_board_run_info(unsigned long arg);
int fpgadrvctl_read_64k_transfer(unsigned long arg);
int fpgadrvctl_write_64k_transfer(unsigned long arg);
int fpgadrvctl_read_2m_transfer(unsigned long arg);
int fpgadrvctl_write_2m_transfer(unsigned long arg);
int fpgadrvctl_read_e1phy_stat(unsigned long arg);
int fpgadrvctl_write_e1phy_stat(unsigned long arg);
int fpgadrvctl_start_up_device(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FPGADRV_CTL_H */
