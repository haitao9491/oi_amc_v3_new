/*
 * (C) Copyright 2015
 *
 * dts_query.c - A description goes here.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/vmalloc.h>

#include "dts_query.h"


int dts_dev_nums(char *devname)
{
	int num = 0;
	struct device_node *node = NULL;

	if (NULL != devname) {
		for_each_node_by_name(node, devname) {
			DEBUG("Full name:%s, name:%s, type:%s\n", of_node_full_name(node), node->name, node->type);
			num++;
		}

		return num;
	}

	return -1;
}

int dts_dev_chind_nums(unsigned int devid, char *devname)
{
	int num = 0, did = 0;
	struct device_node *node = NULL;

	if (NULL != devname) {
		for_each_node_by_name(node, devname)
		{
			if (NULL != node && did == devid) {
				num = of_get_child_count(node);
				DEBUG("Fullname:%s, %d devices.\n", of_node_full_name(node), num);
				return num;
			}
			did++;
		}
	}

	return -1;
}

int dts_dev_phys_addr(char *devname, unsigned int devid,unsigned int childid, unsigned long *addr)
{
	struct device_node *node = NULL, *child = NULL;
	int did = 0, sid = 0;
	struct resource res;

	if (NULL == devname|| NULL == addr)
		return -1;

	memset(&res, 0, sizeof(res));

	for_each_node_by_name(node, devname)
	{
		if ((NULL != node) && (did == devid))
		{
			for_each_child_of_node(node, child) {
				if (sid == childid && NULL != child) {
					if (of_address_to_resource(node, childid, &res) == 0) {
						*addr = res.start;
						DEBUG("Fullname:%s, addr:0x%08lx.\n", of_node_full_name(node), res.start);
						return 0;
					}
					break;
				}
				sid++;
			}
			break;
		}
		did++;
	}

	return -1;
}

int dts_dev_size(char *devname, unsigned int devid, unsigned int childid, unsigned long *size)
{
	struct device_node *node = NULL, *child = NULL;
	int did = 0, sid = 0;
	struct resource res;

	memset(&res, 0, sizeof(res));
	if (NULL != devname && NULL != size) {
		for_each_node_by_name(node, devname)
		{
			if (NULL != node && did == devid) {
				for_each_child_of_node(node, child) {
					if (NULL != child && sid == childid) {
						if (of_address_to_resource(node, childid, &res) == 0) {
							*size = resource_size(&res);
							DEBUG("Fullname:%s, size:0x%08lx.\n", of_node_full_name(node), *size);
							return 0;
						}
						break;
					}
					sid++;
				}
				break;
			}
			did++;
		}
	}

	return -1;
}

#ifdef __KERNEL_OBJECT__
static int __init dts_test_init(void)
{
	int i = 0;
	unsigned int num = 0, cnum = 0;
	unsigned long size = 0;
	unsigned long addr = 0;
	char *fpganame = "fpga";
	char *i2cname = "i2c";


	num = dts_dev_nums(fpganame);
	printk(KERN_INFO "fpga num device %d.\n", num);

	for (i = 0; i < num; i++) {
		cnum = dts_dev_chind_nums(0, fpganame);
		dts_dev_phys_addr(fpganame, i, 0, &addr);
		dts_dev_size(fpganame, i, 0, &size);
		printk(KERN_INFO "%d:%d, %d, %s, 0x%08lx,0x%08lx\n", num, i, cnum, fpganame, size, addr);
	}

	num = dts_dev_nums(i2cname);
	printk(KERN_INFO "i2c:num device %d.\n", num);
	for (i = 0; i < num; i++) {
		cnum = dts_dev_chind_nums(0, "i2c");
		dts_dev_phys_addr(i2cname, i, 0, &addr);
		dts_dev_size(i2cname, i, 0, &size);
		printk(KERN_INFO "%d:%d, %d, %s, 0x%08lx,0x%08lx\n", num, i, cnum, i2cname, size, addr);
	}

	return 0;
}

static void __exit dts_test_exit(void)
{
	printk(KERN_INFO "exit.\n");
}

module_init(dts_test_init);
module_exit(dts_test_exit);
MODULE_DESCRIPTION("kernel module");
MODULE_LICENSE("GPL");
#endif
