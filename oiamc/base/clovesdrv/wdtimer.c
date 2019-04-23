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
#include <linux/timer.h>
#include <linux/jiffies.h>
#include "clovesdev.h"
#include "clovesctl.h"
#include "clovesdbg.h"
#include "wdtimer.h"

static struct timer_list	wd_timer;
static int			wdtimerfreq = 200;	/* every 200ms */

static void watchdog_timer_register_to_system(void);

/*
 * enable watchdog
 */
static int watchdog_en_wd(void)
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdat;
	val &= ~(GPIO_WD_EN_BIT);
	cloves_devp->gpiop->gpdat = val;

	CLOVESDBG("enable watchdog! register data is 0x%04x\n", cloves_devp->gpiop->gpdat);
	return 0;
}

/*
 * disable watchdog
 */
static int watchdog_disable_wd(void)
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdat;
	val |= GPIO_WD_EN_BIT;
	cloves_devp->gpiop->gpdat = val;

	CLOVESDBG("disable watchdog! register data is 0x%04x\n", cloves_devp->gpiop->gpdat);
	return 0;
}

/*
 * feed watchdog
 */
static int watchdog_feed_wd(void)
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdat;
	val |= GPIO_WD_FEED_BIT;
	cloves_devp->gpiop->gpdat = val;

	val = cloves_devp->gpiop->gpdat;
	val &= ~GPIO_WD_FEED_BIT;
	cloves_devp->gpiop->gpdat = val;

	return 0;
}

static void watchdog_timer_function(unsigned long arg)
{
	watchdog_feed_wd();
	watchdog_timer_register_to_system();
}

static void watchdog_timer_register_to_system()
{
	init_timer(&wd_timer);
	wd_timer.function = watchdog_timer_function;
	wd_timer.data     = 0;
	wd_timer.expires  = jiffies + (HZ * wdtimerfreq / 1000);
	add_timer(&wd_timer);
}

void watchdog_timer_init()
{
	watchdog_feed_wd();
	watchdog_en_wd();
	watchdog_timer_register_to_system();
}

void watchdog_timer_exit()
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdat;
	val |= GPIO_WD_FEED_BIT;
	cloves_devp->gpiop->gpdat = val;

	del_timer_sync(&wd_timer);
}

