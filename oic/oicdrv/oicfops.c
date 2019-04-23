/*

 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oicfops.c - A description goes here.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "oicdbg.h"
#include "oic.h"
#include "oicdev.h"
#include "fpga.h"
#include "gfp_fpga.h"
#include "sdh_fpga.h"
#include "oicfops.h"
#include "fst.h"

#if defined(HAVE_REGCHK)
#define BDREGCHK(reg, val) { \
	(reg) = (val); \
	if ((reg) != (val)) { \
	    (reg) = (val); \
        mdelay(10); \
	    if ((reg) != (val)) { \
	        (reg) = (val); \
            mdelay(100); \
            if ((reg) != (val)) { \
                OICERR("%s %d : %d <---> %d %p", __FILE__, __LINE__, (val), (reg), (&reg)); \
                mutex_unlock(&oicdev->lock); \
		        return -EIO; \
	        } \
	    } \
	} \
}

#define BDREGCHK_FREE(reg, val, kfr) { \
	(reg) = (val); \
	if ((reg) != (val)) { \
		OICERR("%s %d: %d <---> %d", __FILE__, __LINE__, (val), (reg)); \
		kfree(kfr); \
		mutex_unlock(&oicdev->lock); \
		return -EIO; \
	} \
}
#else
#define BDREGCHK(reg, val) { \
	(reg) = (val); \
}

#define BDREGCHK_FREE(reg, val, kfr) { \
	(reg) = (val); \
	kfree(kfr); \
}
#endif

extern int fpgadev;
unsigned int g_channel1 = 0;
unsigned int g_channel2 = 0;

static int oic_init_fpga_start(void)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	int val = 0;
	int i;

	/* reset fpga ddr */
	msleep(500);
	mutex_lock(&oicdev->lock);
	BDREGCHK(fpga->age_ch.age_chan1, 0x00);
	mutex_unlock(&oicdev->lock);
	msleep(500);
	mutex_lock(&oicdev->lock);
	BDREGCHK(fpga->age_ch.age_chan1, 0x01);
	mutex_unlock(&oicdev->lock);

#define TIME_OUT  100

	for (i = 0; i < TIME_OUT; i++) {
		mutex_lock(&oicdev->lock);
		val = fpga->startup.dsr;
		mutex_unlock(&oicdev->lock);
		if (val == 0x01) {
			break;
		}
		msleep(100);
	}

	if (i >= TIME_OUT) {
		OICERR("board init fpga start: check ddr %d failed", val);
		return -EFAULT;
	}

	mutex_lock(&oicdev->lock);
	BDREGCHK(fpga->startup.bcr, 0x01);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_init_gfp_fpga_start(void)
{
	volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
	volatile sdh_cpld_reg *cpld = (volatile sdh_cpld_reg *)oicdev->cpld_virt;
	int val = 0;
	int i = 0;

	/* ddr stop reset */
    mutex_lock(&oicdev->lock);
    BDREGCHK(fpga->startup.dstop, 0x01);
    mutex_unlock(&oicdev->lock);
    msleep(500);

#define GFP_TIME_OUT  10
	/* check ddr status */
	for (i = 0; i < GFP_TIME_OUT; i++) {
		msleep(500);
		mutex_lock(&oicdev->lock);
		val = fpga->startup.dsr;
		mutex_unlock(&oicdev->lock);
		if (val == 0x01) {
			break;
		}
	}
	if (i >= GFP_TIME_OUT) {
		OICERR("board init fpga start: check ddr %d failed", val);
		return -EFAULT;
	}

    msleep(500);
	/* enable control reg */
    if (cpld->cpld_status != 0xef) {
        mutex_lock(&oicdev->lock);
        BDREGCHK(fpga->startup.bcr, 0x01);
        mutex_unlock(&oicdev->lock);
    } else {
        OICINFO("CPLD is changed."); 
    }

	return 0;
}

static int oic_fops_ioctl_get_fpga_version(unsigned long arg)
{
	struct fpga_verinfo ver;
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile verinfo_t *verinfo = (volatile verinfo_t *)&fpga->verinfo;

	memset(&ver, 0, sizeof(ver));
	mutex_lock(&oicdev->lock);
	ver.version = verinfo->vrr;
	ver.date    = (verinfo->dar[0] << 24) | (verinfo->dar[1] << 16) |
		(verinfo->dar[2] <<  8) | (verinfo->dar[3] <<  0);
	mutex_unlock(&oicdev->lock);

	if (copy_to_user((void __user *)arg, &ver, sizeof(ver)))
		return -EFAULT;

	return 0;
}

static int oic_fops_ioctl_get_fpga_ddr_status(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdstartup_t *startup = (volatile bdstartup_t *)&fpga->startup;
	unsigned char val = 0;

	mutex_lock(&oicdev->lock);
	val = startup->dsr;
	mutex_unlock(&oicdev->lock);

	return put_user(val, (int *)arg);
}

static int oic_fops_ioctl_set_fpga_bd_startup(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdstartup_t *startup = (volatile bdstartup_t *)&fpga->startup;
	int val;

	if (get_user(val, (int *)arg))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	BDREGCHK(startup->bcr, (val) ? 1 : 0);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_fops_ioctl_get_fpga_bd_startup(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdstartup_t *startup = (volatile bdstartup_t *)&fpga->startup;
	unsigned char val = 0;

	mutex_lock(&oicdev->lock);
	val = startup->bcr;
	mutex_unlock(&oicdev->lock);

	return put_user(val, (int *)arg);
}

static int oic_fops_ioctl_set_fpga_synctime(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile timesync_t *timesync = (volatile timesync_t *)&fpga->timesync;
	struct fpga_time_info tinfo;

	if (copy_from_user(&tinfo, (void __user *)arg, sizeof(tinfo)))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	BDREGCHK(timesync->ser[0], (tinfo.sec >> 24) & 0xff);
	BDREGCHK(timesync->ser[1], (tinfo.sec >> 16) & 0xff);
	BDREGCHK(timesync->ser[2], (tinfo.sec >>  8) & 0xff);
	BDREGCHK(timesync->ser[3], (tinfo.sec >>  0) & 0xff);
	BDREGCHK(timesync->usr[0], (tinfo.usec >> 24) & 0xff);
	BDREGCHK(timesync->usr[1], (tinfo.usec >> 16) & 0xff);
	BDREGCHK(timesync->usr[2], (tinfo.usec >>  8) & 0xff);
	BDREGCHK(timesync->usr[3], (tinfo.usec >>  0) & 0xff);
	BDREGCHK(timesync->ter, 0);
	BDREGCHK(timesync->ter, 1);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_fops_ioctl_set_fpga_bd_cfginfo(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdcfginfo_t *cfginfo = (volatile bdcfginfo_t *)&fpga->cfginfo;
	struct fpga_board_cfginfo bdinfo;
	char sel, data;
	int i;

	if (copy_from_user(&bdinfo, (void __user *)arg, sizeof(bdinfo)))
		return -EFAULT;

	if (FMASK_DSTMAC == (bdinfo.fmask & FMASK_DSTMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x0; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->wvr, bdinfo.dstmac[i]);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCMAC == (bdinfo.fmask & FMASK_SRCMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x10; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->wvr, bdinfo.srcmac[i]);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_ETHERTYPE == (bdinfo.fmask & FMASK_ETHERTYPE)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x21; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			data = (bdinfo.ethertype >> (8 * i)) & 0xff;
			BDREGCHK(cfginfo->wvr, data);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SLOT == (bdinfo.fmask & FMASK_SLOT)) {
		mutex_lock(&oicdev->lock);
		BDREGCHK(cfginfo->itr, 0x30);
		data = bdinfo.slot;
		BDREGCHK(cfginfo->wvr, data);
		BDREGCHK(cfginfo->wer, 0);
		BDREGCHK(cfginfo->wer, 1);
		mutex_unlock(&oicdev->lock);
	}

	return 0;
}

static int oic_fops_ioctl_get_fpga_bd_cfginfo(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdcfginfo_t *cfginfo = (volatile bdcfginfo_t *)&fpga->cfginfo;
	struct fpga_board_cfginfo bdinfo;
	char sel;
	int i;

	if (copy_from_user(&bdinfo, (void __user *)arg, sizeof(bdinfo)))
		return -EFAULT;

	if (FMASK_DSTMAC == (bdinfo.fmask & FMASK_DSTMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x0; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			bdinfo.dstmac[i] = cfginfo->rvr;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCMAC == (bdinfo.fmask & FMASK_SRCMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x10; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			bdinfo.srcmac[i] = cfginfo->rvr;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_ETHERTYPE == (bdinfo.fmask & FMASK_ETHERTYPE)) {
		mutex_lock(&oicdev->lock);
		BDREGCHK(cfginfo->itr, 0x20);
		BDREGCHK(cfginfo->rer, 0);
		BDREGCHK(cfginfo->rer, 1);
		bdinfo.ethertype = cfginfo->rvr;
		bdinfo.ethertype = (bdinfo.ethertype << 8);
		BDREGCHK(cfginfo->itr, 0x21);
		BDREGCHK(cfginfo->rer, 0);
		BDREGCHK(cfginfo->rer, 1);
		bdinfo.ethertype |= cfginfo->rvr;
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SLOT == (bdinfo.fmask & FMASK_SLOT)) {
		mutex_lock(&oicdev->lock);
		BDREGCHK(cfginfo->itr, 0x30);
		BDREGCHK(cfginfo->rer, 0);
		BDREGCHK(cfginfo->rer, 1);
		bdinfo.slot = cfginfo->rvr;
		mutex_unlock(&oicdev->lock);
	}

	return copy_to_user((void __user *)arg, &bdinfo, sizeof(bdinfo));
}

static int oic_fops_ioctl_set_fpga_bd_cfginfo_ex(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdcfginfo_t *cfginfo = (volatile bdcfginfo_t *)&fpga->cfginfo;
	struct fpga_board_cfginfo_ex bdinfo;
	char sel, data;
	int i;

	if (copy_from_user(&bdinfo, (void __user *)arg, sizeof(bdinfo)))
		return -EFAULT;

	if (FMASK_DSTMAC == (bdinfo.fmask & FMASK_DSTMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x0; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->wvr, bdinfo.dstmac[i]);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCMAC == (bdinfo.fmask & FMASK_SRCMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x10; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->wvr, bdinfo.srcmac[i]);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_DSTIP == (bdinfo.fmask & FMASK_DSTIP)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x23; i < 4; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			data = (bdinfo.dstip >> (8 * i)) & 0xff;
			BDREGCHK(cfginfo->wvr, data);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCIP == (bdinfo.fmask & FMASK_SRCIP)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x33; i < 4; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			data = (bdinfo.srcip >> (8 * i)) & 0xff;
			BDREGCHK(cfginfo->wvr, data);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_DSTPORT == (bdinfo.fmask & FMASK_DSTPORT)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x41; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			data = (bdinfo.dstport >> (8 * i)) & 0xff;
			BDREGCHK(cfginfo->wvr, data);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCPORT == (bdinfo.fmask & FMASK_SRCPORT)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x51; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			data = (bdinfo.srcport >> (8 * i)) & 0xff;
			BDREGCHK(cfginfo->wvr, data);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_DEVID == (bdinfo.fmask & FMASK_DEVID)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x61; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			data = (bdinfo.devid >> (8 * i)) & 0xff;
			BDREGCHK(cfginfo->wvr, data);
			BDREGCHK(cfginfo->wer, 0);
			BDREGCHK(cfginfo->wer, 1);
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SLOT == (bdinfo.fmask & FMASK_SLOT)) {
		mutex_lock(&oicdev->lock);
		BDREGCHK(cfginfo->itr, 0x70);
		data = bdinfo.slot;
		BDREGCHK(cfginfo->wvr, data);
		BDREGCHK(cfginfo->wer, 0);
		BDREGCHK(cfginfo->wer, 1);
		mutex_unlock(&oicdev->lock);
	}

	return 0;
}

static int oic_fops_ioctl_get_fpga_bd_cfginfo_ex(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdcfginfo_t *cfginfo = (volatile bdcfginfo_t *)&fpga->cfginfo;
	struct fpga_board_cfginfo_ex bdinfo;
	char sel;
	int i;
	unsigned int ip, devid;
	unsigned short port;

	if (copy_from_user(&bdinfo, (void __user *)arg, sizeof(bdinfo)))
		return -EFAULT;

	if (FMASK_DSTMAC == (bdinfo.fmask & FMASK_DSTMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x0; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			bdinfo.dstmac[i] = cfginfo->rvr;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCMAC == (bdinfo.fmask & FMASK_SRCMAC)) {
		mutex_lock(&oicdev->lock);
		for (i = 0, sel = 0x10; i < 6; sel++, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			bdinfo.srcmac[i] = cfginfo->rvr;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_DSTIP == (bdinfo.fmask & FMASK_DSTIP)) {
		mutex_lock(&oicdev->lock);
		bdinfo.dstip = 0;
		for (i = 0, sel = 0x23; i < 4; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			ip = cfginfo->rvr;
			ip = ip << (8 * i);
			bdinfo.dstip |= ip;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCIP == (bdinfo.fmask & FMASK_SRCIP)) {
		mutex_lock(&oicdev->lock);
		bdinfo.srcip = 0;
		for (i = 0, sel = 0x33; i < 4; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			ip = cfginfo->rvr;
			ip = ip << (8 * i);
			bdinfo.srcip |= ip;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_DSTPORT == (bdinfo.fmask & FMASK_DSTPORT)) {
		mutex_lock(&oicdev->lock);
		bdinfo.dstport = 0;
		for (i = 0, sel = 0x41; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			port = cfginfo->rvr;
			port = port << (8 * i);
			bdinfo.dstport |= port;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SRCPORT == (bdinfo.fmask & FMASK_SRCPORT)) {
		mutex_lock(&oicdev->lock);
		bdinfo.srcport = 0;
		for (i = 0, sel = 0x51; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			port = cfginfo->rvr;
			port = port << (8 * i);
			bdinfo.srcport |= port;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_DEVID == (bdinfo.fmask & FMASK_DEVID)) {
		mutex_lock(&oicdev->lock);
		bdinfo.devid = 0;
		for (i = 0, sel = 0x61; i < 2; sel--, i++) {
			BDREGCHK(cfginfo->itr, sel);
			BDREGCHK(cfginfo->rer, 0);
			BDREGCHK(cfginfo->rer, 1);
			devid = cfginfo->rvr;
			devid = devid << (8 * i);
			bdinfo.devid |= devid;
		}
		mutex_unlock(&oicdev->lock);
	}

	if (FMASK_SLOT == (bdinfo.fmask & FMASK_SLOT)) {
		mutex_lock(&oicdev->lock);
		BDREGCHK(cfginfo->itr, 0x70);
		BDREGCHK(cfginfo->rer, 0);
		BDREGCHK(cfginfo->rer, 1);
		bdinfo.slot = cfginfo->rvr;
		mutex_unlock(&oicdev->lock);
	}

	return copy_to_user((void __user *)arg, &bdinfo, sizeof(bdinfo));
}

static int oic_fops_ioctl_get_fpga_bd_runinfo(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile bdruninfo_t *runinforeg = (volatile bdruninfo_t *)&fpga->runinfo;
	struct fpga_board_runinfo runinfo;
	unsigned char data;
	int i;

	if (copy_from_user(&runinfo, (void __user *)arg, sizeof(runinfo)))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		BDREGCHK(runinforeg->ptr, (i << 4) & 0xff);
		data = runinforeg->oir;
		runinfo.ports[i].los = (data >> 7) & 0x01;
		runinfo.ports[i].stm1_synced = (data >> 6) & 0x01;
		runinfo.ports[i].e1_synced_num = data & 0x3f;
		BDREGCHK(runinforeg->str, 0);
		runinfo.ports[i].ch_64k_num = ((runinforeg->scnr0) << 8) | (runinforeg->scnr1);
		runinfo.ports[i].ch_64k_frames = ((runinforeg->sfnr0) << 24) | ((runinforeg->sfnr1) << 16) |
										 ((runinforeg->sfnr2) << 8) | (runinforeg->sfnr3);
		BDREGCHK(runinforeg->str, 1);
		runinfo.ports[i].ch_2m_num = ((runinforeg->scnr0) << 8) | (runinforeg->scnr1);
		runinfo.ports[i].ch_2m_frames = ((runinforeg->sfnr0) << 24) | ((runinforeg->sfnr1) << 16) |
										 ((runinforeg->sfnr2) << 8) | (runinforeg->sfnr3);
	}
	
	runinfo.traffic =  ((runinforeg->etr0) << 24) | ((runinforeg->etr1) << 16) |
						((runinforeg->etr2) << 8) | (runinforeg->etr3);
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, &runinfo, sizeof(runinfo));
}

static int oic_fops_ioctl_set_fpga_64k_ch_tran(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile trsch_t *reg = (volatile trsch_t *)&fpga->trsch;
	struct fpga_trans_info trsch;
	unsigned char data;

	if (copy_from_user(&trsch, (void __user *)arg, sizeof(trsch)))
		return -EFAULT;
	
	mutex_lock(&oicdev->lock);
	data = (trsch.channel >> 8) & 0x1f;
	OICDBG("set 64k ch tran, channel:0x%x\n", trsch.channel);
	BDREGCHK(reg->btcr0, data);
	data = trsch.channel & 0xff;
	BDREGCHK(reg->btcr1, data);
	data = trsch.valid ? 1 : 0;	
	OICDBG("set 64k ch tran, valid:0x%x\n", data);
	BDREGCHK(reg->btvr, data);
	BDREGCHK(reg->bter, 0);
	BDREGCHK(reg->bter, 1);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_fops_ioctl_set_fpga_2m_ch_tran(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile trs2mch_t *reg = (volatile trs2mch_t *)&fpga->trs2mch;
	struct fpga_trans_info trsch;
	unsigned char data;

	if (copy_from_user(&trsch, (void __user *)arg, sizeof(trsch)))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	data = trsch.channel & 0xff;
	OICDBG("set 2M ch tran, link:0x%x\n", data);
	BDREGCHK(reg->bcr, data);
	data = trsch.valid ? 1 : 0;	
	OICDBG("set 2M ch tran, valid:0x%x\n", data);
	BDREGCHK(reg->bvr, data);
	BDREGCHK(reg->ber, 0);
	BDREGCHK(reg->ber, 1);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_fops_ioctl_set_fpga_2m_ch_valid(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile vld2mch_t *reg = (volatile vld2mch_t *)&fpga->vld2mch;
	struct fpga_trans_info trsch;
	unsigned char data;

	if (copy_from_user(&trsch, (void __user *)arg, sizeof(trsch)))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	BDREGCHK(reg->sel, 0);
	data = trsch.channel & 0xff;
	OICDBG("set 2M ch valid, link:0x%x\n", data);
	BDREGCHK(reg->bcr, data);
	data = trsch.valid ? 1 : 0;	
	OICDBG("set 2M ch valid, valid:0x%x\n", data);
	BDREGCHK(reg->wrr, data);
	BDREGCHK(reg->ber, 0);
	BDREGCHK(reg->ber, 1);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_fops_ioctl_get_fpga_2m_ch_valid(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile vld2mch_t *reg = (volatile vld2mch_t *)&fpga->vld2mch;
	struct fpga_trans_info trsch;
	unsigned char data;

	if (copy_from_user(&trsch, (void __user *)arg, sizeof(trsch)))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	BDREGCHK(reg->sel, 1);
	data = trsch.channel & 0xff;
	OICDBG("get 2M ch valid, link:0x%x\n", data);
	BDREGCHK(reg->bcr, data);
	BDREGCHK(reg->ber, 0);
	BDREGCHK(reg->ber, 1);
	data = reg->rdr;
	trsch.valid = data;
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, &trsch, sizeof(trsch));
}
static int  oic_fops_ioctl_set_fpga_pl_mode(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile plcheck0_t *reg = (volatile plcheck0_t *)&fpga->plc0;
	struct fpga_cfg cfg;
	unsigned char data;
	
	if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg)))
		return -EFAULT;

		if (cfg.mode > 3) {
		OICERR("MODE parm error.---%s\n",__FUNCTION__);
		return -EFAULT;
	}

	mutex_lock(&oicdev->lock);
	data = reg->csr;

	data = (data & 0xfd) | (cfg.mode & 0x03);
	OICDBG("linkmap, Mode:%x\n", (data&0x03));
	BDREGCHK(reg->csr, data);
	mutex_unlock(&oicdev->lock);

	return 0;
}
static int oic_fops_ioctl_set_fpga_pl_cfgch(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile plcheck0_t *reg = (volatile plcheck0_t *)&fpga->plc0;
	struct  fpga_cfg cfg;
	unsigned char data;
	unsigned char tmp;

	if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg)))
		return -EFAULT;

	if (cfg.index > 65536){
		OICERR("CIC range out.---%s\n",__FUNCTION__);
		return -EFAULT;
	}
	
	mutex_lock(&oicdev->lock);
	data = (cfg.index >> 8) & 0xff;
	OICDBG("linkmap, CIC0:0x%x\n", data);
	BDREGCHK(reg->cicr0, data);
	data = cfg.index & 0xff;
	OICDBG("linkmap, CIC1:0x%x\n", data);
	BDREGCHK(reg->cicr1, data);
	
	
	tmp = reg->csr;	
	cfg.ts = (((cfg.en << 7) & 0x80) | ((cfg.ts << 2) & 0x7c));
	data =(tmp & 0x03) | (cfg.ts & 0xfc);
	OICDBG("linkmap, stat:0x%x\n", (data & 0xfc));
	BDREGCHK(reg->csr, data);
	BDREGCHK(reg->cer, 0);
	BDREGCHK(reg->cer, 1);
	mutex_unlock(&oicdev->lock);

	return 0;
}

static int oic_fops_ioctl_set_fpga_pl_rstch(void)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile plcheck0_t *reg = (volatile plcheck0_t *)&fpga->plc0;

	mutex_lock(&oicdev->lock);
	fst_table_unset((struct fst_ctl *)oicdev->link_map);
	OICDBG("linkmap, reset.\n");
	BDREGCHK(reg->plr, 1);
	BDREGCHK(reg->plr, 0);
	mutex_unlock(&oicdev->lock);

	return 0;
}
static int oic_fops_ioctl_set_fpga_pl_tbl_info(unsigned long arg)
{
	struct fpga_pl_tbl_info tblinfo;
	struct fst_ctl *ctl = NULL;

	if (oicdev->link_map == NULL) {
		printk("Parameter invalid.\n");
		return -1;
	}

	if (copy_from_user(&tblinfo, (void __user *)arg, sizeof(tblinfo)))
		return -EFAULT;
	
	mutex_lock(&oicdev->lock);
	ctl = (struct fst_ctl *)oicdev->link_map;
	ctl->tbl.slot = tblinfo.slot;
	ctl->tbl.subslot = tblinfo.subslot;
	mutex_unlock(&oicdev->lock);

	return 0;
}
static int oic_fops_ioctl_set_fpga_pl_age_ch(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile age_channel *reg = (volatile age_channel *)&fpga->age_ch;
	struct fpga_set_age set_age;
	
	if (copy_from_user(&set_age, (void __user *)arg, sizeof(set_age)))
		return -EFAULT;

	if(set_age.flag != 1) {
		printk("Age channel:Parameter invalid. 0 or 1\n");
		return -EFAULT;
	}
	mutex_lock(&oicdev->lock);
	BDREGCHK(reg->age_chan1, 0x00);
	msleep(500);
	BDREGCHK(reg->age_chan1, 0x01);
	BDREGCHK(reg->age_chan2, 0x00);
	msleep(500);
	BDREGCHK(reg->age_chan2, 0x01);
	mutex_unlock(&oicdev->lock);
	
	return 0;
}
static int oic_fops_ioctl_get_fpga_pl_match(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile plcheck0_t *reg = (volatile plcheck0_t *)&fpga->plc0;
	struct fst_ctl *ctrl = NULL;
	int tblnum = 0;

	if (oicdev->link_map == NULL) {
		printk("Initialize fst first.\n");
		return -1;
	}

	OICDBG("linkmap, get stat.\n");

	mutex_lock(&oicdev->lock);
	ctrl = (struct fst_ctl *)oicdev->link_map;

	BDREGCHK(reg->plr, 0x2);
	BDREGCHK(reg->plr, 0x0);
	tblnum = 0;
	fst_read_stat((void *)ctrl, tblnum);

	BDREGCHK(reg->plr, 0x4);
	BDREGCHK(reg->plr, 0x0);
	tblnum = 1;
	fst_read_stat((void *)ctrl, tblnum);
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, (void *)&ctrl->tbl, sizeof(ctrl->tbl));
}

static int oic_fops_ioctl_get_fpga_in_stat(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile becode_t *breg = (volatile becode_t *)&fpga->becode;
	volatile bdruninfo_t *bdreg = (volatile bdruninfo_t *)&fpga->runinfo;
	volatile chipstat_t *csreg = (volatile chipstat_t *)&fpga->cstat;
	struct fpga_in_stat stat;
	int i, j;
	unsigned char sel;

	mutex_lock(&oicdev->lock);
	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		sel = i * 3;
		BDREGCHK(breg->bsr, sel);
		stat.b1_ecode[i] = (breg->bvr0 << 8) | breg->bvr1;
		BDREGCHK(breg->bsr, sel + 1);
		stat.b2_ecode[i] = (breg->bvr0 << 8) | breg->bvr1;
		BDREGCHK(breg->bsr, sel + 2);
		stat.b3_ecode[i] = (breg->bvr0 << 8) | breg->bvr1;
	}

	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		sel = i;
		BDREGCHK(bdreg->ptr, sel << 4);
		stat.auptr[i].auptr = (csreg->auptr[0] << 8) | csreg->auptr[1];
		stat.auptr[i].aundf0 = ((csreg->auptrndf0[0] & 0xf) << 8) | (csreg->auptrndf0[1] & 0xf);
		stat.auptr[i].aundf1 = ((csreg->auptrndf1[0] & 0xf) << 8) | (csreg->auptrndf1[1] & 0xf);
	}

	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		sel = i;
		BDREGCHK(bdreg->ptr, sel << 4);
		for (j = 0; j < FPGA_OI_E1NUM; j++) {
			sel = j;
			BDREGCHK(csreg->enr, sel);
			stat.v5[i].v5_ecode[j] = (csreg->v5ecr[0] << 8) | csreg->v5ecr[1];
			stat.v5[i].v5ptr[j] = (csreg->v5ptr << 8) | csreg->v5vr;
			stat.v5[i].v5ndf0[j] = ((csreg->tuptrndf0[0] & 0xf) << 8) | (csreg->tuptrndf0[1] & 0xf);
			stat.v5[i].v5ndf1[j] = ((csreg->tuptrndf1[0] & 0xf) << 8) | (csreg->tuptrndf1[1] & 0xf);

		}
	}

	for (i = 0; i < FPGA_OI_PORTNUM; i++) {
		sel = i;
		BDREGCHK(bdreg->ptr, sel << 4);
		stat.e1[i].e1_change = csreg->eccr;
		for (j = 0; j < FPGA_OI_E1NUM; j++) {
			sel = j;
			BDREGCHK(csreg->enr, sel);
			stat.e1[i].e1_sync[j] = csreg->ser & 0x1;
			stat.e1[i].e1_sync_ecode[j] = ((csreg->seecr[0] & 0xef) << 8) | csreg->seecr[1];
			stat.e1[i].e1_speed_diff[j] = (csreg->esdr & 0x1);
		}
	}
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, (void *)&stat, sizeof(stat));
}

static int oic_fops_ioctl_get_fpga_is_sigch(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile sigchannel_t *reg = (volatile sigchannel_t *)&fpga->issig;
	struct fpga_is_sigch sigch;
	unsigned short ramaddr, ramdata;
	short idx;
	unsigned char data;
	unsigned char port, e1, ts;

	if (copy_from_user(&sigch, (void __user *)arg, sizeof(sigch)))
		return -EFAULT;

	mutex_lock(&oicdev->lock);
	for (idx = 0; idx < 4096; idx++) {
		ramaddr = (0x1000 | idx);
		data = ramaddr;
		BDREGCHK(reg->adr0, data);
		data = (ramaddr >> 8);
		BDREGCHK(reg->adr1, data);
		data = reg->data1;
		if ((data & 0x20) != 0x20) {
			continue ;
		}
		ramdata = (data << 8);
		ramdata |= reg->data0;
		port = (ramdata >> 11) & 0x3;
		e1 = (ramdata >> 5) & 0x3f;
		ts = ramdata & 0x1f;
		sigch.sig[port][e1] |= (1 << ts);
		OICDBG("signal ch: %d %d %d", port, e1, ts);
	}
	/*clear table*/
	BDREGCHK(reg->adr1, 0x10);
	BDREGCHK(reg->adr1, 0x30);
	BDREGCHK(reg->adr1, 0x0);
	mutex_unlock(&oicdev->lock);
	
	return copy_to_user((void __user *)arg, &sigch, sizeof(sigch));
}

int oic_fops_ioctl_set_silence_range(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile silence_t *slp = (volatile silence_t *)&fpga->slp;
	unsigned int range = 0;

	copy_from_user(&range, (void __user *)arg, sizeof(range));
	mutex_lock(&oicdev->lock);
	BDREGCHK(slp->srl0, (range >> 24) & 0xff);
	BDREGCHK(slp->srh0, (range >> 16) & 0xff);
	BDREGCHK(slp->srl1, (range >> 8) & 0xff);
	BDREGCHK(slp->srh1, (range >> 0) & 0xff);
	mutex_unlock(&oicdev->lock);

    return 0;
}

int oic_fops_ioctl_get_silence_range(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile silence_t *slp = (volatile silence_t *)&fpga->slp;
	unsigned int range = 0;

	mutex_lock(&oicdev->lock);
	range = (slp->srl0 << 24) | (slp->srh0 << 16) | (slp->srl1 << 8) | slp->srh1;
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, &range, sizeof(range));
}

int oic_fops_ioctl_get_silence_result(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile silence_t *slp = (volatile silence_t *)&fpga->slp;
	struct silence_result srp;

	memset(&srp, 0, sizeof(srp));
	copy_from_user(&srp, (void __user *)arg, sizeof(srp));
	mutex_lock(&oicdev->lock);
	BDREGCHK(slp->schr, (srp.ch >> 8) & 0xff);
	BDREGCHK(slp->sclr, srp.ch & 0xff);
	srp.stat = slp->srsr;
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, &srp, sizeof(srp));
}

int oic_fops_ioctl_set_silence_timeout(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile silence_t *slp = (volatile silence_t *)&fpga->slp;
	unsigned char timeout = 0;

	copy_from_user(&timeout, (void __user *)arg, sizeof(timeout));
	mutex_lock(&oicdev->lock);
	BDREGCHK(slp->str, timeout);
	mutex_unlock(&oicdev->lock);

    return 0;
}

int oic_fops_ioctl_get_silence_timeout(unsigned long arg)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile silence_t *slp = (volatile silence_t *)&fpga->slp;
	unsigned char timeout = 0;

	mutex_lock(&oicdev->lock);
	timeout = slp->str;
	mutex_unlock(&oicdev->lock);

	return copy_to_user((void __user *)arg, &timeout, sizeof(timeout));
}

static int get_info_rule (unsigned char sel_port, unsigned char sel_chan, unsigned char sel_info, int *val)
{
	volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    int reg_val = 0;
    unsigned char tmp3val = 0;
    unsigned char tmp2val = 0;
    unsigned char tmp1val = 0;
    unsigned char tmp0val = 0;

	mutex_lock(&oicdev->lock);
	BDREGCHK(ginfo->select_port, sel_port);
    if (sel_chan != 0xff) {
        BDREGCHK(ginfo->select_chan, sel_chan);
    }
	BDREGCHK(ginfo->select_info, sel_info);

    //mdelay(1);
    tmp3val = ginfo->get_val_3;

    tmp2val = ginfo->get_val_2;

    tmp1val = ginfo->get_val_1;

    tmp0val = ginfo->get_val_0;

    reg_val = (tmp3val << 24) | (tmp2val << 16) | (tmp1val << 8) | (tmp0val << 0);

    *val = reg_val;
	mutex_unlock(&oicdev->lock);

    return 0;
}

#define CHANNEL_HIGH 0x01
static int analysis_sel_info(unsigned int reg_val, unsigned int sel_info, struct fpga_board_runinfo_port_ex *port_rinfo, struct slink_info *slink)
{
    unsigned char tmp = 0;

	mutex_lock(&oicdev->lock);
    switch (sel_info) {
        case 0x00:
            port_rinfo->auptr_1001_no = (reg_val >> 0) & 0x0f;
            port_rinfo->auptr_1001_rev = (reg_val >> 4) & 0x0f;
            port_rinfo->auptr_0110_no = (reg_val >> 8) & 0x0f;
            port_rinfo->auptr_0110_rev = (reg_val >> 12) & 0x0f;
            port_rinfo->auptr_val = (reg_val >> 16) & 0xffff;
            break;

        case 0x01:
            slink->v5_cnt = (reg_val >> 0) & 0xffff;
            slink->v5_val = (reg_val >> 16) & 0xff;
            slink->tuptr_val = (reg_val >> 24) & 0xff;
            break;

        case 0x02:
            slink->tuptr_1001_no = (reg_val >> 0) & 0x0f;
            slink->tuptr_1001_rev = (reg_val >> 4) & 0x0f;
            slink->tuptr_0110_no = (reg_val >> 8) & 0x0f;
            slink->tuptr_0110_rev = (reg_val >> 12) & 0x0f;
            port_rinfo->b1_cnt = (reg_val >> 16) & 0xffff;
            break;

        case 0x03:
            port_rinfo->b2_cnt = (reg_val >> 16) & 0xffff;
            port_rinfo->b3_cnt = (reg_val >> 0) & 0xffff;
            break;

        case 0x04:
            port_rinfo->los = (reg_val >> 7) & 0x01;
            port_rinfo->stm1_synced = (reg_val >> 6) & 0x01;
            port_rinfo->phy_type = (reg_val >> 2) & 0x0f;
            port_rinfo->c2_val = (reg_val >> 8) & 0xff;
            port_rinfo->chan_rate = (reg_val >> 16) & 0x03;
            slink->fiber_rate = (reg_val >> 2) & 0x0f;
            slink->channel_rate = (reg_val >> 16) & 0x03;
            OICDBG("reg_val 0x%x fiber_rate %d chan rate %d c2 %d", reg_val, slink->fiber_rate, slink->channel_rate, port_rinfo->c2_val);
            break;

        case 0x05:
#if defined(XSCB_V2)
            port_rinfo->j1_3 = reg_val;
#else
            slink->sq = (reg_val >> 0) & 0xff;
            slink->mfi = (reg_val >> 8) & 0xfff;
            slink->svc_type = (reg_val >> 20) & 0x0f;
            tmp = (reg_val >> 24) & 0x0f;
            if (tmp == 0x02 || tmp == 0x03) {
                slink->is_lcas = 1;
                slink->is_member = 1;
            } else {
                slink->is_lcas = 0;
                slink->is_member = 0;
            }
            if (tmp == 0x03) {
                slink->is_last_member = 1;
            } else {
                slink->is_last_member = 0;
            }
            tmp = (reg_val >> 28) & 0x03;
            if (tmp == 0x03) {
                slink->vc_valid = 1;
            } else {
                slink->vc_valid = 0;
            }
#endif
            break;

        case 0x06:
#if defined(XSCB_V2)
            port_rinfo->j1_2 = reg_val;
#else
            slink->pre_gid = reg_val; 
#endif
            break;

        case 0x07:
#if defined(XSCB_V2)
            port_rinfo->j1_1 = reg_val;
#else
            slink->cur_gid = reg_val; 
#endif
            break;

        case 0x08:
#if defined(XSCB_V2)
            port_rinfo->j1_0 = reg_val;
#else
            slink->v5_sync_cnt = (reg_val >> 0) & 0xff;
            slink->k4_sync_cnt = (reg_val >> 8) & 0xff;
            slink->sq_cnt = (reg_val >> 16) & 0xff;
            slink->mfi_cnt = (reg_val >> 24) & 0xff;
#endif
            break;

        case 0x09:
#if defined(XSCB_V2)
        break;
#endif
            if (slink->channel_rate == CHANNEL_HIGH) {
                slink->sq = (reg_val >> 0) & 0xff;
                slink->mfi = (reg_val >> 8) & 0xfff;
                slink->svc_type = (reg_val >> 20) & 0x0f;
                tmp = (reg_val >> 24) & 0x0f;
                if (tmp == 0x02 || tmp == 0x03) {
                    slink->is_lcas = 1;
                    slink->is_member = 1;
                } else {
                    slink->is_lcas = 0;
                    slink->is_member = 0;
                }
                if (tmp == 0x03) {
                    slink->is_last_member = 1;
                } else {
                    slink->is_last_member = 0;
                }
                tmp = (reg_val >> 28) & 0x01;
                if (tmp == 0x01) {
                    slink->vc_valid = 1;
                } else {
                    slink->vc_valid = 0;
                }
            }
            break;

        case 0x0a:
#if defined(XSCB_V2)
            break;
#endif
            if (slink->channel_rate == CHANNEL_HIGH) {
                slink->pre_gid = reg_val;
            }
            break;

        case 0x0b:
#if defined(XSCB_V2)
            break;
#endif
            if (slink->channel_rate == CHANNEL_HIGH) {
                slink->cur_gid = reg_val;
            }
            break;

        case 0x0c:
#if defined(XSCB_V2)
            break;
#endif
            slink->e1_sync = (reg_val & 0x01) >> 0;
            port_rinfo->e1_cnt = (reg_val & 0x7e) >> 1;
            slink->e1_sync_err = (reg_val & 0xff00) >> 8;

            slink->nfm_e1_sync = (reg_val & 0x10000) >> 16;
            port_rinfo->nfm_e1_cnt = (reg_val & 0x7e0000) >> 17;
            slink->nfm_e1_sync_err = (reg_val & 0xff000000) >> 24;
            break;

        case 0x0d:
        case 0x0e:
        case 0x0f:
            break;

        case 0x10:
            slink->vc_valid = (reg_val >> 28) & 0x0f;
            slink->svc_type = (reg_val >> 20) & 0x0f;
            tmp = (reg_val >> 24) & 0x0f;
            if ((tmp == 0x02) || (tmp == 0x03)) {
                slink->is_lcas = 1;
                slink->is_member = 1;
            } else {
                slink->is_lcas = 0;
                slink->is_member = 0;
            }
            if (tmp == 0x03) {
                slink->is_last_member = 1;
            } else {
                slink->is_last_member = 0;
            }
            tmp = (reg_val >> 28) & 0x01;
            if (tmp == 0x01) {
                slink->vc_valid = 1;
            } else {
                slink->vc_valid = 0;
            }
            slink->sq = (reg_val >> 0) & 0xff;
            slink->mfi = (reg_val >> 8) & 0xfff;
            break;

        case 0x11:
            slink->pre_gid = reg_val; 
            break;

        case 0x12:
            slink->cur_gid = reg_val; 
            break;

        case 0x13:
            slink->mfi_cnt = (reg_val >> 24) & 0xff;
            slink->sq_cnt = (reg_val >> 16) & 0xff;
            slink->k4_sync_cnt = (reg_val >> 8) & 0xff;
            slink->v5_sync_cnt = (reg_val >> 0) & 0xff;
            break;

        case 0x14:
            port_rinfo->vc_valid = (reg_val >> 28) & 0x0f;
            port_rinfo->svc_type = (reg_val >> 20) & 0x0f;
            tmp = (reg_val >> 24) & 0x0f;
            if ((tmp == 0x02) || (tmp == 0x03)) {
                port_rinfo->is_lcas = 1;
                port_rinfo->is_member = 1;
            } else {
                port_rinfo->is_lcas = 0;
                port_rinfo->is_member = 0;
            }
            if (tmp == 0x03) {
                port_rinfo->is_last_member = 1;
            } else {
                port_rinfo->is_last_member = 0;
            }
            tmp = (reg_val >> 28) & 0x01;
            if (tmp == 0x01) {
                port_rinfo->vc_valid = 1;
            } else {
                port_rinfo->vc_valid = 0;
            }
            port_rinfo->sq = (reg_val >> 0) & 0xff;
            port_rinfo->mfi = (reg_val >> 8) & 0xfff;
            break;

        case 0x15:
            port_rinfo->pre_gid = reg_val;
            break;

        case 0x16:
            port_rinfo->cur_gid = reg_val;
            break;

        case 0x17:
            port_rinfo->mfi_cnt = (reg_val >> 24) & 0xff;
            port_rinfo->sq_cnt = (reg_val >> 20) & 0xff;
            port_rinfo->sync_cnt = (reg_val >> 0) & 0xffff;
            break;

        case 0x18:
            slink->e1_sync = (reg_val & 0x01) >> 0;
            port_rinfo->e1_cnt = (reg_val & 0x7e) >> 1;
            slink->e1_sync_err = (reg_val & 0xff80) >> 7;

            slink->nfm_e1_sync = (reg_val & 0x10000) >> 16;
            port_rinfo->nfm_e1_cnt = (reg_val & 0x7e0000) >> 17;
            slink->nfm_e1_sync_err = (reg_val & 0xff800000) >> 23;
            break;

        default :
            OICERR("%d--%s: sel_info %d err.", __LINE__, __FUNCTION__,sel_info);
            mutex_unlock(&oicdev->lock);
            return -1;
    }
	mutex_unlock(&oicdev->lock);
    return 0;
}

#define GFP_GET_INFO_SELNUM       0x18
#define HDLC_TYPE_64K             0x00
#define HDLC_TYPE_2M              0x01
#define HDLC_SCAN_CONFIG_MODE     0x00
#define HDLC_SCAN_LOCAL_MODE      0x01
/* service hdlc and sub-service */
#define SVC_HDLC                  0x00
#define SVC_SUB_HDLC_LEN          0x00
#define SVC_SUB_HDLC_SCAN_MODE    0x02
#define SVC_SUB_HDLC_TYPE         0x03
static int get_hdlc_info(unsigned int port, struct fpga_board_runinfo_port_ex *port_rinfo)
{
	volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    volatile set_svc_t *ssvc = (volatile set_svc_t *)&fpga->set_svc;

    //64k
    BDREGCHK(ginfo->select_port, port);
    BDREGCHK(ssvc->task_sel, SVC_HDLC);
    BDREGCHK(ssvc->cfg_op, SVC_SUB_HDLC_TYPE);
    BDREGCHK(ssvc->cfg_data_0, HDLC_TYPE_64K);
    BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));

    BDREGCHK(ginfo->select_info, 0x10);
    port_rinfo->ch_64k_num = (ginfo->get_val_0 << 0) | (ginfo->get_val_1 << 8);
    BDREGCHK(ginfo->select_info, 0x11);
    port_rinfo->ch_64k_frames = (ginfo->get_val_0 << 0) | (ginfo->get_val_1 << 8) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 24);

    //2m
    BDREGCHK(ssvc->task_sel, SVC_HDLC);
    BDREGCHK(ssvc->cfg_op, SVC_SUB_HDLC_TYPE);
    BDREGCHK(ssvc->cfg_data_0, HDLC_TYPE_2M);
    BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));

    BDREGCHK(ginfo->select_info, 0x10);
    port_rinfo->ch_2m_num = (ginfo->get_val_0 << 0) | (ginfo->get_val_1 << 8);
    BDREGCHK(ginfo->select_info, 0x11);
    port_rinfo->ch_2m_frames = (ginfo->get_val_0 << 0) | (ginfo->get_val_1 << 8) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 24);

    return 0;
}

static int oic_fops_ioctl_get_sdh_au4_status(unsigned long arg)
{
    struct fpga_board_runinfo_port_ex pinfo;
    struct slink_info slink;
    int j = 0;
    int ret = -1;
    int regval = 0;

    memset(&slink, 0, sizeof(&slink));
    memset(&pinfo, 0, sizeof(&pinfo));
    if (copy_from_user(&pinfo, (void *)arg, sizeof(struct fpga_board_runinfo_port_ex)) != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    for (j = 0; j <= GFP_GET_INFO_SELNUM; j++) {
        get_info_rule(pinfo.fiber, 0, j, &regval);

        ret = analysis_sel_info(regval, j, &pinfo, &slink);
        if (ret != 0)
            return -1;
    }

    ret = copy_to_user((void __user *)arg, &pinfo, sizeof(struct fpga_board_runinfo_port_ex));
    if (ret != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

static int oic_fops_ioctl_get_fpga_bd_runinfo_ex(unsigned long arg)
{
	volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    unsigned int i = 0;
    unsigned int j = 0;
    int val = 0;
    unsigned int id = 0;
    struct fpga_board_runinfo_ex rinfo;
    struct slink_info slink;
    int ret = -1;

    memset(&slink, 0, sizeof(&slink));
    memset(&rinfo, 0, sizeof(struct fpga_board_runinfo_ex));
    if (copy_from_user(&rinfo, (void *)arg, sizeof(struct fpga_board_runinfo_ex)) != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    if (rinfo.clear) {
        BDREGCHK(ginfo->clear_bcode, 0x0f);
        mdelay(1);
        BDREGCHK(ginfo->clear_bcode, 0x00);
    }

    for (i = rinfo.start_port; i < (FPGA_OI_PORTNUM + rinfo.start_port); i++) {
        for (j = 0; j <= GFP_GET_INFO_SELNUM; j++) {
            ret = get_info_rule(i, 0, j, &val);
            if (ret != 0)
                return -1;

            ret = analysis_sel_info(val, j, &rinfo.ports[id], &slink);
            if (ret != 0)
                return -1;

        }
#if (!defined(XSCB_V2))
        //hdlc info
        //get_hdlc_info(i, &rinfo.ports[i]);
#endif
        id++;
    }

    ret = copy_to_user((void __user *)arg, &rinfo, sizeof(struct fpga_board_runinfo_ex));
    if (ret != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        goto err;
    }

	return 0;
err:
    return -1;
}

static int oic_fops_ioctl_get_fpga_slink_info(unsigned long arg)
{
    struct fpga_board_runinfo_ex rinfo;
    struct slink_info slink;
    int ret = -1;
    int port = 0;
    int channel = 0;
    int val = 0;
    unsigned int i = 0;

    memset(&slink, 0, sizeof(struct slink_info));
    memset(&rinfo, 0, sizeof(struct fpga_board_runinfo_ex));

	ret = copy_from_user(&slink, (void __user *)arg, sizeof(&slink));
    if (ret != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    port = slink.fiber;
    channel = slink.channel;

    for (i = 0; i <= GFP_GET_INFO_SELNUM; i++) {
        ret = get_info_rule(port, channel, i, &val);
        if (ret != 0)
            goto err;

        ret = analysis_sel_info(val, i, &rinfo.ports[0], &slink);
        if (ret != 0)
            goto err;
    }

    ret = copy_to_user((void __user *)arg, &slink, sizeof(struct slink_info));
    if (ret != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        goto err;
    }

	return 0;

err:
    return -1;
}

static int oic_fops_ioctl_set_fpga_slink_info_start(unsigned long arg)
{
	volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    
	mutex_lock(&oicdev->lock);
    BDREGCHK(ginfo->refresh_en, 0);
	mutex_unlock(&oicdev->lock);
    return 0;
}

static int oic_fops_ioctl_set_fpga_slink_info_end(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;

    mutex_lock(&oicdev->lock);
    BDREGCHK(ginfo->refresh_en, 1);
    mutex_unlock(&oicdev->lock);
    return 0;
}

#define SET_TRY_GROUP        0x01
#define SET_OK_GROUP         0x02
#define CHANNEL_RATE_C4      0x01 
#define CHANNEL_RATE_C12     0x02 
#define CHANNEL_RATE_C11     0x03 
/**********************************************************************
 *set_info_rule : set all group info to fpga.  
************************************************************************/
static int set_info_rule(struct sgroup_info *sg, int gsize, int port_group, int set_type)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_info_t *sinfo = (volatile set_info_t *)&fpga->set_info;
    int i = 0;
    int j = 0;
    int linksize = 0;
    unsigned int ram1_data = 0;
    unsigned int next_group_addr = 0;
    unsigned int cur_group_addr = 0;
    unsigned int link_addr = 0;
    unsigned char reg_val = 0;
    int group_maxnum = 0;
    int link_maxnum = 0;

    mutex_lock(&oicdev->lock);

#if defined(IN_X86)
    reg_val = 0;
#else
    reg_val = port_group & 0xff;
    BDREGCHK(sinfo->mif_cfg_ram_group, reg_val);
#endif
    /* set set or try */
    if (set_type == SET_TRY_GROUP) {
        if (sg[0].linkarrays->channel_rate == CHANNEL_RATE_C4) {
            reg_val = 0x03;
        } else if (sg[0].linkarrays->channel_rate == CHANNEL_RATE_C12) {
            reg_val = 0x01;
        } else {
            OICERR("%d--: unkown set_type %d", __LINE__, set_type);
            return -1;
        }
        group_maxnum = TRY_GROUP_MAXNUM;
        link_maxnum = TRY_LINK_MAXNUM;

    } else if (set_type == SET_OK_GROUP) {
        if (sg[0].linkarrays->channel_rate == CHANNEL_RATE_C4) {
            reg_val = 0x04;
        } else if (sg[0].linkarrays->channel_rate == CHANNEL_RATE_C12) {
            reg_val = 0x02;
        } else {
            OICERR("%d--: unkown set_type %d", __LINE__, set_type);
            return -1;
        }
        group_maxnum = SET_GROUP_MAXNUM;
        link_maxnum = SET_LINK_MAXNUM;

    } else {
        OICERR("%d--: unkown set_type %d", __LINE__, set_type);
        return -1;
    }
    BDREGCHK(sinfo->mif_cfg_op, reg_val);
    
    for (i = 0; i < gsize; i++) {
        if (sg[i].group_id >= group_maxnum) {
            OICERR("%d--: %s group %d[%d] id %d is err. group_maxnum %d", __LINE__, \
                   (set_type == SET_TRY_GROUP) ? "try" : "set", gsize, i, sg[i].group_id, group_maxnum);
            continue;
        }
        //group id

#if defined(IN_X86)
        if (sg[i].group_id > 255) {
            reg_val = (sg[i].group_id & 0xff00) >> 8;
            BDREGCHK(sinfo->mif_cfg_ram_addr1, reg_val);
        } else {
            BDREGCHK(sinfo->mif_cfg_ram_addr1, 0);
        } 
#endif
        reg_val = sg[i].group_id & 0xff;
        BDREGCHK(sinfo->mif_cfg_ram_addr, reg_val);

        /* select 1st ram */
        BDREGCHK(sinfo->mif_cfg_mix, 0x00);

        linksize = sg[i].linkarrays_size;
        if (linksize < 1 || linksize > link_maxnum) {
            OICERR("%d--: g[%d] linksize %d", __LINE__, i, linksize);
            continue;
        }

        ram1_data = 0;
#if defined(IN_X86)
        reg_val = 0;
        reg_val |= (sg[i].linkarrays->fiber_rate & 0x0f) << 6;
        reg_val |= (sg[i].linkarrays->channel_rate & 0x03) << 1;
        reg_val |= (sg[i].is_valid & 0x01);
        BDREGCHK(sinfo->mif_cfg_ram_data_4, reg_val);

        reg_val = 0;
        reg_val = (linksize & 0xff);
        BDREGCHK(sinfo->mif_cfg_ram_data_3, reg_val);

        reg_val = 0;
        cur_group_addr = next_group_addr;
        reg_val = (cur_group_addr & 0xff0) >> 4;
        BDREGCHK(sinfo->mif_cfg_ram_data_2, reg_val);

        reg_val = 0;
        reg_val |= (cur_group_addr & 0x00f) << 4;
        reg_val |= (sg[i].linkarrays->fiber & 0x3c) >> 2;
        BDREGCHK(sinfo->mif_cfg_ram_data_1, reg_val);

        reg_val = 0;
        reg_val = (sg[i].linkarrays->fiber & 0x03) << 6;
        reg_val |= (sg[i].linkarrays->channel & 0x3f) << 0;
        BDREGCHK(sinfo->mif_cfg_ram_data_0, reg_val);

#else
        ram1_data |= (sg[i].linkarrays->fiber_rate & 0x0f) << 27;
        ram1_data |= (sg[i].linkarrays->channel_rate & 0x03) << 25;
        ram1_data |= (sg[i].is_valid << 24);
        ram1_data |= (linksize & 0xff) << 16;
        cur_group_addr = next_group_addr;
        ram1_data |= (cur_group_addr & 0xff) << 8;
        ram1_data |= (sg[i].linkarrays->fiber & 0x03) << 6;
        ram1_data |= (sg[i].linkarrays->channel & 0x3f) << 0;

        reg_val = (ram1_data >> 24) & 0xff;
        BDREGCHK(sinfo->mif_cfg_ram_data_3, reg_val);
        reg_val = (ram1_data >> 16) & 0xff;
        BDREGCHK(sinfo->mif_cfg_ram_data_2, reg_val);
        reg_val = (ram1_data >> 8) & 0xff;
        BDREGCHK(sinfo->mif_cfg_ram_data_1, reg_val);
        reg_val = (ram1_data >> 0) & 0xff;
        BDREGCHK(sinfo->mif_cfg_ram_data_0, reg_val);

#endif
        
        /*1st ram end*/
        BDREGCHK(sinfo->mif_cfg_mix, 0x00);
        BDREGCHK(sinfo->mif_cfg_mix, 0x02);

        /*2nd ram*/
        BDREGCHK(sinfo->mif_cfg_mix, 0x04);

        for(j = 0; j < linksize; j++) {
            reg_val = 0;
            link_addr = cur_group_addr + j;
#if defined(IN_X86)
            if (link_addr > 255) {
                reg_val = (link_addr >> 8) & 0xff;
                BDREGCHK(sinfo->mif_cfg_ram_addr1, reg_val);
            } else {
                BDREGCHK(sinfo->mif_cfg_ram_addr1, 0);
            }
#endif
            reg_val = link_addr & 0xff;
            BDREGCHK(sinfo->mif_cfg_ram_addr, reg_val);

#if defined(IN_X86)
            reg_val = 0;
            reg_val = (sg[i].linkarrays[j].fiber & 0x3c) >> 2;
            BDREGCHK(sinfo->mif_cfg_ram_data_1, reg_val);
#endif
            reg_val = (sg[i].linkarrays[j].fiber & 0x03) << 6;
            reg_val |= (sg[i].linkarrays[j].channel & 0x3f) << 0;
            BDREGCHK(sinfo->mif_cfg_ram_data_0, reg_val);
            BDREGCHK(sinfo->mif_cfg_mix, 0x04);
            BDREGCHK(sinfo->mif_cfg_mix, 0x06);
        }

        next_group_addr = cur_group_addr + linksize;
    }
    /*1st,2nd end*/
    BDREGCHK(sinfo->mif_cfg_mix, 0x00);
    BDREGCHK(sinfo->mif_cfg_mix, 0x01);

    mutex_unlock(&oicdev->lock);
    
    return 0;
}

static int oic_fops_ioctl_set_fpga_group(unsigned long arg)
{
    struct sgroup_all_info *sg_all = NULL;
    int ret = -1;
    unsigned int gsize = 0;

    OICDBG("%d--%s", __LINE__, __FUNCTION__);
    sg_all = (struct sgroup_all_info *)kmalloc(KMALLOC_SIZE, GFP_KERNEL);
    if (sg_all == NULL) {
        OICERR("%d--%s: kmalloc size %d failed.", __LINE__, __FUNCTION__, KMALLOC_SIZE); 
        return -1;
    }
    memset(sg_all, 0, KMALLOC_SIZE);
	ret = copy_from_user(sg_all, (void __user *)arg, KMALLOC_SIZE);
    if (ret != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);    
        goto err;
    }

    gsize = sg_all->gsize;
    if (gsize < 1) {
        OICERR("%d--%s: group size %d err.", __LINE__, __FUNCTION__, gsize); 
        goto err;
    }
    ret = set_info_rule(sg_all->ginfo, sg_all->gsize, sg_all->port_group, SET_OK_GROUP);
    if (ret != 0)
        goto err;

    kfree(sg_all);
    return 0;

err:
    kfree(sg_all);
    return -1;
}

static int oic_fops_ioctl_get_fpga_try_result(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile scan_t *scan = (volatile scan_t *)&fpga->scan;
    struct sgroup_all_info *sg_all = NULL;
    unsigned int i = 0;
    int ret = -1;
    unsigned char regval = 0;

    sg_all = (struct sgroup_all_info *)kmalloc(KMALLOC_SIZE, GFP_KERNEL);
    if (sg_all == NULL) {
        OICERR("%d--%s: kmalloc failed.", __LINE__, __FUNCTION__); 
        return -1;
    }

    memset(sg_all, 0, KMALLOC_SIZE);
	ret = copy_from_user(sg_all, (void __user *)arg, KMALLOC_SIZE);
    if (ret != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);    
        goto err;
    }

    if (scan->scan_status == 0x01) {
        /* get scan result */
        mutex_lock(&oicdev->lock);
        for (i = 0; i < sg_all->gsize; i++) {
#if defined(IN_X86)
            if (sg_all->ginfo[i].group_id > 255) {
                regval = (sg_all->ginfo[i].group_id & 0xff00) >> 4;
                BDREGCHK(scan->group_id1, regval);
            }
#endif
            regval = (sg_all->ginfo[i].group_id & 0xff) >> 0;
            BDREGCHK(scan->group_id, regval);
            sg_all->ginfo[i].reserrpkt = (scan->scan_res >> 4) & 0x0f;
            sg_all->ginfo[i].resokpkt = (scan->scan_res >> 0) & 0x0f;
        }
        mutex_unlock(&oicdev->lock);
        ret = copy_to_user((void __user *)arg, sg_all, (sizeof(struct sgroup_info) * sg_all->gsize));
        if (ret != 0) {
            OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__); 
            goto err;
        }
    } else {
        kfree(sg_all);
        return TRY_NO_RESULT;
    }
    
    kfree(sg_all);
    return 0;
err:
    kfree(sg_all);
    return -1;
}

static int oic_fops_ioctl_set_fpga_try_group(unsigned long arg)
{
    struct sgroup_all_info *sg_all = NULL;
    unsigned int gsize = 0;
    int ret = -1;

    sg_all = (struct sgroup_all_info *)kmalloc(KMALLOC_SIZE, GFP_KERNEL);
    if (sg_all == NULL) {
        OICERR("%d--%s: kmalloc failed.", __LINE__, __FUNCTION__); 
        return -1;
    }
    memset(sg_all, 0, KMALLOC_SIZE);
	ret = copy_from_user(sg_all, (void __user *)arg, KMALLOC_SIZE);
    if (ret != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);    
        goto err;
    }

    gsize = sg_all->gsize;
    if (gsize < 1) {
        OICERR("%d--%s: group size %d err.", __LINE__, __FUNCTION__, gsize); 
        goto err;
    }

    ret = set_info_rule(sg_all->ginfo, gsize, sg_all->port_group, SET_TRY_GROUP);
    if (ret != 0)
        goto err;

    kfree(sg_all);
	return 0;

err:
    kfree(sg_all);
    return -1;
}

static int oic_fops_ioctl_get_fpga_try_group(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_info_t *sinfo = (volatile set_info_t *)&fpga->set_info;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    int i = 0;
    int ret = -1;
    struct ram_info rinfo;

    memset(&rinfo, 0, sizeof(struct ram_info));
    BDREGCHK(sinfo->mif_cfg_op, 0x80);
    for(i = 0; i < 256; i++) {
        BDREGCHK(sinfo->mif_cfg_ram_addr, i);
        BDREGCHK(ginfo->select_info, 0x20);
        rinfo.ram1[i] = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);
        BDREGCHK(ginfo->select_info, 0x23);
        rinfo.ram2[i] = (ginfo->get_val_2 << 0);
    }

    ret = copy_to_user((void __user *)arg, &rinfo, sizeof(struct ram_info));
    if (ret != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

static int oic_fops_ioctl_get_fpga_group(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_info_t *sinfo = (volatile set_info_t *)&fpga->set_info;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    int i = 0;
    int ret = -1;
    struct ram_info rinfo;

    memset(&rinfo, 0, sizeof(struct ram_info));
    BDREGCHK(sinfo->mif_cfg_op, 0x80);
    for(i = 0; i < 256; i++) {
        BDREGCHK(sinfo->mif_cfg_ram_addr, i);

        BDREGCHK(ginfo->select_info, 0x21);
        rinfo.ram1_c4[i] = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);

        BDREGCHK(ginfo->select_info, 0x22);
        rinfo.ram1_c12[i] = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);

        BDREGCHK(ginfo->select_info, 0x23);
        rinfo.ram2_c4[i] = (ginfo->get_val_1 << 0) & 0xff;
        rinfo.ram2_c12[i] = (ginfo->get_val_0 << 0) & 0xff;
    }

    ret = copy_to_user((void __user *)arg, &rinfo, sizeof(struct ram_info));
    if (ret != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }
    return 0;
}

static int oic_fops_ioctl_set_fpga_hdlc_cfg(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_svc_t *ssvc = (volatile set_svc_t *)&fpga->set_svc;
    struct hdlc_cfg hcfg;
    char rval = 0x00;

    memset(&hcfg, 0, sizeof(&hcfg));
    if (copy_from_user(&hcfg, (void *)arg, sizeof(struct hdlc_cfg))) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    if (HDLC_CFG_LEN == (hcfg.mask & HDLC_CFG_LEN)) {
        BDREGCHK(ssvc->task_sel, SVC_HDLC);
        BDREGCHK(ssvc->cfg_op, SVC_SUB_HDLC_LEN);
        rval = (hcfg.len_max >> 0) & 0xff; 
        BDREGCHK(ssvc->cfg_data_0, rval);
        rval = (hcfg.len_max >> 8) & 0xff;
        BDREGCHK(ssvc->cfg_data_1, rval);

        rval = (hcfg.len_min >> 0) & 0xff;
        BDREGCHK(ssvc->cfg_data_2, rval);
        rval = (hcfg.len_min >> 8) & 0xff;
        BDREGCHK(ssvc->cfg_data_3, rval);
        BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));
    } 

    if (HDLC_CFG_SCAN_MODE == (hcfg.mask & HDLC_CFG_SCAN_MODE)) {
        BDREGCHK(ssvc->task_sel, SVC_HDLC);
        BDREGCHK(ssvc->cfg_op, SVC_SUB_HDLC_SCAN_MODE);
        BDREGCHK(ssvc->cfg_data_0, (hcfg.scan_mode - 1));
        BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));
    }

    return 0;
}
static int oic_fops_ioctl_get_fpga_hdlc_ge_stat(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_ge_stat_t *gge = (volatile get_ge_stat_t *)&fpga->get_gstat;
    volatile get_info_t *ginfo = (volatile get_info_t *)&fpga->get_info;
    volatile set_clr_t *sclr = (volatile set_clr_t *)&fpga->set_clr;
    struct bd_ge_stat bd_gstat;
    int i = 0;

    memset(&bd_gstat, 0, sizeof(struct bd_ge_stat));
    if (copy_from_user(&bd_gstat, (void *)arg, sizeof(struct bd_ge_stat))) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    if (bd_gstat.clear) {
        BDREGCHK(sclr->mis_cnt_clr, 0x00);
        mdelay(1);
        BDREGCHK(sclr->mis_cnt_clr, 0x01);
    }

    for (i = 0; i < GE_NUM_MAX; i++) {
        bd_gstat.gstat[i].ge = (i + 1);
        BDREGCHK(gge->ge, i);
        BDREGCHK(ginfo->select_info, 0x12);
        bd_gstat.gstat[i].tran = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);
        BDREGCHK(ginfo->select_info, 0x13);
        bd_gstat.gstat[i].tran_clr = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);
        BDREGCHK(ginfo->select_info, 0x14);
        bd_gstat.gstat[i].hdlc = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);
        BDREGCHK(ginfo->select_info, 0x15);
        bd_gstat.gstat[i].hdlc_clr = (ginfo->get_val_3 << 24) | (ginfo->get_val_2 << 16) | (ginfo->get_val_1 << 8) | (ginfo->get_val_0 << 0);
    }

    return (copy_to_user((void __user *)arg, &bd_gstat, sizeof(struct bd_ge_stat)));
}

static int oic_fops_ioctl_get_fpga_hdlc_chstat(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile hdlc_scan_t *hdlc_scan = (volatile hdlc_scan_t *)&fpga->hdlc_scan;
    struct hdlc_port_stat *p_ch_stat = NULL;
    unsigned char rval = 0;
    int i = 0;
    int chan_cnt = 0;

    p_ch_stat = (struct hdlc_port_stat *)kmalloc(sizeof(struct hdlc_port_stat), GFP_KERNEL);
    if (p_ch_stat == NULL) {
        OICERR("%d--%s: kmalloc failed.", __LINE__, __FUNCTION__);
        return -1;
    }
    memset(p_ch_stat, 0, sizeof(struct hdlc_port_stat));

    mutex_lock(&oicdev->lock);
    for(i = 0; i < CHANNEL_NUM_MAX; i++) {
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x04 | 0x80));
        if (i > 255) {
            rval = (i >> 8) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, rval);
            rval = (i >> 0) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, rval);
        } else {
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, 0x00);
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, i);
        }
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x04 | 0x80 | 0x40));
        if ((hdlc_scan->mif_cfg_data_5 & 0x04) != 0x04) {
            continue;
        }

        p_ch_stat->chstat[chan_cnt].type = hdlc_scan->mif_cfg_data_5 & 0x03;
        p_ch_stat->chstat[chan_cnt].port = hdlc_scan->mif_cfg_data_4;
        p_ch_stat->chstat[chan_cnt].e1 = hdlc_scan->mif_cfg_data_3;
        p_ch_stat->chstat[chan_cnt].ts = hdlc_scan->mif_cfg_data_2;
        p_ch_stat->chstat[chan_cnt].ok_pkts = (hdlc_scan->mif_cfg_data_1 << 8) | (hdlc_scan->mif_cfg_data_0 << 0);
        ++chan_cnt;
    }
    p_ch_stat->num_chstats = chan_cnt;
    mutex_unlock(&oicdev->lock);

    if (copy_to_user((void __user *)arg, p_ch_stat, sizeof(struct hdlc_port_stat)) != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        kfree(p_ch_stat);
        return -1;
    }
    kfree(p_ch_stat);
    return 0;
}

static int oic_fops_ioctl_get_fpga_hdlc_local_schan(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_svc_t *ssvc = (volatile set_svc_t *)&fpga->set_svc;
    volatile hdlc_scan_t *hdlc_scan = (volatile hdlc_scan_t *)&fpga->hdlc_scan;
    struct hdlc_port_stat *p_ch_stat = NULL;
    unsigned char rval = 0;
    int i = 0;
    int chan_cnt = 0;

    p_ch_stat = (struct hdlc_port_stat *)kmalloc(sizeof(struct hdlc_port_stat), GFP_KERNEL);
    if (p_ch_stat == NULL) {
        OICERR("%d--%s: kmalloc failed.", __LINE__, __FUNCTION__);
        return -1;
    }
    memset(p_ch_stat, 0, sizeof(struct hdlc_port_stat));

    mutex_lock(&oicdev->lock);
    BDREGCHK(ssvc->task_sel, SVC_HDLC);
    BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));
    for(i = 0; i < CHANNEL_NUM_MAX; i++) {
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x01 | 0x80));
        if (i > 255) {
            rval = (i >> 8) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, rval);
            rval = (i >> 0) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, rval);
        } else {
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, 0x00);
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, i);
        }
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x01 | 0x80 | 0x40));
        if ((hdlc_scan->mif_cfg_data_2 & 0x80) != 0x80)
            continue;

        p_ch_stat->chstat[chan_cnt].match_num = ((hdlc_scan->mif_cfg_data_4 & 0x3f) << 8) | hdlc_scan->mif_cfg_data_3 ;
        p_ch_stat->chstat[chan_cnt].type = (hdlc_scan->mif_cfg_data_2 & 0x60) >> 5;
        p_ch_stat->chstat[chan_cnt].port = hdlc_scan->mif_cfg_data_2 & 0x1f;
        p_ch_stat->chstat[chan_cnt].e1 = hdlc_scan->mif_cfg_data_1 & 0x3f;
        p_ch_stat->chstat[chan_cnt].ts = hdlc_scan->mif_cfg_data_0 & 0x1f;
        ++chan_cnt;
    }
    p_ch_stat->num_chstats = chan_cnt;
    mutex_unlock(&oicdev->lock);

    if (copy_to_user((void __user *)arg, p_ch_stat, sizeof(struct hdlc_port_stat)) != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        kfree(p_ch_stat);
        return -1;
    }
    kfree(p_ch_stat);
    return 0;
}

static int oic_fops_ioctl_get_fpga_hdlc_config_schan(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_svc_t *ssvc = (volatile set_svc_t *)&fpga->set_svc;
    volatile hdlc_scan_t *hdlc_scan = (volatile hdlc_scan_t *)&fpga->hdlc_scan;
    struct hdlc_port_stat *p_ch_stat = NULL;
    unsigned char rval = 0;
    int i = 0;
    int chan_cnt = 0;

    p_ch_stat = (struct hdlc_port_stat *)kmalloc(sizeof(struct hdlc_port_stat), GFP_KERNEL);
    if (p_ch_stat == NULL) {
        OICERR("%d--%s: kmalloc failed.", __LINE__, __FUNCTION__);
        return -1;
    }
    memset(p_ch_stat, 0, sizeof(struct hdlc_port_stat));

    mutex_lock(&oicdev->lock);
    BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));
    for(i = 0; i < 2048; i++) {
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x02 | 0x80));
        if (i > 255) {
            rval = (i >> 8) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, rval);
            rval = (i >> 0) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, rval);
        } else {
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, 0x00);
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, i);
        }
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x02 | 0x80 | 0x40));
        if ((hdlc_scan->mif_cfg_data_2 & 0x80) != 0x80) {
            continue;
        }

        p_ch_stat->chstat[chan_cnt].match_num = ((hdlc_scan->mif_cfg_data_4 & 0x3f) << 8) | hdlc_scan->mif_cfg_data_3;
        p_ch_stat->chstat[chan_cnt].type = (hdlc_scan->mif_cfg_data_2 & 0x60) >> 5;
        p_ch_stat->chstat[chan_cnt].port = hdlc_scan->mif_cfg_data_2 & 0x1f;
        p_ch_stat->chstat[chan_cnt].e1 = hdlc_scan->mif_cfg_data_1 & 0x3f;
        p_ch_stat->chstat[chan_cnt].ts = hdlc_scan->mif_cfg_data_0 & 0x1f;
        ++chan_cnt;
    }
    p_ch_stat->num_chstats = chan_cnt;
    mutex_unlock(&oicdev->lock);

    if (copy_to_user((void __user *)arg, p_ch_stat, sizeof(struct hdlc_port_stat)) != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        kfree(p_ch_stat);
        return -1;
    }
    kfree(p_ch_stat);
    return 0;
}

static int oic_fops_ioctl_get_fpga_hdlc_db_status(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile set_svc_t *ssvc = (volatile set_svc_t *)&fpga->set_svc;
    volatile hdlc_scan_t *hdlc_scan = (volatile hdlc_scan_t *)&fpga->hdlc_scan;
    struct hdlc_port_stat *p_ch_stat = NULL;
    unsigned char rval = 0;
    int i = 0;
    int chan_cnt = 0;

    p_ch_stat = (struct hdlc_port_stat *)kmalloc(sizeof(struct hdlc_port_stat), GFP_KERNEL);
    if (p_ch_stat == NULL) {
        OICERR("%d--%s: kmalloc failed.", __LINE__, __FUNCTION__);
        return -1;
    }
    memset(p_ch_stat, 0, sizeof(struct hdlc_port_stat));

    mutex_lock(&oicdev->lock);
    BDREGCHK(ssvc->task_sel, (SVC_HDLC | 0x80));
    for(i = 0; i < 8192; i++) {
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x05 | 0x80));
        if (i > 255) {
            rval = (i >> 8) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, rval);
            rval = (i >> 0) & 0xff;
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, rval);
        } else {
            BDREGCHK(hdlc_scan->mif_cfg_addr_h, 0x00);
            BDREGCHK(hdlc_scan->mif_cfg_addr_l, i);
        }
        BDREGCHK(hdlc_scan->mif_cfg_op, (0x05 | 0x80 | 0x40));
        if ((hdlc_scan->mif_cfg_data_5 & 0x80) != 0x80)
            continue;

        p_ch_stat->chstat[chan_cnt].type = (hdlc_scan->mif_cfg_data_5 & 0x60) >> 5;
        p_ch_stat->chstat[chan_cnt].port = hdlc_scan->mif_cfg_data_5 & 0x1f;
        p_ch_stat->chstat[chan_cnt].e1 = hdlc_scan->mif_cfg_data_4 & 0xff;
        p_ch_stat->chstat[chan_cnt].ts = hdlc_scan->mif_cfg_data_3 & 0xff;
        p_ch_stat->chstat[chan_cnt].ok_pkts = (hdlc_scan->mif_cfg_data_2 & 0xf0) >> 4;
        p_ch_stat->chstat[chan_cnt].n_7e = (hdlc_scan->mif_cfg_data_2 & 0x0f);
        p_ch_stat->chstat[chan_cnt].err_pkts = ((hdlc_scan->mif_cfg_data_1 & 0xff) << 8) | (hdlc_scan->mif_cfg_data_0 & 0xff);
        ++chan_cnt;
    }
    p_ch_stat->num_chstats = chan_cnt;
    mutex_unlock(&oicdev->lock);

    if (copy_to_user((void __user *)arg, p_ch_stat, sizeof(struct hdlc_port_stat)) != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        kfree(p_ch_stat);
        return -1;
    }
    kfree(p_ch_stat);
    return 0;
}

static int oic_fops_ioctl_set_gfp_fpga_spu_selnum(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    unsigned char val = 0;

    if (get_user(val, (unsigned char *)arg))
        return -EFAULT;

    mutex_lock(&oicdev->lock);
    BDREGCHK(fpga->selnum, val);
    mutex_unlock(&oicdev->lock);

    return 0;
}

static int oic_fops_ioctl_get_gfp_fpga_spu_selnum(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    unsigned char val = 0;

    mutex_lock(&oicdev->lock);
    val = fpga->selnum;
    mutex_unlock(&oicdev->lock);

    if(put_user(val, (unsigned char *)arg))
        return -EFAULT;

    return 0;
}

static int oic_fops_ioctl_set_gfp_fpga_spu_forward_enable(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;

    mutex_lock(&oicdev->lock);
    BDREGCHK(fpga->forward_en, 1);
    mutex_unlock(&oicdev->lock);

    return 0;
}

static int oic_fops_ioctl_set_gfp_fpga_spu_forward_disable(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;

    mutex_lock(&oicdev->lock);
    BDREGCHK(fpga->forward_en, 0);
    mutex_unlock(&oicdev->lock);

    return 0;
}

static int oic_fops_ioctl_set_gfp_fpga_spu_forward_rule(unsigned long arg)
{
    struct spu_forward_rule ver;
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile rule_t *rule = (volatile rule_t *)&fpga->rule;
    unsigned char value = 0;

    if (copy_from_user(&ver,(void __user *)arg , sizeof(ver)))
        return -EFAULT;

    mutex_lock(&oicdev->lock);
    value = (ver.src_port<<6) | (ver.src_channel&0x3f);   
    BDREGCHK(rule->mif_list_cfg_ch, value);
    BDREGCHK(rule->mif_list_cfg_info_h, ver.dst_port);
    BDREGCHK(rule->mif_list_cfg_info_l, ver.dst_selnum);

    BDREGCHK(rule->mif_list_cfg_en, 0);
    BDREGCHK(rule->mif_list_cfg_en, 1);

    mutex_unlock(&oicdev->lock);

    return 0;
}

static int oic_fops_ioctl_set_gfp_fpga_spu_channel_forward(unsigned long arg)
{
    struct chan_flag ver;

    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile chan_forward_t *chan = (volatile chan_forward_t *)&fpga->chan;

    if (copy_from_user(&ver,(void __user *)arg , sizeof(ver)))
        return -EFAULT;

    if((1 <= ver.chan) && (ver.chan <= 32)){
        mutex_lock(&oicdev->lock);
        BDREGCHK(chan->reg_sel, 0);
        if(0 == ver.flag){
            g_channel1 = ~(1 << (ver.chan - 1)) & g_channel1;
            BDREGCHK(chan->reg_val[0], (g_channel1 >> 24) & 0xff);
            BDREGCHK(chan->reg_val[1], (g_channel1 >> 16) & 0xff);
            BDREGCHK(chan->reg_val[2], (g_channel1 >> 8) & 0xff);
            BDREGCHK(chan->reg_val[3], g_channel1 & 0xff);
        }
        else{
            g_channel1 = (1 << (ver.chan - 1)) | g_channel1;
            BDREGCHK(chan->reg_val[0], (g_channel1 >> 24) & 0xff);
            BDREGCHK(chan->reg_val[1], (g_channel1 >> 16) & 0xff);
            BDREGCHK(chan->reg_val[2], (g_channel1 >> 8) & 0xff);
            BDREGCHK(chan->reg_val[3], g_channel1 & 0xff);
        }
        BDREGCHK(chan->reg_cfg_en, 0);
        BDREGCHK(chan->reg_cfg_en, 1);
        mutex_unlock(&oicdev->lock);
    }
    if((33 <= ver.chan) && (ver.chan <= 64)){
        mutex_lock(&oicdev->lock);
        BDREGCHK(chan->reg_sel, 1);
        if(0 == ver.flag){
            g_channel2 = ~(1 << (ver.chan - 33)) & g_channel2;
            BDREGCHK(chan->reg_val[0],(g_channel2 >> 24) & 0xff);
            BDREGCHK(chan->reg_val[1],(g_channel2 >> 16) & 0xff);
            BDREGCHK(chan->reg_val[2],(g_channel2 >> 8) & 0xff);
            BDREGCHK(chan->reg_val[3],g_channel2 & 0xff);
        }
        else{
            g_channel2 = (1 << (ver.chan - 33)) | g_channel2;
            BDREGCHK(chan->reg_val[0], (g_channel2 >> 24) & 0xff);
            BDREGCHK(chan->reg_val[1], (g_channel2 >> 16) & 0xff);
            BDREGCHK(chan->reg_val[2], (g_channel2 >> 8) & 0xff);
            BDREGCHK(chan->reg_val[3], g_channel2 & 0xff);
        }
        BDREGCHK(chan->reg_cfg_en, 0x00);
        BDREGCHK(chan->reg_cfg_en, 0x01);
        mutex_unlock(&oicdev->lock);
    }
    return 0;
}

int sdh_cfg_channle_en(uint8_t port, uint8_t channel, uint8_t e1, uint8_t en)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile ch_en_t  *chen = (volatile ch_en_t *)&fpga->ch_en;

    mutex_lock(&oicdev->lock);
    if (oicdev->sdhtype == FPGA_VERSION_LP) {
        BDREGCHK(chen->sw_ch_sel, (channel & 0x0f));
        BDREGCHK(chen->sw_e1_sel, e1);
        BDREGCHK(chen->sw_ch_en, en);
        BDREGCHK(chen->sw_cfg_en, 0);
        BDREGCHK(chen->sw_cfg_en, 1);
    } else if (oicdev->sdhtype == FPGA_VERSION_HP) {
        BDREGCHK(chen->sw_ch_sel, ((port & 0x03) << 6) | (channel & 0x3f));
        BDREGCHK(chen->sw_e1_sel, 0);
        BDREGCHK(chen->sw_ch_en, en);
        BDREGCHK(chen->sw_cfg_en, 0);
        BDREGCHK(chen->sw_cfg_en, 1);
    }
    else {
        OICERR("Unkown fpga type LP or HP.");
        mutex_unlock(&oicdev->lock);
        return -1;
    }
    mutex_unlock(&oicdev->lock);
    return 0;
}

int sdh_cfg_rule(uint8_t list_type, uint8_t chan, uint8_t sel_addr, uint8_t sel, uint8_t ratio)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile set_list_t *s_list = (volatile set_list_t *)&fpga->set_list;


    mutex_lock(&oicdev->lock);
    BDREGCHK(s_list->list_cfg_type, list_type);
    BDREGCHK(s_list->list_cfg_ch, chan);

    if (list_type != LIST_TYPE_HP_TO_LP_AU4) {
        BDREGCHK(s_list->list_cfg_ext_reg,sel_addr);
        BDREGCHK(s_list->list_cfg_info_l,sel);
        BDREGCHK(s_list->list_cfg_info_h,ratio);
    } else {
        BDREGCHK(s_list->list_cfg_ext_reg,0);
        BDREGCHK(s_list->list_cfg_info_l,sel);
        BDREGCHK(s_list->list_cfg_info_h,0);
    }

    BDREGCHK(s_list->list_cfg_en,0);
    BDREGCHK(s_list->list_cfg_en,1);
    mutex_unlock(&oicdev->lock);
    return 0;
}

static int oic_fops_ioctl_set_sdh_local_rule(unsigned long arg)
{
    struct sdh_local_rule rule;
    uint8_t list_type = 0;
    uint8_t i;
    int ret = -1;

    memset(&rule, 0, sizeof(struct sdh_local_rule));
    if (copy_from_user(&rule, (void __user *)arg, sizeof(struct sdh_local_rule)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }

    if (oicdev->sdhtype == FPGA_VERSION_LP) {
        list_type = LIST_TYPE_LP_LOCAL;
    } else if (oicdev->sdhtype == FPGA_VERSION_HP) {
        list_type = LIST_TYPE_HP_LOCAL;
    } else {
        OICERR("Unkown fpga type LP or HP.");
        return -1;
    }
    for(i = 0; i < SEL_LIST_NUM; i++) {
        if(i < rule.valid_num) {
            ret = sdh_cfg_rule(list_type, rule.proto, i, rule.sel[i], 1);
            if (ret != 0)
                goto err_exit;

        } else {
            ret = sdh_cfg_rule(list_type, rule.proto, i, 0, 0);
            if (ret != 0)
                goto err_exit;

        }
    }

    return 0;
err_exit:
    return ret;
}

int oic_fops_ioctl_set_sdh_global_rule(unsigned long arg)
{
    struct sdh_global_rule rule;
    uint8_t i = 0;
    uint8_t list_type = 0;
    int ret = -1;

    memset(&rule, 0, sizeof(struct sdh_global_rule));

    if (copy_from_user(&rule, (void __user *)arg, sizeof(struct sdh_global_rule)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }

    OICDBG("%s proto %d valid_num %d sel %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", 
            __FUNCTION__, rule.proto, rule.valid_num, rule.sel[0], rule.sel[1], rule.sel[2], \
            rule.sel[3], rule.sel[4], rule.sel[5], rule.sel[6], rule.sel[7] , \
            rule.sel[8], rule.sel[9], rule.sel[10], rule.sel[11], rule.sel[12], \
            rule.sel[13], rule.sel[14], rule.sel[15]);

    if (oicdev->sdhtype == FPGA_VERSION_LP) {
        list_type = LIST_TYPE_LP_GLOBLA;
    } else if (oicdev->sdhtype == FPGA_VERSION_HP) {
        list_type = LIST_TYPE_HP_GLOBLA;
    } else {
        OICERR("Unkown fpga type LP or HP.");
        return -1;
    }

    for(i = 0; i < SEL_LIST_NUM; i++) {
        if(i < rule.valid_num) {
            ret = sdh_cfg_rule(list_type, rule.proto, i, rule.sel[i], 1);
            if (ret != 0)
                goto err_exit;

        } else {
            ret = sdh_cfg_rule(list_type, rule.proto, i, 0, 0);
            if (ret != 0)
                goto err_exit;

        }
    }

    return 0;
err_exit:
    return -1;
}

int oic_fops_ioctl_set_sdh_inner_rule(unsigned long arg)
{
    struct sdh_inner_rule rule;
    uint8_t channel = 0;
    int ret = -1;

    if (oicdev->sdhtype == FPGA_VERSION_LP) {
        OICERR("Fpga type is LP, set inner rule faield.");
        return -1;
    }
    if (oicdev->sdhtype != FPGA_VERSION_HP) {
        OICERR("Fpga type is unkown, set inner rule faield.");
        return -1;
    }
    memset(&rule, 0, sizeof(struct sdh_inner_rule));
    if (copy_from_user(&rule, (void __user *)arg, sizeof(struct sdh_inner_rule)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }
    OICDBG("inner rule: src %d  %d, dst %d %d, enable %d", rule.src_port, rule.src_channel, rule.dst_sel, rule.dst_channel, rule.enable);

    //disable srcport+srcau4
    if (sdh_cfg_channle_en(rule.src_port, rule.src_channel, 0, 0) != 0) {
        OICERR("Failed to set sdh cfg channel enable.");
        return -1;
    }
    if (rule.enable == 0) {
        return 0;
    }

    //set LIST_TYPE_HP_TO_LP_SEL srcport+srcau4 -- dstsel
    channel = ((rule.src_port & 0x3) << 6) | (rule.src_channel & 0x3f);
    ret = sdh_cfg_rule(LIST_TYPE_HP_TO_LP_SEL, channel, 0, rule.dst_sel, 1);
    if (ret != 0)
        goto err_exit;

    //set LIST_TYPE_HP_TO_LP_AU4 srcport+srcau4 -- dstau4
    ret = sdh_cfg_rule(LIST_TYPE_HP_TO_LP_AU4, channel, 0, rule.dst_channel, 0);
    if (ret != 0)
        goto err_exit;

    //enable srcport+srcau4
    ret = sdh_cfg_channle_en(rule.src_port, rule.src_channel, 0, 1);
    if (ret != 0)
        goto err_exit;

    return 0;

err_exit:
    return -1;
}

int oic_fops_ioctl_set_sdh_sel(unsigned long arg)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    struct sdh_sel s_sel;

    memset(&s_sel, 0, sizeof(struct sdh_sel));
    if (copy_from_user(&s_sel, (void __user *)arg, sizeof(struct sdh_sel)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }
	mutex_lock(&oicdev->lock);
    //fpga->selnum = s_sel.sel;
    BDREGCHK(fpga->selnum, s_sel.sel);
	mutex_unlock(&oicdev->lock);

    return 0;
}

int oic_fops_ioctl_set_sdh_e1user(unsigned long arg)
{
    struct sdh_e1user_map map;
    int ret = -1;

    memset(&map, 0, sizeof(struct sdh_e1user_map));
    if (copy_from_user(&map, (void __user *)arg, sizeof(struct sdh_e1user_map)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }
    OICDBG("%s : au4 %d e1 %d user %d-%d-%d-%d", __FUNCTION__, map.au4, map.e1,\
            map.user_chassisid, map.user_slot, map.user_port, map.user_e1);

    ret = sdh_cfg_rule(LIST_TYPE_LP_E1USER, map.au4, map.e1, map.user_e1, map.user_port);
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_rule(LIST_TYPE_LP_DEVMAP, map.au4, map.e1, map.user_slot, map.user_chassisid);
    if (ret != 0)
        goto err_exit;

    if (map.rule == E1_RULE_LOCAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_LOCAL_E1USER, map.au4, map.e1, map.sel, map.enable);
    } else if (map.rule == E1_RULE_GLOBAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_GLOBLA_E1USER, map.au4, map.e1, map.sel, map.enable);
    } else {
        OICERR("Unkown e1 rule .");
        ret = -1;
    }
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_channle_en(0, map.au4, map.e1, map.enable);
    if (ret != 0)
        goto err_exit;

    return 0;

err_exit:
    return -1;
}

int oic_fops_ioctl_set_sdh_e1linkmap(unsigned long arg)
{
    struct sdh_e1linkmap_map lm_map;
    struct sdh_e1user_map *user_map = NULL;
    int ret = -1;

    memset(&lm_map, 0, sizeof(struct sdh_e1linkmap_map));
    if (copy_from_user(&lm_map, (void __user *)arg, sizeof(struct sdh_e1linkmap_map)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }

    user_map = &lm_map.map;
    OICDBG("%s : au4 %d e1 %d sel %d user %d-%d-%d-%d", __FUNCTION__, user_map->au4, user_map->e1,\
            lm_map.lmp_sel,user_map->user_chassisid, user_map->user_slot, \
            user_map->user_port, user_map->user_e1);

    ret = sdh_cfg_rule(LIST_TYPE_LP_E1USER, user_map->au4, user_map->e1, user_map->user_e1, user_map->user_port);
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_rule(LIST_TYPE_LP_DEVMAP, user_map->au4, user_map->e1, user_map->user_slot, user_map->user_chassisid);
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_rule(LIST_TYPE_LP_SEL, user_map->au4, user_map->e1, lm_map.lmp_sel, user_map->enable);
    if (ret != 0)
        goto err_exit;

    if (user_map->rule == E1_RULE_LOCAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_LOCAL_LM, user_map->au4, user_map->e1, user_map->sel, user_map->enable);
    } else if (user_map->rule == E1_RULE_GLOBAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_GLOBLA_LM, user_map->au4, user_map->e1, user_map->sel, user_map->enable);
    } else {
        OICERR("Unkown e1 rule .");
        ret = -1;
    }
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_channle_en(0, user_map->au4, user_map->e1, user_map->enable);
    if (ret != 0)
        goto err_exit;

    return 0;

err_exit:
    return -1;
}

int oic_fops_ioctl_set_sdh_e164k(unsigned long arg)
{
    struct sdh_e164k_map e164k_map;
    struct sdh_e1user_map *user_map = NULL;
    int ret = -1;

    memset(&e164k_map, 0, sizeof(struct sdh_e164k_map));
    if (copy_from_user(&e164k_map, (void __user *)arg, sizeof(struct sdh_e164k_map)) != 0) {
        OICERR("copy_from_user failed.");
        return -1;
    }

    user_map = &e164k_map.map;

    OICDBG("%s : au4 %d e1 %d user %d-%d-%d-%d", __FUNCTION__, user_map->au4, user_map->e1,\
            user_map->user_chassisid, user_map->user_slot, user_map->user_port, user_map->user_e1);

    ret = sdh_cfg_rule(LIST_TYPE_LP_E1USER, user_map->au4, user_map->e1, user_map->user_e1, user_map->user_port);
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_rule(LIST_TYPE_LP_DEVMAP, user_map->au4, user_map->e1, user_map->user_slot, user_map->user_chassisid);
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_rule(LIST_TYPE_LP_64KPPP_SEL, user_map->au4, user_map->e1, e164k_map.e164k_sel, user_map->enable);
    if (ret != 0)
        goto err_exit;

    if (user_map->rule == E1_RULE_LOCAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_LOCAL_64KPPP, user_map->au4, user_map->e1, user_map->sel, user_map->enable);
    } else if (user_map->rule == E1_RULE_GLOBAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_GLOBLA_64KPPP, user_map->au4, user_map->e1, user_map->sel, user_map->enable);
    } else {
        OICERR("Unkown e1 rule .");
        ret = -1;
    }
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_channle_en(0, user_map->au4, user_map->e1, user_map->enable);
    if (ret != 0)
        goto err_exit;

    return 0;

err_exit:
    return -1;
}

int oic_fops_ioctl_set_sdh_e1gfp(unsigned long arg)
{
    struct sdh_e1gfp_map e1gfp_map;
    struct sdh_e1user_map *user_map = NULL;
    int ret = -1;

    memset(&e1gfp_map, 0, sizeof(struct sdh_e1gfp_map));
    if (copy_from_user(&e1gfp_map, (void __user *)arg, sizeof(struct sdh_e1gfp_map)) != 0) {
        return -1;
    }

    user_map = &e1gfp_map.map;

    ret = sdh_cfg_rule(LIST_TYPE_LP_E1GFP_USER, user_map->au4, user_map->e1, user_map->user_e1, user_map->user_port);
    if (ret != 0)
        goto err_exit;

    ret = sdh_cfg_rule(LIST_TYPE_LP_E1GFP_DEVMAP, user_map->au4, user_map->e1, user_map->user_slot, user_map->user_chassisid);
    if (ret != 0)
        goto err_exit;

    if (user_map->rule == E1_RULE_LOCAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_LOCAL_E1GFP, user_map->au4, user_map->e1, user_map->sel, user_map->enable);
    } else if (user_map->rule == E1_RULE_GLOBAL) {
        ret = sdh_cfg_rule(LIST_TYPE_LP_GLOBAL_E1GFP, user_map->au4, user_map->e1, user_map->sel, user_map->enable);
    } else {
        OICERR("Unkown e1 rule .");
        ret = -1;
    }
    if (ret != 0)
        goto err_exit;

    return 0;

err_exit:
    return -1;
}

int oic_fops_ioctl_set_sdh_board_info(unsigned long arg)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile sdh_devinfo_t *devinfo= (volatile sdh_devinfo_t *)&fpga->devinfo;
    struct sdh_fpga_info info;
    unsigned char val = 0;

    memset(&info, 0, sizeof(struct sdh_fpga_info));
    if (copy_from_user(&info, (void __user *)arg, sizeof(struct sdh_fpga_info)) != 0) {
        OICERR("copy_from_user failed.");
        return -1; 
    }

    mutex_lock(&oicdev->lock);
    BDREGCHK(devinfo->fpgaid, info.fpgaid);
    val = ((info.chassisid & 0x0f) << 4) | (info.physlot & 0x0f);
    BDREGCHK(devinfo->chassisid_slot, val);
    mutex_unlock(&oicdev->lock);

    return 0;
}

int oic_fops_ioctl_get_sdh_board_info(unsigned long arg)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile sdh_bdstartup_t *status = (volatile sdh_bdstartup_t *)&fpga->startup;
    volatile ver_t *ver = (volatile ver_t *)&fpga->verinfo;
    volatile sdh_devinfo_t *dev = (volatile sdh_devinfo_t *)&fpga->devinfo;
    struct sdh_fpga_info info;

    if ((status->dstop == 1) && (status->dsr == 1) && (status->bcr == 1)) {
        info.status = 1;
    } else {
        info.status = 0;
    }
    
    info.type = oicdev->sdhtype;
    info.version = ver->vrr;
    info.subtype = ver->sub_vrr;
    info.date[0] = ver->dar[0];
    info.date[1] = ver->dar[1];
    info.date[2] = ver->dar[2];
    info.date[3] = 0;
    info.fpgaid = dev->fpgaid;

    return (copy_to_user((void __user *)arg, &info, sizeof(struct sdh_fpga_info)));
}

int oic_fops_ioctl_get_sdh_e1_info(unsigned long arg)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile refer_E1 *refer= (volatile refer_E1 *)&fpga->E1info;
    struct e1_info info;
    memset(&info, 0, sizeof(struct e1_info));

    mutex_lock(&oicdev->lock);
    BDREGCHK(refer->cfg_refer, 0x30);
    info.e1_cnt = (refer->read2_2m << 8) | refer->read3_2m;
    BDREGCHK(refer->cfg_refer, 0x31);
    info.m64k_cnt = (refer->read2_2m << 8) | refer->read3_2m;
    BDREGCHK(refer->cfg_refer, 0x31);
    info.m2m_cnt = (refer->read0_2m << 8) | refer->read1_2m;
    BDREGCHK(refer->cfg_refer, 0x32);
    info.m2m_nfm_cnt = (refer->read2_2m << 8) | refer->read3_2m;
    mutex_unlock(&oicdev->lock);

    return (copy_to_user((void __user *)arg, &info, sizeof(struct e1_info)));
}

int oic_fops_ioctl_set_sdh_payload_info(unsigned long arg)
{
    return 0;
}

int sdh_board_stat(uint8_t sel, unsigned long *val)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile mis_cnt_t *cnt = (volatile mis_cnt_t *)&fpga->mis_cnt;
    unsigned long tmpval = 0;

    BDREGCHK(cnt->mis_cnt_sel, sel);
    tmpval = (cnt->mis_cnt_val_0 << 0) |  (cnt->mis_cnt_val_1 << 8) | (cnt->mis_cnt_val_2 << 16) | (cnt->mis_cnt_val_3 << 24);
    *val = tmpval;

    return 0;
}

int oic_fops_ioctl_set_sdh_board_stat_type(unsigned long arg)
{
    volatile sdh_fpga_reg *fpga = (volatile sdh_fpga_reg *)oicdev->fpga_virt;
    volatile mis_cnt_t *cnt = (volatile mis_cnt_t *)&fpga->mis_cnt;
    int tmpval = 0;
    unsigned char regval = 0;

    if (copy_from_user(&tmpval, (int __user *)arg, sizeof(int)) != 0) {
        OICERR("copy_from_user failed.");
        return -1; 
    }
    //OICINFO("%d--%s type %d", __LINE__, __func__, tmpval);

    switch (tmpval) {
        case SDH_BOARD_STAT_TYPE_CLEAR:

            mutex_lock(&oicdev->lock);
            regval = cnt->mis_cnt_clr & 0xfe;
            BDREGCHK(cnt->mis_cnt_clr, regval);
            regval = cnt->mis_cnt_clr | 0x01;
            BDREGCHK(cnt->mis_cnt_clr, regval);
            mutex_unlock(&oicdev->lock);
            break;

        case SDH_BOARD_STAT_TYPE_PKTS:

            mutex_lock(&oicdev->lock);
            regval = cnt->mis_cnt_clr & 0xfd;
            BDREGCHK(cnt->mis_cnt_clr, regval);
            mutex_unlock(&oicdev->lock);
            break;

        case SDH_BOARD_STAT_TYPE_BYTES:

            mutex_lock(&oicdev->lock);
            regval = cnt->mis_cnt_clr | 0x02;
            BDREGCHK(cnt->mis_cnt_clr, regval);
            mutex_unlock(&oicdev->lock);
            break;

        default:
            OICERR("Unkown board stat type %d", tmpval);
            return -1;
    }

    return 0;
}

int oic_fops_ioctl_set_sdh_board_stat(unsigned long arg)
{
    struct sdh_fpga_stat stat; 

    mutex_lock(&oicdev->lock);
    memset(&stat, 0, sizeof(&stat));
    sdh_board_stat(0x10, &stat.task_p);
    sdh_board_stat(0x11, &stat.ch_err);
    sdh_board_stat(0x12, &stat.gfp_ok);
    sdh_board_stat(0x13, &stat.gfp_fisu);
    sdh_board_stat(0x14, &stat.gfp_we_err);
    sdh_board_stat(0x15, &stat.atm_ok);
    sdh_board_stat(0x16, &stat.atm_fisu);
    sdh_board_stat(0x17, &stat.atm_we_err);
    sdh_board_stat(0x18, &stat.ppp_ok);
    sdh_board_stat(0x19, &stat.ppp_fisu);
    sdh_board_stat(0x1a, &stat.ppp_we_err);
    sdh_board_stat(0x1b, &stat.gfp_los);
    sdh_board_stat(0x1c, &stat.ppp_los);
    sdh_board_stat(0x1d, &stat.atm_los);
    sdh_board_stat(0x1e, &stat.gfp_cnt);
    sdh_board_stat(0x1f, &stat.atm_cnt);
    sdh_board_stat(0x20, &stat.ppp_cnt);
    sdh_board_stat(0x21, &stat.e1user_cnt);
    sdh_board_stat(0x22, &stat.e164k_ppp_cnt);
    sdh_board_stat(0x23, &stat.e1lm_cnt);
    sdh_board_stat(0x24, &stat.hp2lp_cnt);
    sdh_board_stat(0x25, &stat.laps_ok);
    sdh_board_stat(0x26, &stat.laps_fisu);
    sdh_board_stat(0x27, &stat.laps_cnt);
    sdh_board_stat(0x28, &stat.laps_we_err);
    sdh_board_stat(0x29, &stat.laps_los);

    sdh_board_stat(0x0a, &stat.local_eth0);
    sdh_board_stat(0x0b, &stat.local_eth1);
    sdh_board_stat(0x0c, &stat.local_eth2);
    sdh_board_stat(0x0d, &stat.local_eth3);

    sdh_board_stat(0x06, &stat.sw_eth0);
    sdh_board_stat(0x07, &stat.sw_eth1);
    sdh_board_stat(0x08, &stat.sw_eth2);
    sdh_board_stat(0x09, &stat.sw_eth3);

    sdh_board_stat(0x30, &stat.gfp_tx_cnt);
    sdh_board_stat(0x31, &stat.atm_tx_cnt);
    sdh_board_stat(0x32, &stat.ppp_tx_cnt);
    sdh_board_stat(0x33, &stat.e3_tx_cnt);
    sdh_board_stat(0x34, &stat.lap_tx_cnt);
    sdh_board_stat(0x35, &stat.cx_rx_cnt);
    sdh_board_stat(0x36, &stat.e3_rx_cnt);
    sdh_board_stat(0x37, &stat.rx_los_cnt);
    mutex_unlock(&oicdev->lock);

	if(copy_to_user((void __user *)arg, &stat, sizeof(struct sdh_fpga_stat)) != 0) {
        OICERR("copy_to_user err."); 
        return -1;
    }
    return 0;
}

int oic_fops_ioctl_get_slink_info_hp(unsigned long arg)
{
    struct fpga_board_runinfo_port_ex slink_h;
    struct slink_info slink;
    int ret = -1;
    int port = 0;
    int val = 0;
    unsigned int i = 0;

    memset(&slink_h, 0, sizeof(struct fpga_board_runinfo_port_ex));
	ret = copy_from_user(&slink_h, (void __user *)arg, sizeof(struct fpga_board_runinfo_port_ex));
    if (ret != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    port = slink_h.fiber;

    for (i = 0; i <= GFP_GET_INFO_SELNUM; i++) {
        ret = get_info_rule(port, 0, i, &val);
        if (ret != 0)
            goto err;

        ret = analysis_sel_info(val, i, &slink_h, &slink);
        if (ret != 0)
            goto err;
    }

	ret = copy_to_user((void __user *)arg, &slink_h, sizeof(struct fpga_board_runinfo_port_ex));
    if (ret != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        goto err;
    }
    return 0;

err:
    return -1;
}

int oic_fops_ioctl_get_channel2global(unsigned long arg)
{
    volatile gfp_fpga_reg *fpga = (volatile gfp_fpga_reg *)oicdev->fpga_virt;
    volatile get_info_t *gsinfo = (volatile get_info_t *)&fpga->get_info;
    struct gfp_local2global map;
    int au4 = 0;
    int e1 = 0;

    memset(&map, 0, sizeof(&map));
    if (copy_from_user(&map, (void * __user)arg, sizeof(struct gfp_local2global)) != 0) {
        OICERR("%d--%s: copy_from_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }
    au4 = map.local_au4;
    e1 = map.local_e1;

    BDREGCHK(gsinfo->select_port, au4);
    BDREGCHK(gsinfo->select_chan, e1);
    BDREGCHK(gsinfo->select_info, 0x09);
    map.chassisid = (gsinfo->get_val_3 & 0xf0) >> 4;
    map.slot = (gsinfo->get_val_3 & 0x0f) >> 0;
    map.port = (gsinfo->get_val_2 & 0xff) >> 0;
    map.au4 = (gsinfo->get_val_1 & 0xff) >> 0;
    map.e1 = (gsinfo->get_val_0 & 0xff) >> 0;

    if (copy_to_user((void * __user)arg, &map, sizeof(struct gfp_local2global)) != 0) {
        OICERR("%d--%s: copy_to_user failed.", __LINE__, __FUNCTION__);
        return -1;
    }

    return 0;
}

static int oic_open(struct inode *inodp, struct file *filp)
{
	filp->private_data = (struct oic_device *)container_of(inodp->i_cdev, struct oic_device, cdev);

	OICDBG("device openned.");
	return 0;
}

static int oic_close(struct inode *inodp, struct file *filp)
{
	OICDBG("device closed.");
	return 0;
}

static long oic_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	if (_IOC_TYPE(cmd) != OIC_IOC_MAGIC) {
		OICERR("magic number mismatch.");
		return -EINVAL;
	}

	if (_IOC_NR(cmd) >= OIC_IOC_NUMBER) {
		OICERR("invalid command: %u", cmd);
		return -EINVAL;
	}

	switch (cmd) {
	case OIC_GET_FPGA_VERSION:
		rc = oic_fops_ioctl_get_fpga_version(arg);
		break;

	case OIC_GET_FPGA_DDR_STATUS:
		rc = oic_fops_ioctl_get_fpga_ddr_status(arg);
		break;

	case OIC_SET_FPGA_BD_STARTUP:
		rc = oic_fops_ioctl_set_fpga_bd_startup(arg);
		break;

	case OIC_GET_FPGA_BD_STARTUP:
		rc = oic_fops_ioctl_get_fpga_bd_startup(arg);
		break;

	case OIC_SET_FPGA_SYNCTIME:
		rc = oic_fops_ioctl_set_fpga_synctime(arg);
		break;

	case OIC_SET_FPGA_BD_CFGINFO:
		rc = oic_fops_ioctl_set_fpga_bd_cfginfo(arg);
		break;

	case OIC_GET_FPGA_BD_CFGINFO:
		rc = oic_fops_ioctl_get_fpga_bd_cfginfo(arg);
		break;

	case OIC_SET_FPGA_BD_CFGINFO_EX:
		rc = oic_fops_ioctl_set_fpga_bd_cfginfo_ex(arg);
		break;

	case OIC_GET_FPGA_BD_CFGINFO_EX:
		rc = oic_fops_ioctl_get_fpga_bd_cfginfo_ex(arg);
		break;

	case OIC_GET_FPGA_BD_RUNINFO:
		rc = oic_fops_ioctl_get_fpga_bd_runinfo(arg);
		break;

	case OIC_SET_FPGA_64K_CH_TRAN:
		rc = oic_fops_ioctl_set_fpga_64k_ch_tran(arg);
		break;

	case OIC_SET_FPGA_2M_CH_TRAN:
		rc = oic_fops_ioctl_set_fpga_2m_ch_tran(arg);
		break;

	case OIC_SET_FPGA_2M_CH_VALID:
		rc = oic_fops_ioctl_set_fpga_2m_ch_valid(arg);
		break;

	case OIC_GET_FPGA_2M_CH_VALID:
		rc = oic_fops_ioctl_get_fpga_2m_ch_valid(arg);
		break;

	case OIC_SET_FPGA_PL_CFGCH:
		rc = oic_fops_ioctl_set_fpga_pl_cfgch(arg);
		break;

	case OIC_SET_FPGA_PL_RSTCH:
		rc = oic_fops_ioctl_set_fpga_pl_rstch();
		break;

	case OIC_GET_FPGA_PL_MATCH:
		rc = oic_fops_ioctl_get_fpga_pl_match(arg);
		break;

	case OIC_GET_FPGA_IN_STAT:
		rc = oic_fops_ioctl_get_fpga_in_stat(arg);
		break;

	case OIC_SET_FPGA_PL_MODE:
		rc = oic_fops_ioctl_set_fpga_pl_mode(arg);
		break;

	case OIC_SET_FPGA_PL_SLOT:
		rc = oic_fops_ioctl_set_fpga_pl_tbl_info(arg);
		break;

	case OIC_SET_FPGA_PL_AGE_CHANNEL:
		rc = oic_fops_ioctl_set_fpga_pl_age_ch(arg);
		break;

	case OIC_GET_FPGA_IS_SIGCH:
		rc = oic_fops_ioctl_get_fpga_is_sigch(arg);
		break;

	case OIC_SET_FPGA_SILENCE_RANGE:
		rc = oic_fops_ioctl_set_silence_range(arg);
		break;

	case OIC_GET_FPGA_SILENCE_RANGE:
		rc = oic_fops_ioctl_get_silence_range(arg);
		break;

	case OIC_GET_FPGA_SILENCE_RESULT:
		rc = oic_fops_ioctl_get_silence_result(arg);
		break;

	case OIC_SET_FPGA_SILENCE_TOUT:
		rc = oic_fops_ioctl_set_silence_timeout(arg);
		break;

	case OIC_GET_FPGA_SILENCE_TOUT:
		rc = oic_fops_ioctl_get_silence_timeout(arg);
		break;

    case OIC_GET_FPGA_BD_RUNINFO_EX:
        rc = oic_fops_ioctl_get_fpga_bd_runinfo_ex(arg);
        break;

    case OIC_GET_FPGA_SLINK_INFO:
        rc = oic_fops_ioctl_get_fpga_slink_info(arg);
        break;

    case OIC_SET_FPGA_SLINK_INFO_START:
        rc = oic_fops_ioctl_set_fpga_slink_info_start(arg);
        break;

    case OIC_SET_FPGA_SLINK_INFO_END:
        rc = oic_fops_ioctl_set_fpga_slink_info_end(arg);
        break;

    case OIC_SET_FPGA_TRY_GROUP:
        rc = oic_fops_ioctl_set_fpga_try_group(arg);
        break;

    case OIC_SET_FPGA_GROUP:
        rc = oic_fops_ioctl_set_fpga_group(arg);
        break;

    case OIC_GET_FPGA_TRY_RESULT:
        rc = oic_fops_ioctl_get_fpga_try_result(arg);
        break;

    case OIC_GET_FPGA_GROUP:
        rc = oic_fops_ioctl_get_fpga_group(arg);
        break;

    case OIC_GET_FPGA_TRY_GROUP:
        rc = oic_fops_ioctl_get_fpga_try_group(arg);
        break;

    case OIC_SET_FPGA_HDLC_CFG:
        rc = oic_fops_ioctl_set_fpga_hdlc_cfg(arg);
        break;

    case OIC_GET_FPGA_HDLC_CHSTAT:
        rc = oic_fops_ioctl_get_fpga_hdlc_chstat(arg);
        break;

    case OIC_GET_FPGA_HDLC_LOCAL_SCHAN:
        rc = oic_fops_ioctl_get_fpga_hdlc_local_schan(arg);
        break;

    case OIC_GET_FPGA_HDLC_CONFIG_SCHAN:
        rc = oic_fops_ioctl_get_fpga_hdlc_config_schan(arg);
        break;

    case OIC_GET_FPGA_HDLC_GE_STAT:
        rc = oic_fops_ioctl_get_fpga_hdlc_ge_stat(arg);
        break;

    case OIC_GET_FPGA_HDLC_DB_STATUS:
        rc = oic_fops_ioctl_get_fpga_hdlc_db_status(arg);
        break;

    case OIC_SET_GFP_FPGA_SPU_SELNUM:
        rc = oic_fops_ioctl_set_gfp_fpga_spu_selnum(arg);
        break;

    case OIC_GET_GFP_FPGA_SPU_SELNUM:
        rc = oic_fops_ioctl_get_gfp_fpga_spu_selnum(arg);
        break;

    case OIC_SET_GFP_FPGA_SPU_FORWARD_ENABLE:
        rc = oic_fops_ioctl_set_gfp_fpga_spu_forward_enable(arg);
        break;

    case OIC_SET_GFP_FPGA_SPU_FORWARD_DISABLE:
        rc = oic_fops_ioctl_set_gfp_fpga_spu_forward_disable(arg);
        break;

    case OIC_SET_GFP_FPGA_SPU_FORWARD_RULE:
        rc = oic_fops_ioctl_set_gfp_fpga_spu_forward_rule(arg);
        break;

    case OIC_SET_GFP_FPGA_SPU_CHANNEL_FORWARD:
        rc = oic_fops_ioctl_set_gfp_fpga_spu_channel_forward(arg);
        break;

    case OIC_SET_SDH_LOCAL_RULE:
        rc = oic_fops_ioctl_set_sdh_local_rule(arg);
        break;

    case OIC_SET_SDH_GLOBAL_RULE:
        rc = oic_fops_ioctl_set_sdh_global_rule(arg);
        break;

    case OIC_SET_SDH_INNER_RULE:
        rc = oic_fops_ioctl_set_sdh_inner_rule(arg);
        break;

    case OIC_SET_SDH_SET_SEL:
        rc = oic_fops_ioctl_set_sdh_sel(arg);
        break;

    case OIC_SET_SDH_SET_E1USER:
        rc = oic_fops_ioctl_set_sdh_e1user(arg);
        break;

    case OIC_SET_SDH_SET_E1LINKMAP:
        rc = oic_fops_ioctl_set_sdh_e1linkmap(arg);
        break;

    case OIC_SET_SDH_GET_BOARD_INFO:
        rc = oic_fops_ioctl_get_sdh_board_info(arg);
        break;

    case OIC_SET_SDH_GET_PAYLOAD_INFO:
        rc = oic_fops_ioctl_set_sdh_payload_info(arg);
        break;

    case OIC_SET_SDH_GET_BOARD_STAT:
        rc = oic_fops_ioctl_set_sdh_board_stat(arg);
        break;

    case OIC_GET_FPGA_SLINK_INFO_HP:
        rc = oic_fops_ioctl_get_slink_info_hp(arg);
        break;

    case OIC_GET_SDH_SET_BOARD_STAT_TYPE:
        rc = oic_fops_ioctl_set_sdh_board_stat_type(arg);
        break;

    case OIC_SET_SDH_SET_E164K:
        rc = oic_fops_ioctl_set_sdh_e164k(arg);
        break;

    case OIC_SET_SDH_SET_BOARD_INFO:
        rc = oic_fops_ioctl_set_sdh_board_info(arg);
        break;

    case OIC_SET_SDH_GET_E1_INFO:
        rc = oic_fops_ioctl_get_sdh_e1_info(arg);
        break;

    case OIC_GET_SDH_GET_AU4_STATUS:
        rc = oic_fops_ioctl_get_sdh_au4_status(arg);
        break;

    case OIC_SET_SDH_SET_E1GFP:
        rc = oic_fops_ioctl_set_sdh_e1gfp(arg);
        break;

    case OIC_GET_SDH_CHANNEL2GLOBAL:
        rc = oic_fops_ioctl_get_channel2global(arg);
        break;

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

struct file_operations oic_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = oic_ioctl,
	.open		= oic_open,
	.release	= oic_close,
};

int oic_fops_init()
{
	dev_t devno;
	int   rc = 0;
	char devname[32];

	if (gfpmode == 1) {
		if (oic_init_gfp_fpga_start() != 0) {
			OICERR("failed to init gfp fpga start");
			return -1;
		}
	} else {
		if (oic_init_fpga_start() != 0) {
			OICERR("failed to init fpga start");
			return -1;
		}
	}

	if (fpgadev > 0)
	{
		memset(devname, 0, sizeof(devname));
		sprintf(devname, "%s%d", OIC_NAME, fpgadev);
	}
	else
	{
		memset(devname, 0, sizeof(devname));
		sprintf(devname, "%s", OIC_NAME);
	}

	rc = alloc_chrdev_region(&devno, 0, 1, devname);
	if (rc < 0) {
		OICERR("failed to alloc chrdev region");
		return -1;
	}
	oicdev->major = MAJOR(devno);

	/* chrdev setup */
	cdev_init(&(oicdev->cdev), &oic_fops);
	oicdev->cdev.owner = THIS_MODULE;
	if (cdev_add(&(oicdev->cdev), devno, 1) != 0) {
		OICERR("failed to add cdev.");
		goto oic_fops_init_dealloc_region;
	}

	oicdev->class = class_create(THIS_MODULE, devname);
	if (oicdev->class == NULL) {
		OICERR("failed to create class");
		goto oic_fops_init_delete_cdev;
	}

	device_create(oicdev->class, NULL, MKDEV(oicdev->major, 0), NULL, devname);

	init_fst(oicdev);

	return 0;

oic_fops_init_delete_cdev:
	cdev_del(&(oicdev->cdev));
oic_fops_init_dealloc_region:
	unregister_chrdev_region(devno, 1);

	return -1;
}

void oic_fops_exit()
{
	dev_t devno = MKDEV(oicdev->major, 0);
	
	exit_fst();

	device_destroy(oicdev->class, MKDEV(oicdev->major, 0));
	class_destroy(oicdev->class);
	cdev_del(&(oicdev->cdev));
	unregister_chrdev_region(devno, 1);
}

