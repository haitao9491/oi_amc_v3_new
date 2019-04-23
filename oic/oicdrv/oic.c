/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oic.c - A description goes here.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include <asm/io.h>
#include "oicdbg.h"
#include "oicdev.h"
#include "oicfops.h"
#include "oicproc.h"
#include "oic.h"

int gfpmode = 0;
module_param(gfpmode, int, 0);
MODULE_PARM_DESC(gfpmode, "GFP MODE");

int fpgadev = 0;
module_param(fpgadev, int, 0);
MODULE_PARM_DESC(fpgadev, "FPGA DEV");

int sdhtype = 0;
module_param(sdhtype, int, 0);
MODULE_PARM_DESC(sdhtype, "Distinguish HP or LP.");

MODULE_AUTHOR("wangxiumei");
MODULE_LICENSE("GPL");

struct oic_device *oicdev = NULL;

static int oic_init_register_pointer(void)
{
	struct device_node *node, *child;
	struct resource res;
    char devname[32];

	/* fpga */
	node = of_find_node_by_name(NULL, "fpga");
	if (node == NULL) {
		OICERR("No fpga node found.");
		return -1;
	}

    memset(devname, 0, sizeof(devname));
    if (fpgadev > 0)
    {
        sprintf(devname, "fpga%d", (fpgadev - 1));
        if((child = of_get_child_by_name(node, devname)) == NULL) {
            OICERR("get %s node failed.", devname);
            return -1;
        }
        if (of_address_to_resource(child, 0, &res) != 0) {
            OICERR("No valid fpga space defined.");
            return -1;
        }
        oicdev->fpga_virt = of_iomap(child, 0);
    }
    else
    {
        sprintf(devname, "fpga");
        if (of_address_to_resource(node, 0, &res) != 0) {
            OICERR("No valid fpga space defined.");
            return -1;
        }
        oicdev->fpga_virt = of_iomap(node, 0);
    }
    
	oicdev->fpga_phys = res.start;
	oicdev->fpga_size = resource_size(&res);
    OICINFO("Get node of %s. %08x remap@ %p size %d", devname, res.start, oicdev->fpga_virt, resource_size(&res));
#if defined(XSCB_V2)
    node = of_find_compatible_node(NULL, NULL, "ryx,XSCB_V2-cpld");
    if (node) {
        if (of_address_to_resource(node, 0, &res) == 0) {
            oicdev->cpld_phys = res.start;
            oicdev->cpld_size = resource_size(&res);
        }
        else {
            OICERR("cpld addr2res failed");
            return -1;
        }
    }
    else {
        OICERR("CPLD: not found.");
        return -1;
    }
	oicdev->cpld_virt = ioremap(res.start, resource_size(&res));
	if (!oicdev->cpld_virt) {
        OICERR("CPLD: ioremap err.");
		return -1;
    }
    OICINFO("Get node of CPLD. %08x remap@ %p size %d", oicdev->cpld_phys, oicdev->cpld_virt, oicdev->cpld_size);
#endif
	return 0;
}

static void oic_exit_register_pointer(void)
{
	if (oicdev->fpga_virt) {
		iounmap(oicdev->fpga_virt);
		oicdev->fpga_virt = 0;
	}
	if (oicdev->cpld_virt) {
		iounmap(oicdev->cpld_virt);
		oicdev->cpld_virt = 0;
	}
}

static int __init oic_init_module(void)
{
	oicdev = (struct oic_device *)kmalloc(sizeof(*oicdev), GFP_KERNEL);
	if (oicdev == NULL) {
		OICERR("failed to allocate oic device");
		return -1;
	}
	memset(oicdev, 0, sizeof(*oicdev));

    oicdev->sdhtype = sdhtype;

	if (oic_init_register_pointer() != 0) {
		OICERR("failed to init register pointer.");
		goto oic_init_module_free_device;
	}

	mutex_init(&oicdev->lock);

	if (oic_fops_init() != 0) {
		OICERR("failed to init fops.");
		goto oic_init_module_exit_register_pointer;
	}

	if (oic_proc_init() != 0) {
		OICERR("failed to init proc.");
		goto oic_init_module_exit_fops;
	}

	OICINFO("inited.");

	return 0;

oic_init_module_exit_fops:
	oic_fops_exit();
oic_init_module_exit_register_pointer:
	oic_exit_register_pointer();
oic_init_module_free_device:
	kfree(oicdev);

	return -1;
}

static void __exit oic_exit_module(void)
{
	if (oicdev) {
		oic_proc_exit();
		oic_fops_exit();
		kfree(oicdev);
	}

	OICINFO("exited.");
}

module_init(oic_init_module);
module_exit(oic_exit_module);

