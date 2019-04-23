/*
 * (C) Copyright 2014
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
#include "clovesdbg.h"
#include "clovesctl.h"
#include "ledtimer.h"
#include "wdtimer.h"
#include "cloves.h"
#include "clovesdev.h"
#include "fpgactl.h"

static int flashled = 0;
static int enablewd = 1;

module_param(flashled, int, 0);
MODULE_PARM_DESC(flashled, "Flash LED");
module_param(enablewd, int, 0);
MODULE_PARM_DESC(enablewd, "Watchdog Enable");

cloves_dev_t *cloves_devp = NULL;


static int cloves_open(struct inode *inodp, struct file *filp)
{
	filp->private_data = (cloves_dev_t *)container_of(inodp->i_cdev, cloves_dev_t, cdev);
	CLOVESDBG("device opened.");
	return 0;
}

static int cloves_release(struct inode *inodp, struct file *filp)
{
	CLOVESDBG("device closed.");
	return 0;
}

static long cloves_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int rc = 0;

	if (_IOC_TYPE(cmd) != CLOVES_IOC_MAGIC) {
		CLOVESERR("magic number mismatch.");	
		return -EINVAL;
	}

	if (_IOC_NR(cmd) >= CLOVES_IOC_NUMBER) {
		CLOVESERR("invalid command: %u", cmd);	
		return -EINVAL;
	}

	switch (cmd) {
	case CLOVES_CTL_WATCHDOG:
		rc = cloves_ctl_watchdog(arg);
		break;

	case CLOVES_CTL_GREEN:
		rc = cloves_ctl_green(arg);
		break;

	case CLOVES_GET_DIALCODE:
		rc = cloves_get_dialcode(arg);
		break;

	case CLOVES_CFG_FPGA_START:
		rc = fpgactl_cfg_start(arg);
		break;

	case CLOVES_CFG_FPGA_LOAD:
		rc = fpgactl_cfg_load(arg);
		break;

	case CLOVES_CFG_FPGA_DONE:
		rc = fpgactl_cfg_done(arg);
		break;

	case CLOVES_CFG_FPGA_STOP:
		rc = fpgactl_cfg_stop(arg);
		break;

	case CLOVES_GET_FPGA_REG:
		rc = fpgactl_get_register(arg);
		break;

	case CLOVES_SET_FPGA_REG:
		rc = fpgactl_set_register(arg);
		break;

	case CLOVES_GET_FPGA_NUM:
		rc = fpgactl_get_number(arg);
		break;

	case CLOVES_GET_FPGA_INFO:
		rc = fpgactl_get_info(arg);
		break;

	default :
		return -EINVAL;
	}

	return rc;
}

struct file_operations cloves_fops = {
	.owner 		= THIS_MODULE,
	.unlocked_ioctl	= cloves_ioctl,
	.open		= cloves_open,
	.release	= cloves_release,
};

static int cloves_register_device(void)
{
	dev_t devno;
	int   rc = 0;

	rc = alloc_chrdev_region(&devno, 0, 1, CLOVES_NAME);
	if (rc < 0) {
		CLOVESERR("failed to alloc chrdev region");
		return -1;
	}
	cloves_devp->major = MAJOR(devno);

	/* chrdev setup */
	cdev_init(&(cloves_devp->cdev), &cloves_fops);
	cloves_devp->cdev.owner = THIS_MODULE;
	if (cdev_add(&(cloves_devp->cdev), devno, 1) != 0) {
		CLOVESERR("failed to add cdev.");
		goto cloves_register_device_err_exit;
	}

	cloves_devp->class = class_create(THIS_MODULE, CLOVES_NAME);
	if (cloves_devp->class == NULL) {
		CLOVESERR("failed to create class");
		goto cloves_register_device_err_create;
	}

	device_create(cloves_devp->class, NULL, MKDEV(cloves_devp->major, 0), NULL, CLOVES_NAME);

	return 0;

cloves_register_device_err_create:
	cdev_del(&(cloves_devp->cdev));
cloves_register_device_err_exit:
	unregister_chrdev_region(devno, 1);

	return -1;
}

static void cloves_unregister_device(void)
{
	dev_t devno = MKDEV(cloves_devp->major, 0);

	device_destroy(cloves_devp->class, MKDEV(cloves_devp->major, 0));
	class_destroy(cloves_devp->class);
	cdev_del(&(cloves_devp->cdev));
	unregister_chrdev_region(devno, 1);
}

static int __init cloves_init_module(void)
{
	int rc = 0;

	/* allocate device */
	cloves_devp = (cloves_dev_t *)kmalloc(sizeof(cloves_dev_t), GFP_KERNEL);
	if (!cloves_devp) {
		CLOVESERR("failed to allocate device struct.");
		return -1;
	}
	memset(cloves_devp, 0, sizeof(cloves_dev_t));
	cloves_devp->ledfreq = LEDTIMER_DEFAULT_FREQ;

	/* register chrdev */
	rc = cloves_register_device();
	if (rc != 0) {
		CLOVESERR("failed to register chrdev.");
		kfree(cloves_devp);
		return -1;
	}

	if (cloves_memmap() != 0) {
		cloves_unregister_device();
		kfree(cloves_devp);
		return -1;
	}
	cloves_gpio_init();

	if (fpgactl_init() != 0) {
		CLOVESERR("failed to init fpgactl.");
		cloves_unmemmap();
		cloves_unregister_device();
		kfree(cloves_devp);
		return -1;
	}

	if(flashled) {
		led_timer_init();
	}

	if(enablewd) {
		watchdog_timer_init();
	}

	CLOVESINFO("inited.");

	return 0;
}

static void __exit cloves_exit_module(void)
{
	led_timer_exit();
	watchdog_timer_exit();
	
	fpgactl_exit();
	cloves_unmemmap();
	cloves_unregister_device();
	kfree(cloves_devp);

	CLOVESINFO("exited.");
}

module_init(cloves_init_module);
module_exit(cloves_exit_module);
MODULE_AUTHOR("www.raycores.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("OI_AMC_V3 P1014 MISC DRIVER");

