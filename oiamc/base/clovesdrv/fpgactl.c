/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * fpgactl.c - A description goes here.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include "clovesdev.h"
#include "clovesdbg.h"
#include "cloves.h"
#include "fpgactl.h"

#define FPGACTL_TIMEOUT		(HZ)

static int fpgactl_init_child(struct device_node *node, int index)
{
	struct fpga_info *fpga = &cloves_devp->fpgas[index];
	const char *type_prop;
	const __be32 *pins_prop;
	const __be32 *addr_prop;
	int len;
	struct resource res;
	unsigned int val;

	fpga->index = index;

	if (of_property_read_string(node, "config-type", &type_prop) != 0) {
		CLOVESERR("fpga[%d]: failed to read config-type.", index);
		return -1;
	}
	if (!strcmp(type_prop, "gpio")) {
		fpga->cfg_type = FPGA_CFGTYPE_GPIO;
	}
	else if (!strcmp(type_prop, "cpld")) {
		fpga->cfg_type = FPGA_CFGTYPE_CPLD;
	}
	else {
		CLOVESERR("fpga[%d]: unknown config-type: %s", index, type_prop);
		return -1;
	}

	pins_prop = of_get_property(node, "config-pins", &len);
	if ((pins_prop == NULL) || (len != (5 * sizeof(__be32)))) {
		CLOVESERR("fpga[%d]: no valid config-pins.", index);
		return -1;
	}
	fpga->cfg_cs = be32_to_cpu(pins_prop[0]);
	fpga->cfg_rdwr = be32_to_cpu(pins_prop[1]);
	fpga->cfg_program = be32_to_cpu(pins_prop[2]);
	fpga->cfg_init = be32_to_cpu(pins_prop[3]);
	fpga->cfg_done = be32_to_cpu(pins_prop[4]);

	if (fpga->cfg_type == FPGA_CFGTYPE_CPLD) {
		addr_prop = of_get_property(node, "config-address", &len);
		if ((addr_prop == NULL) || (len != (2 * sizeof(__be32)))) {
			CLOVESERR("fpga[%d]: no valid config-address.", index);
			return -1;
		}
		fpga->cfg_ctrl_addr = ((unsigned char *)cloves_devp->cpldp) + be32_to_cpu(addr_prop[0]);
		fpga->cfg_load_addr = ((unsigned char *)cloves_devp->cpldp) + be32_to_cpu(addr_prop[1]);

		CLOVESINFO("fpga[%d]: %p %p", fpga->index, fpga->cfg_ctrl_addr, fpga->cfg_load_addr);
	}

	if (of_address_to_resource(node, 0, &res) != 0) {
		CLOVESERR("fpga[%d]: no valid mif defined.", index);
		return -1;
	}
	fpga->mif_phys = res.start;
	fpga->mif_size = resource_size(&res);
	fpga->mif_virt = ioremap(fpga->mif_phys, fpga->mif_size);
	if (fpga->mif_virt == NULL) {
		CLOVESERR("fpga[%d]: failed to ioremap.", index);
		return -1;
	}

	/* Set load address for GPIO mode */
	if (fpga->cfg_type == FPGA_CFGTYPE_GPIO) {
		if (cloves_devp->gpiop == NULL) {
			CLOVESERR("fpga[%d]: No valid gpio base address.", index);
			iounmap(fpga->mif_virt);
			fpga->mif_virt = NULL;
			return -1;
		}
		fpga->cfg_load_addr = fpga->mif_virt;

		/* init direction register */
		val = ioread32be(&cloves_devp->gpiop->gpdir);
		val &= ~((1 << fpga->cfg_init) |
			 (1 << fpga->cfg_done));
		val |=  ((1 << fpga->cfg_cs) |
			 (1 << fpga->cfg_rdwr) |
			 (1 << fpga->cfg_program));
		iowrite32be(val, &cloves_devp->gpiop->gpdir);
	}
	
	CLOVESINFO("fpga[%d]: %s: %d %d %d %d %d, %08x remap @ %p, size %d",
			index, 
			fpga->cfg_type == FPGA_CFGTYPE_GPIO ? "gpio" : "cpld",
			fpga->cfg_cs, fpga->cfg_rdwr, fpga->cfg_program, fpga->cfg_init, fpga->cfg_done,
			fpga->mif_phys, fpga->mif_virt, fpga->mif_size);
	
	return 0;
}

static void fpgactl_exit_child(int index)
{
	struct fpga_info *fpga = &cloves_devp->fpgas[index];

	if (fpga->mif_virt) {
		iounmap(fpga->mif_virt);
		fpga->mif_virt = NULL;
	}
}

int fpgactl_init()
{
	struct device_node *node, *child;
	int i;

	node = of_find_node_by_name(NULL, "gpio");
	if (!node) {
		node = of_find_node_by_name(NULL, "gpio-controller");
	}
	if (node) {
		cloves_devp->gpiop = of_iomap(node, 0);
	}

	node = of_find_node_by_name(NULL, "fpga");
	if (node == NULL) {
		CLOVESERR("No fpga node found.");
		return -1;
	}

	cloves_devp->num_fpgas = of_get_child_count(node);
	if (cloves_devp->num_fpgas == 0) {
		CLOVESERR("No valid fpgas defined.");
		return -1;
	}
	else if (cloves_devp->num_fpgas > FPGA_MAX_NUM) {
		CLOVESERR("Too much fpga defined.");
		return -1;
	}

	i = 0;
	for_each_child_of_node(node, child) {
		if (fpgactl_init_child(child, i) != 0) {
			CLOVESERR("failed to init fpga[%d]", i);
			fpgactl_exit();
			return -1;
		}
		i++;
	}

	return 0;
}

static inline void fpgactl_cfg_abort(struct fpga_info *fpga)
{
	unsigned int val;

	if (fpga->cfg_type == FPGA_CFGTYPE_GPIO) {
		val = ioread32be(&cloves_devp->gpiop->gpdat);
		val |= ((1 << fpga->cfg_cs) |
			(1 << fpga->cfg_rdwr));
		iowrite32be(val, &cloves_devp->gpiop->gpdat);
	}
	else {
		/* CPLD */
		val = ~((1 << fpga->cfg_cs) |
			(1 << fpga->cfg_program));
		iowrite8(val, fpga->cfg_ctrl_addr);
	}
}

int fpgactl_cfg_start(unsigned long arg)
{
	struct fpga_cfg_info cfginfo, *cfg = &cfginfo;
	struct fpga_info *fpga;
	unsigned int val;
	unsigned long orig_jiffies;

	memset(&cfginfo, 0, sizeof(cfginfo));
	if (copy_from_user(&cfginfo, (void *)arg, sizeof(cfginfo))) {
		return -EFAULT;
	}
	if (cfg->index > cloves_devp->num_fpgas) {
		return -EINVAL;
	}
	fpga = &cloves_devp->fpgas[cfg->index];

	if (fpga->cfg_type == FPGA_CFGTYPE_GPIO) {
		val = ioread32be(&cloves_devp->gpiop->gpdat);
		val &= ~((1 << fpga->cfg_program));		/* PROGRAM: low */
		val |=  ((1 << fpga->cfg_cs) |			/* CS: high */
			 (1 << fpga->cfg_rdwr));		/* RDWR: high */
		iowrite32be(val, &cloves_devp->gpiop->gpdat);

		orig_jiffies = jiffies;
		while (ioread32be(&cloves_devp->gpiop->gpdat) & (1 << fpga->cfg_init)) {
			schedule();
			if (time_after(jiffies, orig_jiffies + FPGACTL_TIMEOUT)) {
				fpgactl_cfg_abort(fpga);
				return -EFAULT;
			}
		}

		val = ioread32be(&cloves_devp->gpiop->gpdat);
		val |= ((1 << fpga->cfg_program));
		iowrite32be(val, &cloves_devp->gpiop->gpdat);

		orig_jiffies = jiffies;
		while (!(ioread32be(&cloves_devp->gpiop->gpdat) & (1 << fpga->cfg_init))) {
			schedule();
			if (time_after(jiffies, orig_jiffies + FPGACTL_TIMEOUT)) {
				fpgactl_cfg_abort(fpga);
				return -EFAULT;
			}
		}

		val = ioread32be(&cloves_devp->gpiop->gpdat);
		val &= ~((1 << fpga->cfg_cs) |
			 (1 << fpga->cfg_rdwr));
		iowrite32be(val, &cloves_devp->gpiop->gpdat);
	}
	else {
		/* CPLD */
		val = ((1 << fpga->cfg_cs) |			/* CS: high */
		       (1 << fpga->cfg_program));		/* PROGRAM: high */
		iowrite8(val, fpga->cfg_ctrl_addr);

		orig_jiffies = jiffies;
		while (ioread8(fpga->cfg_ctrl_addr) & (1 << fpga->cfg_init)) {
			schedule();
			if (time_after(jiffies, orig_jiffies + FPGACTL_TIMEOUT)) {
				fpgactl_cfg_abort(fpga);
				return -EFAULT;
			}
		}

		val = ((1 << fpga->cfg_cs) |
		      ~(1 << fpga->cfg_program));
		iowrite8(val, fpga->cfg_ctrl_addr);

		orig_jiffies = jiffies;
		while (!(ioread8(fpga->cfg_ctrl_addr) & (1 << fpga->cfg_init))) {
			schedule();
			if (time_after(jiffies, orig_jiffies + FPGACTL_TIMEOUT)) {
				fpgactl_cfg_abort(fpga);
				return -EFAULT;
			}
		}
	}

	return 0;
}

int fpgactl_cfg_load(unsigned long arg)
{
	struct fpga_cfg_info cfginfo, *cfg = &cfginfo;
	struct fpga_info *fpga;
	unsigned char *data;
	unsigned int i;

	memset(&cfginfo, 0, sizeof(cfginfo));
	if (copy_from_user(&cfginfo, (void *)arg, sizeof(cfginfo))) {
		return -EFAULT;
	}
	if (cfg->index > cloves_devp->num_fpgas) {
		return -EINVAL;
	}
	fpga = &cloves_devp->fpgas[cfg->index];

	data = (unsigned char *)vmalloc(cfg->len);
	if (data == NULL) {
		CLOVESERR("failed to allocate data buffer for fpga[%d]", cfg->index);
		return -EFAULT;
	}
	if (copy_from_user(data, cfg->data, cfg->len)) {
		return -EFAULT;
	}
	
	for (i = 0; i < cfg->len; i++) {
		iowrite8(data[i], fpga->cfg_load_addr);
	}

	vfree(data);
	return 0;
}

int fpgactl_cfg_done(unsigned long arg)
{
	struct fpga_cfg_info cfginfo, *cfg = &cfginfo;
	struct fpga_info *fpga;
	unsigned long orig_jiffies;

	memset(&cfginfo, 0, sizeof(cfginfo));
	if (copy_from_user(&cfginfo, (void *)arg, sizeof(cfginfo))) {
		return -EFAULT;
	}
	if (cfg->index > cloves_devp->num_fpgas) {
		return -EINVAL;
	}
	fpga = &cloves_devp->fpgas[cfg->index];

	if (fpga->cfg_type == FPGA_CFGTYPE_GPIO) {
		orig_jiffies = jiffies;
		while (!(ioread32be(&cloves_devp->gpiop->gpdat) & (1 << fpga->cfg_done))) {
			schedule();
			if (time_after(jiffies, orig_jiffies + FPGACTL_TIMEOUT)) {
				return -EFAULT;
			}
		}
	}
	else {
		/* CPLD */
		orig_jiffies = jiffies;
		while (!(ioread8(fpga->cfg_ctrl_addr) & (1 << fpga->cfg_done))) {
			schedule();
			if (time_after(jiffies, orig_jiffies + FPGACTL_TIMEOUT)) {
				return -EFAULT;
			}
		}
	}

	return 0;
}

int fpgactl_cfg_stop(unsigned long arg)
{
	struct fpga_cfg_info cfginfo, *cfg = &cfginfo;
	struct fpga_info *fpga;

	memset(&cfginfo, 0, sizeof(cfginfo));
	if (copy_from_user(&cfginfo, (void *)arg, sizeof(cfginfo))) {
		return -EFAULT;
	}
	if (cfg->index > cloves_devp->num_fpgas) {
		return -EINVAL;
	}
	fpga = &cloves_devp->fpgas[cfg->index];

	fpgactl_cfg_abort(fpga);
	return 0;
}

int fpgactl_get_register(unsigned long arg)
{
	struct fpga_reg freg, *reg = &freg;
	struct fpga_info *fpga;
	int found, i;
	void *addr;

	memset(reg, 0, sizeof(*reg));
	if (copy_from_user(reg, (void *)arg, sizeof(*reg))) {
		return -EFAULT;
	}

	found = 0;
	for (i = 0, fpga = &cloves_devp->fpgas[0]; i < cloves_devp->num_fpgas; i++, fpga++) {
		if ((reg->addr >= fpga->mif_phys) && (reg->addr < (fpga->mif_phys + fpga->mif_size))) {
			found = 1;
			break;
		}
	}
	if (!found) {
		return -EINVAL;
	}

	addr = fpga->mif_virt + (reg->addr - fpga->mif_phys);
	reg->value = ioread8(addr);

	if (copy_to_user((void *)arg, reg, sizeof(*reg))) {
		return -EFAULT;
	}

	return 0;
}

int fpgactl_set_register(unsigned long arg)
{
	struct fpga_reg freg, *reg = &freg;
	struct fpga_info *fpga;
	int found, i;
	void *addr;

	memset(reg, 0, sizeof(*reg));
	if (copy_from_user(reg, (void *)arg, sizeof(*reg))) {
		return -EFAULT;
	}

	found = 0;
	for (i = 0, fpga = &cloves_devp->fpgas[0]; i < cloves_devp->num_fpgas; i++, fpga++) {
		if ((reg->addr >= fpga->mif_phys) && (reg->addr < (fpga->mif_phys + fpga->mif_size))) {
			found = 1;
			break;
		}
	}
	if (!found) {
		return -EINVAL;
	}


	addr = fpga->mif_virt + (reg->addr - fpga->mif_phys);
	iowrite8(reg->value, addr);
	ioread8(addr);			/* To flush write */
	mdelay(10);

	/* readback to verify */
	if (ioread8(addr) != reg->value) {
		return -EFAULT;
	}

	return 0;
}

int fpgactl_get_number(unsigned long arg)
{
	return put_user(cloves_devp->num_fpgas, (unsigned int *)arg);
}

int fpgactl_get_info(unsigned long arg)
{
	struct fpga_info_entry *entries;
	struct fpga_info *fpga;
	int i;

	entries = (struct fpga_info_entry *)vmalloc(cloves_devp->num_fpgas * sizeof(struct fpga_info_entry));
	if (entries == NULL) {
		return -EFAULT;
	}

	for (i = 0, fpga = &cloves_devp->fpgas[0]; i < cloves_devp->num_fpgas; i++, fpga++) {
		entries[i].phys = fpga->mif_phys;
		entries[i].size = fpga->mif_size;
	}

	if (copy_to_user((void *)arg, entries, cloves_devp->num_fpgas * sizeof(struct fpga_info_entry))) {
		vfree(entries);
		return -EFAULT;
	}

	vfree(entries);
	return 0;
}

void fpgactl_exit()
{
	int i;

	for (i = 0; i < cloves_devp->num_fpgas; i++) {
		fpgactl_exit_child(i);
	}
}

