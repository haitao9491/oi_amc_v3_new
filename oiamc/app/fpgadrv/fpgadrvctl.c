/*
 * (C) Copyright 2015
 * <www.raycores.com>
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "fpgadrvdbg.h"
#include "fpgadrv.h"
#include "fpgadrvctl.h"


int fpgadrvctl_read_version(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga;
	struct ver_info ver;

	mutex_lock(&fpgadrv_devp->lock);

	ver.ver  = __raw_readb(addr);
//	ver.data = (addr[1] << 24) | (addr[2] << 16) | (addr[3] << 8) | addr[4];
	ver.data = (__raw_readb(addr+1) << 24) | (__raw_readb(addr+2) << 16) | 
		(__raw_readb(addr+3) << 8) | __raw_readb(addr+4);

	mutex_unlock(&fpgadrv_devp->lock);

	return copy_to_user((void __user *)arg, &ver, sizeof(ver));
}

int fpgadrvctl_read_ddr_init_stat(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0006;
	int ddrinit;

	mutex_lock(&fpgadrv_devp->lock);

	ddrinit = __raw_readb(addr) & 0x01;

	mutex_unlock(&fpgadrv_devp->lock);
	
	return put_user(ddrinit, (int __user *)arg);
}

int fpgadrvctl_read_board_start_opt(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0007;
	int start;

	mutex_lock(&fpgadrv_devp->lock);

	start = __raw_readb(addr) & 0x01;

	mutex_unlock(&fpgadrv_devp->lock);

	return put_user(start, (int __user *)arg);
}

int fpgadrvctl_write_board_start_opt(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0007;
	int start;

	if (get_user(start, (int __user *)arg) == 0) {
		mutex_lock(&fpgadrv_devp->lock);

		__raw_writeb((start & 0x01), addr);
		start = __raw_readb(addr);

		mutex_unlock(&fpgadrv_devp->lock);

		return put_user(start, (int __user *)arg);
	}

	return -1;
}

static int fpgadrv_board_cfg_wr_if(char ad, char data)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0030;
	int ret;
	
	mutex_lock(&fpgadrv_devp->lock);

	__raw_writeb(ad, addr);
	ret = __raw_readb(addr);
	if (ret != ad) {
		FPGADRVERR("board cfg wr: addr %p ad 0x%x, ret 0x%x\n", addr, ad, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(data, (addr + 1));
	ret = __raw_readb(addr + 1);
	if (ret != data) {
		FPGADRVERR("board cfg wr: addr %p, data 0x%x, ret 0x%x\n", addr + 1, data, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(0, (addr + 2));
	ret = __raw_readb(addr + 2);
	if (ret != 0) {
		FPGADRVERR("board cfg wr: addr %p write 0, ret 0x%x\n", addr + 2, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(1, (addr + 2));
	ret = __raw_readb(addr + 2);
	if (ret != 1) {
		FPGADRVERR("board cfg wr: addr %p write 1, ret 0x%x\n", addr + 2, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	mutex_unlock(&fpgadrv_devp->lock);
	return 0;
}

static int fpgadrv_board_cfg_rd_if(char ad, char *data)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0030;
	int ret;
	
	mutex_lock(&fpgadrv_devp->lock);

	__raw_writeb(ad, addr);
	ret = __raw_readb(addr);
	if (ret != ad) {
		FPGADRVERR("board cfg rd: addr %p ad 0x%x, ret 0x%x\n", addr, ad, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0, (addr + 4));
	ret = __raw_readb((addr + 4));
	if (ret != 0) {
		FPGADRVERR("board cfg rd: addr %p will 0, ret 0x%x\n", (addr + 4), ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(1, (addr + 4));
	ret = __raw_readb((addr + 4));
	if (ret != 1) {
		FPGADRVERR("board cfg rd: addr %p will 1, ret 0x%x\n", (addr + 4), ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	
	*data = __raw_readb((addr + 3));
	FPGADRVDBG("board cfg RD: addr %p +4 %p, ad 0x%02x, data: 0x%02x", addr, (addr + 4), ad, *data);
	mutex_unlock(&fpgadrv_devp->lock);

	return 0;
}

int fpgadrvctl_read_board_cfg_info(unsigned long arg)
{
	struct bd_cfg_info bdinfo;
	int rc, i = 0;
	char ad, ethtype;

	for (i = 0; i < 6; i++) {
		ad = i & 0xff;
		rc = fpgadrv_board_cfg_rd_if(ad, &(bdinfo.dstmac[i]));
		if (rc != 0) {
			return -1;
		}
	}

	for (i = 0; i < 6; i++) {
		ad = (i + 0x10) & 0xff;
		rc = fpgadrv_board_cfg_rd_if(ad, &bdinfo.srcmac[i]);
		if (rc != 0) {
			return -1;
		}
	}

	rc = fpgadrv_board_cfg_rd_if(0x20, &ethtype);
	if (rc != 0) {
		return -1;
	}
	bdinfo.ethtype = ethtype << 8;
	rc = fpgadrv_board_cfg_rd_if(0x21, &ethtype);
	if (rc != 0) {
		return -1;
	}
	bdinfo.ethtype |= ethtype;

	rc = fpgadrv_board_cfg_rd_if(0x30, &bdinfo.slot);
	if (rc != 0) {
		return -1;
	}
	bdinfo.mask = 0x0f;

	FPGADRVDBG("read bdinfo: dmac %02x:%02x:%02x:%02x:%02x:%02x", 
			bdinfo.dstmac[0], bdinfo.dstmac[1], bdinfo.dstmac[2], 
			bdinfo.dstmac[3], bdinfo.dstmac[4], bdinfo.dstmac[5]);
	FPGADRVDBG("read bdinfo: smac %02x:%02x:%02x:%02x:%02x:%02x", 
			bdinfo.srcmac[0], bdinfo.srcmac[1], bdinfo.srcmac[2], 
			bdinfo.srcmac[3], bdinfo.srcmac[4], bdinfo.srcmac[5]);
	FPGADRVDBG("read bdinfo: ethtype 0x%x, slot 0x%x, mask 0x%x", 
			bdinfo.ethtype, bdinfo.slot, bdinfo.mask);

	return copy_to_user((void __user *)arg, &bdinfo, sizeof(bdinfo));
}

int fpgadrvctl_write_board_cfg_info(unsigned long arg)
{
	struct bd_cfg_info bdinfo;
	char ad, data;
	int i = 0;

	copy_from_user(&bdinfo, (void __user *)arg, sizeof(bdinfo));

	FPGADRVDBG("write bdinfo: dmac %02x:%02x:%02x:%02x:%02x:%02x", 
			bdinfo.dstmac[0], bdinfo.dstmac[1], bdinfo.dstmac[2], 
			bdinfo.dstmac[3], bdinfo.dstmac[4], bdinfo.dstmac[5]);
	FPGADRVDBG("write bdinfo: smac %02x:%02x:%02x:%02x:%02x:%02x", 
			bdinfo.srcmac[0], bdinfo.srcmac[1], bdinfo.srcmac[2], 
			bdinfo.srcmac[3], bdinfo.srcmac[4], bdinfo.srcmac[5]);
	FPGADRVDBG("write bdinfo: ethtype 0x%x, slot 0x%x, mask 0x%x", 
			bdinfo.ethtype, bdinfo.slot, bdinfo.mask);

	if (bdinfo.mask & 0x01) { /* dstmac valid */
		for (i = 0; i < 6; i++) {
			ad = i & 0xff;
			fpgadrv_board_cfg_wr_if(ad, bdinfo.dstmac[i]);
		}
	}
	if (bdinfo.mask & 0x02) { /* srcmac valid */
		for (i = 0; i < 6; i++) {
			ad = (i + 0x10) & 0xff;
			fpgadrv_board_cfg_wr_if(ad, bdinfo.srcmac[i]);
		}
	}
	if (bdinfo.mask & 0x04) { /* ethtype valid */
		data = (bdinfo.ethtype >> 8) & 0xff;
		fpgadrv_board_cfg_wr_if(0x20, data);
		data = bdinfo.ethtype & 0xff;
		fpgadrv_board_cfg_wr_if(0x21, data);
	}
	if (bdinfo.mask & 0x08) { /* slot valid */
		fpgadrv_board_cfg_wr_if(0x30, bdinfo.slot);
	}

	return 0;
}

int fpgadrvctl_read_voice_cfg(unsigned long arg)
{
	return 0;
}

int fpgadrvctl_write_voice_cfg(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0040;
	int cfgch;  /* bit:31~16, 1: valid 0:invalid; bit:15~0 channel: bit11~0 */
	int ret;
	int value;

	copy_from_user(&cfgch, (void __user *)arg, sizeof(int));

	mutex_lock(&fpgadrv_devp->lock);

	value = (cfgch >> 8) & 0x7;
	__raw_writeb(value, addr);
	ret = __raw_readb(addr);
	if (ret != value) {
		FPGADRVERR("write_voice_cfg: write addr %p data %d, ret %d", addr, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = cfgch & 0xff;
	__raw_writeb(value, addr + 1);
	ret = __raw_readb(addr + 1);
	if (ret != value) {
		FPGADRVERR("write_voice_cfg: write addr %p data %d, ret %d", addr + 1, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = (cfgch >> 16) & 0x01;
	__raw_writeb(value, addr + 2);
	ret = __raw_readb(addr + 2);
	if (ret != value) {
		FPGADRVERR("write_voice_cfg: write addr %p data %d, ret %d", addr + 2, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x0, addr + 3);
	ret = __raw_readb(addr + 3);
	if (ret != 0) {
		FPGADRVERR("write_voice_cfg: write addr %p data 0, ret %d", addr + 3, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(0x1, addr + 3);
	ret = __raw_readb(addr + 3);
	if (ret != 1) {
		FPGADRVERR("write_voice_cfg: write addr %p data 1, ret %d", addr + 3, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	mutex_unlock(&fpgadrv_devp->lock);
	return 0;
}

int fpgadrvctl_read_pl_scan_rst(unsigned long arg)
{

	return 0;
}

int fpgadrvctl_write_pl_scan_rst(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0044;
	int ch;
	int ret;
	int value;

	copy_from_user(&ch, (void __user *)arg, sizeof(int));

	mutex_lock(&fpgadrv_devp->lock);
	value = (ch >> 8) & 0x7;
	__raw_writeb(value, addr + 1);
	ret = __raw_readb(addr + 1);
	if (ret != value) {
		FPGADRVERR("write_pl_scan_rst: write addr %p data %d, ret %d", addr + 1, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = ch & 0xff;
	__raw_writeb(value, addr + 2);
	ret = __raw_readb(addr + 2);
	if (ret != value) {
		FPGADRVERR("write_pl_scan_rst: write addr %p data %d, ret %d", addr + 2, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	ret = __raw_readb(addr);
	if (ret == 0) {
		FPGADRVERR("channel %d is not permit reset", ch);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x0, addr + 3);
	ret = __raw_readb(addr + 3);
	if (ret != 0) {
		FPGADRVERR("write_pl_scan_rst: write addr %p data 0, ret %d", addr + 3, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(0x1, addr + 3);
	ret = __raw_readb(addr + 3);
	if (ret != 1) {
		FPGADRVERR("write_voice_cfg: write addr %p data 1, ret %d", addr + 3, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	mutex_unlock(&fpgadrv_devp->lock);
	return 0;
}

int fpagdrvctl_read_cfg_result(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0048;
	struct scan_chan_ret scaninfo;
	int ret, value;

	ret = copy_from_user(&scaninfo, (void __user *)arg, sizeof(scaninfo));
	if (ret != 0) {
		FPGADRVERR("read_cfg_result: copy_from_user failed\n");
		return -1;
	}

	mutex_lock(&fpgadrv_devp->lock);
	value = (scaninfo.chan >> 8) & 0x7;
	__raw_writeb(value, addr);
	ret = __raw_readb(addr);
	if (ret != value) {
		FPGADRVERR("read_cfg_result: write addr %p data %d, ret %d", addr, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	
	value = scaninfo.chan & 0xff;
	__raw_writeb(value, addr + 1);
	ret = __raw_readb(addr + 1);
	if (ret != value) {
		FPGADRVERR("read_cfg_result: write addr %p data %d, ret %d", addr + 1, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x0, addr + 2);
	ret = __raw_readb(addr + 2);
	if (ret != 0) {
		FPGADRVERR("read_cfg_result: write addr %p data 0, ret %d", addr + 2, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}
	__raw_writeb(0x1, addr + 2);
	ret = __raw_readb(addr + 2);
	if (ret != 1) {
		FPGADRVERR("read_cfg_result: write addr %p data 1, ret %d", addr + 2, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	ret = __raw_readb(addr + 3);
	if (ret == 0) { /* invalidate */
		FPGADRVERR("channel %d scan result invalidate", scaninfo.chan);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	ret = __raw_readb(addr + 4);
	scaninfo.scan_stat = ((ret & 0x08) >> 3) | ((ret & 0x04) >> 1);
	scaninfo.scan_cnt  = (ret & 0x3);
	scaninfo.scan_ret1 = __raw_readb(addr + 5);
	scaninfo.scan_ret2 = __raw_readb(addr + 6);

	mutex_unlock(&fpgadrv_devp->lock);

	return copy_to_user((void __user *)arg, &scaninfo, sizeof(scaninfo));
}

#define FPGADRV_SIG_TYPE_64K      0
#define FPGADRV_SIG_TYPE_2M       1

int fpgadrvctl_read_board_run_info(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0050;
	struct board_run_info bdinfo;
	int i, ret; 

	ret = copy_from_user(&bdinfo, (void __user *)arg, sizeof(bdinfo));
	if (ret != 0) {
		FPGADRVERR("read_board_run_info: copy_from_user failed");
		return -1;
	}

	mutex_lock(&fpgadrv_devp->lock);

	for (i = 0; i < FPGADRV_OPTIC_PORT_CNT; i++) {
		__raw_writeb((i << 4) & 0xff, addr);
		ret = __raw_readb(addr);
		if (ret != ((i << 4) & 0xff)) {
			FPGADRVERR("read_board_run_info: write addr %p data %d, ret %d", addr, i, ret);
			mutex_unlock(&fpgadrv_devp->lock);
			return -1;
		}
		ret = __raw_readb(addr + 1);
		bdinfo.bpi[i].optic_loss  = (ret >> 7) & 0x01;
		bdinfo.bpi[i].stm1_ok     = (ret >> 6) & 0x01;
		bdinfo.bpi[i].e1_sync_cnt = ret & 0x3f;
		__raw_writeb(FPGADRV_SIG_TYPE_64K, addr + 2);
		bdinfo.bpi[i].ch_cnt_64k  = (__raw_readb(addr + 3) << 8) | (__raw_readb(addr + 4));
		bdinfo.bpi[i].frame_cnt_64k  = (__raw_readb(addr + 5) << 24) | 
			(__raw_readb(addr + 6) << 16) | (__raw_readb(addr + 7) << 8) | __raw_readb(addr + 8);

		__raw_writeb(FPGADRV_SIG_TYPE_2M, addr + 2);
		bdinfo.bpi[i].ch_cnt_2m  = (__raw_readb(addr + 3) << 8) | (__raw_readb(addr + 4));
		bdinfo.bpi[i].frame_cnt_2m  = (__raw_readb(addr + 5) << 24) | 
			(__raw_readb(addr + 6) << 16) | (__raw_readb(addr + 7) << 8) | __raw_readb(addr + 8);
	}

	bdinfo.ethbdw = (__raw_readb(addr + 9) << 24) | 
		(__raw_readb(addr + 10) << 16) | (__raw_readb(addr + 11) << 8) | __raw_readb(addr + 12);

	mutex_unlock(&fpgadrv_devp->lock);

	return copy_to_user((void __user *)arg, &bdinfo, sizeof(bdinfo));
}

int fpgadrvctl_read_64k_transfer(unsigned long arg)
{

	return 0;
}

int fpgadrvctl_write_64k_transfer(unsigned long arg)
{	
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0060;
	int ch; /* bit31~16: 1: valid 0: invalid; bit15~bit0: channel: bit12~bit0 */
	int value, ret;

	copy_from_user(&ch, (void __user *)arg, sizeof(int));

	mutex_lock(&fpgadrv_devp->lock);
	value = (ch >> 8) & 0x1f;
	__raw_writeb(value, addr);
	ret = __raw_readb(addr);
	if (ret != value) {
		FPGADRVERR("write_64k_transfer: write addr %p data %d, ret %d", addr, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = ch & 0xff;
	__raw_writeb(value, addr + 1);
	ret = __raw_readb(addr + 1);
	if (ret != value) {
		FPGADRVERR("write_64k_transfer: write addr %p data %d, ret %d", addr + 1, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = (ch >> 16) & 0xff;
	__raw_writeb(value, addr + 2);
	ret = __raw_readb(addr + 2);
	if (ret != value) {
		FPGADRVERR("write_64k_transfer: write addr %p data %d, ret %d", addr + 2, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x0, addr + 3);
	ret = __raw_readb(addr + 3);
	if (ret != 0) {
		FPGADRVERR("write_64k_transfer: write addr %p data 0, ret %d", addr + 3, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x1, addr + 3);
	ret = __raw_readb(addr + 3);
	if (ret != 1) {
		FPGADRVERR("write_64k_transfer: write addr %p data 1 ret %d", addr + 3, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	mutex_unlock(&fpgadrv_devp->lock);
//	FPGADRVINFO("function: %s", __func__);

	return 0;
}

int fpgadrvctl_read_2m_transfer(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0064;
	int ch; /* bit31~16: 1: valid 0: invalid; bit15~bit0: channel: bit7~bit0 */
	int value, ret;

	copy_from_user(&ch, (void __user *)arg, sizeof(int));

	mutex_lock(&fpgadrv_devp->lock);

	__raw_writeb(1, addr);   /* read opt */
	ret = __raw_readb(addr);
	if (ret != 1) {
		FPGADRVERR("write_2m_transfer: write addr %p data 1, ret %d", addr, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = ch & 0xff;
	__raw_writeb(value, addr + 1);   /* write channel */
	ret = __raw_readb(addr + 1);
	if (ret != value) {
		FPGADRVERR("write_2m_transfer: write addr %p data %d, ret %d", addr + 1, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x0, addr + 4);     /* write en */
	ret = __raw_readb(addr + 4);
	if (ret != 0) {
		FPGADRVERR("write_2m_transfer: write addr %p data 0, ret %d", addr + 4, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x1, addr + 4);
	ret = __raw_readb(addr + 4);
	if (ret != 1) {
		FPGADRVERR("write_2m_transfer: write addr %p data 1 ret %d", addr + 4, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	ret = __raw_readb(addr + 3);
	ret = (ret << 16) | (ch & 0xff);

	copy_to_user((void __user *)arg, &ret, sizeof(ret));
	mutex_unlock(&fpgadrv_devp->lock);

	return 0;
}

int fpgadrvctl_write_2m_transfer(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0064;
	int ch; /* bit31~16: 1: valid 0: invalid; bit15~bit0: channel: bit7~bit0 */
	int value, ret;

	copy_from_user(&ch, (void __user *)arg, sizeof(int));

	mutex_lock(&fpgadrv_devp->lock);

	__raw_writeb(0, addr);   /* write opt */
	ret = __raw_readb(addr);
	if (ret != 0) {
		FPGADRVERR("write_2m_transfer: write addr %p data 0, ret %d", addr, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = ch & 0xff;
	__raw_writeb(value, addr + 1);   /* write channel */
	ret = __raw_readb(addr + 1);
	if (ret != value) {
		FPGADRVERR("write_2m_transfer: write addr %p data %d, ret %d", addr + 1, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	value = (ch >> 16) & 0xff;
	__raw_writeb(value, addr + 2);   /* write valid */
	ret = __raw_readb(addr + 2);
	if (ret != value) {
		FPGADRVERR("write_2m_transfer: write addr %p data %d, ret %d", addr + 2, value, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x0, addr + 4);     /* write en */
	ret = __raw_readb(addr + 4);
	if (ret != 0) {
		FPGADRVERR("write_2m_transfer: write addr %p data 0, ret %d", addr + 4, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb(0x1, addr + 4);
	ret = __raw_readb(addr + 4);
	if (ret != 1) {
		FPGADRVERR("write_2m_transfer: write addr %p data 1 ret %d", addr + 4, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	mutex_unlock(&fpgadrv_devp->lock);

	return 0;
}

int fpgadrvctl_read_e1phy_stat(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0070;
	int value, ret;

	mutex_lock(&fpgadrv_devp->lock);

	ret = __raw_readb(addr);
	value = ret << 8;
	ret = __raw_readb(addr + 1);
	value |= (ret & 0xff);

	copy_to_user((void __user *)arg, &value, sizeof(value));
	mutex_unlock(&fpgadrv_devp->lock);

	return 0;
}

int fpgadrvctl_write_e1phy_stat(unsigned long arg)
{
	char __iomem *addr = fpgadrv_devp->vaddr_bk_fpga + 0x0070;
	int value, ret;

	copy_from_user(&value, (void __user *)arg, sizeof(int));

	mutex_lock(&fpgadrv_devp->lock);

	__raw_writeb(((value >> 8) & 0xff), addr);   /* write opt */
	ret = __raw_readb(addr);
	if (ret != ((value >> 8) & 0xff)) {
		FPGADRVERR("write_e1phy_stat: write addr %p data 0x%x, ret %d", 
				addr, (value >> 8) & 0xff, ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	__raw_writeb((value & 0xff), addr + 1);   /* write channel */
	ret = __raw_readb(addr + 1);
	if (ret != (value & 0xff)) {
		FPGADRVERR("write_e1phy_stat: write addr %p data %d, ret %d", addr + 1, (value & 0xff), ret);
		mutex_unlock(&fpgadrv_devp->lock);
		return -1;
	}

	mutex_unlock(&fpgadrv_devp->lock);

	return 0;
}

#define FPGA_CCK_ADDR 0x00006
#define FPGA_SET_ADDR 0x00007
#define FPGA_REB_ADDR 0x00111

int fpgadrvctl_start_up_device(void)
{
	int cnt = 200;
	volatile unsigned char __iomem *cck_addr = NULL;
	volatile unsigned char __iomem *set_addr = NULL;
	volatile unsigned char __iomem *reb_addr = NULL;

	FPGADRVDBG("[%s:%d], Start-up FPGA device.\n", __func__, __LINE__);

	if (NULL != fpgadrv_devp) {
		if (fpgadrv_devp->stat == STARTUPED) {
			FPGADRVINFO("Device has start-up.");
			return 1;
		}

		cck_addr= fpgadrv_devp->vaddr_bk_fpga + FPGA_CCK_ADDR;
		set_addr= fpgadrv_devp->vaddr_bk_fpga + FPGA_SET_ADDR;
		reb_addr= fpgadrv_devp->vaddr_bk_fpga + FPGA_REB_ADDR;

		mdelay(1000);
		//Set DDR reset.
		*reb_addr = 0x00;
		mdelay(1000);
		if (*reb_addr != 0x00) {
			FPGADRVERR("0x%p:0x%x need 0x00.", reb_addr, *reb_addr);
			return -1;
		}

		//Release DDR reset.
		mdelay(1000);
		*reb_addr = 0x01;
		mdelay(1000);
		if (*reb_addr != 0x01) {
			FPGADRVERR("0x%p:0x%x need 0x01.", reb_addr, *reb_addr);
			return -1;
		}

		while (cnt--) {
			mdelay(1000);
			 if (*cck_addr== 1) {
				 break;
			 }
		};

		if (*cck_addr == 1) {
			*set_addr = 0x01;
		}
		else {
			FPGADRVERR("Start-up time out.\n");
			return -1;
		}

		FPGADRVDBG("0x%x:%x, 0x%x:%x, 0x%x:%x", reb_addr, *reb_addr, cck_addr, *cck_addr, set_addr, *set_addr);
		fpgadrv_devp->stat = STARTUPED;

		return 0;
	}

	return -1;
}
