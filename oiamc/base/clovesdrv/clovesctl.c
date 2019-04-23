/*
 * (C) Copyright 2014
 * <www.raycores.com>
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
#include <linux/of_gpio.h>
#include "clovesdbg.h"
#include "ledtimer.h"
#include "wdtimer.h"
#include "clovesdev.h"

static inline void common_sleep(void)
{
	int i = 0;

	while (i < (8 * 1024 * 1024))
		i++;
}

/*
 * memory map
 */
int cloves_memmap(void) 
{
	struct device_node *node;
	struct resource res;

	node = of_find_node_by_name(NULL, "gpio-controller");
	if (node) {
		if (of_address_to_resource(node, 0, &res) == 0) {
			CLOVESINFO("start: %08x, size: %08x", res.start, resource_size(&res));
		}
		else {
			CLOVESERR("addr2res failed");
			return -1;
		}
	}
	else {
		CLOVESERR("GPIO-Controller: not found.");
		return -1;
	}

	cloves_devp->gpiop = ioremap(res.start, resource_size(&res));
	if (!cloves_devp->gpiop) {
		CLOVESERR("ioremap() failed.");
		return -EINVAL;
	}
	CLOVESDBG("GPIO-Controller: %u remap @ %p", res.start, cloves_devp->gpiop);

	return 0;
}

int cloves_unmemmap(void)
{
	iounmap(cloves_devp->gpiop);
	return 0;
}

int cloves_gpio_init(void)
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdir;
	val |= (GPIO_GREEN_BIT | GPIO_WD_FEED_BIT | GPIO_WD_EN_BIT);
	cloves_devp->gpiop->gpdir = val;

	CLOVESDBG("gpio dir is 0x%08x", cloves_devp->gpiop->gpdir);

	return 0;
}

/*
 * usr ctl watchdog enable or disable
 */
int cloves_ctl_watchdog(unsigned long arg)
{
	int __user *p = (int __user *)arg;
	int ctlwd = -1;

	get_user(ctlwd, p);
	CLOVESDBG("ctl wdt get_user data is %d\n", ctlwd);

	if(ctlwd == 1) {    /* enable watchdog */
		watchdog_timer_exit();
		watchdog_timer_init();
	} else{             /* disable watchdog */
		watchdog_timer_exit();
	} 

	return 0;
}

/*
 * usr control cloves state led blink 
 */
int cloves_ctl_green(unsigned long arg)
{
	int __user *p = (int __user*)arg;
	int ctlio;

	get_user(ctlio, p);

	if(ctlio == 1) { /* blink */
		led_timer_exit();
		cloves_devp->ledfreq = 1000;
		led_timer_init();
	} else if(ctlio == 2) { /* fblink */
		led_timer_exit();
		cloves_devp->ledfreq = 100;
		led_timer_init();
	} else { /* stop */
		led_timer_exit();
	}

	return 0;
}

/* 
 * get dial value
 */
int cloves_get_dialcode(unsigned long arg)
{
	u32 val;

	val = cloves_devp->gpiop->gpdat;

	val &= (GPIO_DIAL_1 | GPIO_DIAL_2 | GPIO_DIAL_3 | GPIO_DIAL_4);
	val >>= 22;
	val = (~val) & 0xf;

	put_user(val, (u32 *)arg);

	return 0;
}

