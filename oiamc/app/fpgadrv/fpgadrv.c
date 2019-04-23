/*
 * (C) Copyright 2015
 * <www.raycores.com> 
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include "fpgadrvdbg.h"
#include "fpgadrvctl.h"
#include "fpgadrv.h"
#include "dts_query.h"

#if defined(ATCA_EIPB)
#define BANK_FPGA_START      0xfa000100
#define BANK_FPGA_SIZE       0x900   /* 256K-256 */
#else    /* amc oiv3 */
#define BANK_FPGA_START      0xf1000000
#define BANK_FPGA_SIZE       0x1000000   /* 16M */
#endif

#define FPGA_NAME "fpga"
#define FPGA_NUM 1

static int mmajor = 0;

module_param(mmajor, int, 0);
MODULE_PARM_DESC(mmajor, "Device major number");

fpgadrv_dev_t *fpgadrv_devp = NULL;

static int fpgadrv_open(struct inode *inodp, struct file *filp)
{
	filp->private_data = (fpgadrv_dev_t *)container_of(inodp->i_cdev, fpgadrv_dev_t, cdev);
	FPGADRVDBG("device opened.");
	return 0;
}

static int fpgadrv_release(struct inode *inodp, struct file *filp)
{
	FPGADRVDBG("device closed.");
	return 0;
}

int fpgadrv_check_dev_stat(void)
{
	if (NULL != fpgadrv_devp) {
		return fpgadrv_devp->stat == STARTUPED ? 0 : -1;
	}

	return -1;
}

static long fpgadrv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	if (_IOC_TYPE(cmd) != ESNAPP_IOC_MAGIC) {
		FPGADRVERR("magic number mismatch.");	
		return -EINVAL;
	}

	if (_IOC_NR(cmd) >= ESNAPP_IOC_NUMBER) {
		FPGADRVERR("invalid command: %u", cmd);	
		return -EINVAL;
	}

	if ((ESNAPP_START_UP!= cmd) && (fpgadrv_check_dev_stat() != 0)) {
		FPGADRVINFO("[%s:%d], Start-up device first.\n", __func__, __LINE__);
		return -EINVAL;
	}

	switch (cmd) {
	case ESNAPP_RD_VERSION:
		rc = fpgadrvctl_read_version(arg);
		break;

	case ESNAPP_RD_DDR_INIT_STAT:
		rc = fpgadrvctl_read_ddr_init_stat(arg);
		break;

	case ESNAPP_RD_BD_START_OPT:
		rc = fpgadrvctl_read_board_start_opt(arg);
		break;

	case ESNAPP_WR_BD_START_OPT:
		rc = fpgadrvctl_write_board_start_opt(arg);
		break;

	case ESNAPP_RD_BD_CFG_INFO:
		rc = fpgadrvctl_read_board_cfg_info(arg);
		break;

	case ESNAPP_WR_BD_CFG_INFO:
		rc = fpgadrvctl_write_board_cfg_info(arg);
		break;

	case ESNAPP_RD_VOICE_CFG:
		rc = fpgadrvctl_read_voice_cfg(arg);
		break;

	case ESNAPP_WR_VOICE_CFG:
		rc = fpgadrvctl_write_voice_cfg(arg);
		break;

	case ESNAPP_RD_PL_SCAN_RST:
		rc = fpgadrvctl_read_pl_scan_rst(arg);
		break;

	case ESNAPP_WR_PL_SCAN_RST:
		rc = fpgadrvctl_write_pl_scan_rst(arg);
		break;

	case ESNAPP_RD_CFG_RESULT:
		rc = fpagdrvctl_read_cfg_result(arg);
		break;

	case ESNAPP_RD_BD_RUN_INFO:
		rc = fpgadrvctl_read_board_run_info(arg);
		break;

	case ESNAPP_RD_64K_CH_TRAN:
		rc = fpgadrvctl_read_64k_transfer(arg);
		break;

	case ESNAPP_WR_64K_CH_TRAN:
		rc = fpgadrvctl_write_64k_transfer(arg);
		break;

	case ESNAPP_RD_2M_CH_TRAN:
		rc = fpgadrvctl_read_2m_transfer(arg);
		break;

	case ESNAPP_WR_2M_CH_TRAN:
		rc = fpgadrvctl_write_2m_transfer(arg);
		break;

	case ESNAPP_RD_E1PHY_STAT:
		rc = fpgadrvctl_read_e1phy_stat(arg);
		break;

	case ESNAPP_WR_E1PHY_STAT:
		rc = fpgadrvctl_write_e1phy_stat(arg);
		break;
	case ESNAPP_START_UP:
		rc = fpgadrvctl_start_up_device();
		break;

	default :
		return -EINVAL;
	}

	return rc;
}

struct file_operations fpgadrv_fops = {
	.owner 		= THIS_MODULE,
	.unlocked_ioctl	= fpgadrv_ioctl,
	.open		= fpgadrv_open,
	.release	= fpgadrv_release,
};

static int fpgadrv_register_device(void)
{
	dev_t devno;
	int   rc = 0;

	if (mmajor) {
		devno = MKDEV(mmajor, FPGADRV_MINOR);
		rc = register_chrdev_region(devno, 1, ESNAPP_DEV_NAME);	
	} else {
		rc = alloc_chrdev_region(&devno, 0, 1, ESNAPP_DEV_NAME);
		mmajor = MAJOR(devno);
	}
	if (rc < 0) {
		FPGADRVERR("failed to register/alloc chrdev region");
		return -1;
	}

	/* chrdev setup */
	cdev_init(&(fpgadrv_devp->cdev), &fpgadrv_fops);
	fpgadrv_devp->cdev.owner = THIS_MODULE;

	if (cdev_add(&(fpgadrv_devp->cdev), devno, 1) != 0) {
		FPGADRVERR("failed to add cdev.");
		goto fpgadrv_register_device_err_exit;
	}

	return 0;

fpgadrv_register_device_err_exit:
	unregister_chrdev_region(devno, 1);

	return -1;
}

static void fpgadrv_unregister_device(void)
{
	dev_t devno = MKDEV(mmajor, FPGADRV_MINOR);

	cdev_del(&(fpgadrv_devp->cdev));
	unregister_chrdev_region(devno, 1);
}


static int fpgadrv_init_fpga(void *fpgadrv) {
	int ret = 0;
	fpgadrv_dev_t *devp = (fpgadrv_dev_t *)fpgadrv;

	devp->num_fpga = dts_dev_nums(FPGA_NAME);
	if (devp->num_fpga == FPGA_NUM) {
		ret = dts_dev_phys_addr(FPGA_NAME, devp->num_fpga - 1, 0, &devp->fpga_phys_addr);
		if (ret != 0 ) {
			return -1;
		}

		ret = dts_dev_size(FPGA_NAME, devp->num_fpga - 1, 0, &devp->fpga_phys_size);
		if (ret != 0) {
			return -1;
		}

		return 0;
	}

	return -1;
}

static int __init fpgadrv_init_module(void)
{
	int rc = 0;

	/* allocate device */
	fpgadrv_devp = (fpgadrv_dev_t *)kmalloc(sizeof(fpgadrv_dev_t), GFP_KERNEL);
	if (!fpgadrv_devp) {
		FPGADRVERR("failed to allocate device struct.");
		return -1;
	}
	memset(fpgadrv_devp, 0, sizeof(fpgadrv_dev_t));

	mutex_init(&fpgadrv_devp->lock);

	rc = fpgadrv_init_fpga((void *)fpgadrv_devp);
	if (rc != 0) {
		FPGADRVERR("failed to initialize FPGA.");
		return -1;
	}

	/* register chrdev */
	rc = fpgadrv_register_device();
	if (rc != 0) {
		FPGADRVERR("failed to register chrdev.");
		kfree(fpgadrv_devp);
		return -1;
	}

	/* ioremap */
#ifdef __DEFAULT__
	fpgadrv_devp->vaddr_bk_fpga = ioremap(BANK_FPGA_START, BANK_FPGA_SIZE);
#else
	fpgadrv_devp->vaddr_bk_fpga = ioremap(fpgadrv_devp->fpga_phys_addr, fpgadrv_devp->fpga_phys_size);
#endif
	if (!fpgadrv_devp->vaddr_bk_fpga) {
		FPGADRVERR("ioremap fpga bank start addr failed");
		fpgadrv_unregister_device();
		kfree(fpgadrv_devp);
		return -1;
	}
#ifdef __DEFAULT__
	FPGADRVDBG("fpga bank 0x%x:0x%x ioremap addr %p\n", 
			BANK_FPGA_START, BANK_FPGA_SIZE, fpgadrv_devp->vaddr_bk_fpga);
#else
	FPGADRVDBG("fpga bank 0x%x:0x%x(dts) ioremap addr %p\n", 
			fpgadrv_devp->fpga_phys_addr, fpgadrv_devp->fpga_phys_size, fpgadrv_devp->vaddr_bk_fpga);
#endif

	fpgadrv_devp->fpgadrv_class = class_create(THIS_MODULE, ESNAPP_DEV_NAME);
	if (NULL == fpgadrv_devp->fpgadrv_class) {
		FPGADRVINFO("WARNING: Failed to Create class, so need to create node in /dev/ by mknod. major:%d", mmajor);
	}
	else {
		fpgadrv_devp->fpga_dev_node = device_create(fpgadrv_devp->fpgadrv_class, NULL, MKDEV(mmajor, FPGADRV_MINOR),
				NULL, ESNAPP_DEV_NAME);
		if (NULL == fpgadrv_devp->fpga_dev_node) {
			FPGADRVINFO("WARNING: Failed to Create device node, so need to create node in /dev/ by mknod. major:%d", mmajor);
		}
	}

	FPGADRVINFO("module init ok!");

	return 0;
}

static void __exit fpgadrv_exit_module(void)
{
	if ((NULL != fpgadrv_devp->fpgadrv_class) &&
			(NULL != fpgadrv_devp->fpga_dev_node)) {
		device_destroy(fpgadrv_devp->fpgadrv_class, MKDEV(mmajor, FPGADRV_MINOR));
		fpgadrv_devp->fpga_dev_node = NULL;
	}

	if (NULL != fpgadrv_devp->fpgadrv_class) {
		class_destroy(fpgadrv_devp->fpgadrv_class);
		fpgadrv_devp->fpgadrv_class = NULL;
	}

	iounmap(fpgadrv_devp->vaddr_bk_fpga);
	fpgadrv_unregister_device();
	if (fpgadrv_devp)
		kfree(fpgadrv_devp);

	FPGADRVINFO("module exit!");
}


module_init(fpgadrv_init_module);
module_exit(fpgadrv_exit_module);
MODULE_AUTHOR("xiumei.wang");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("YISAITONG FPGA DRIVER!");

