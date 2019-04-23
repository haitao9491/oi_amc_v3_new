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
#include "clovesdbg.h"
#include "clovesdev.h"
#include "clovesctl.h"
#include "ledtimer.h"

static struct timer_list	led_timer;
static int			ledflag = 0;		/* led status: 0 - off */

static void led_timer_register_to_system(void);


static void led_green_on (void)
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdat;
	val &= ~(GPIO_GREEN_BIT);
	cloves_devp->gpiop->gpdat = val;

	CLOVESDBG("GPIO5 bit5: on data is 0x%08x", cloves_devp->gpiop->gpdat);
}

static void led_green_off (void)
{
	unsigned int val;

	val = cloves_devp->gpiop->gpdat;
	val |= GPIO_GREEN_BIT;
	cloves_devp->gpiop->gpdat = val;

	CLOVESDBG("GPIO5 bit5: off data is 0x%08x", cloves_devp->gpiop->gpdat);
}

/*
 * ledtimer call this function
 * timer control led blink.
 */
static void led_green_blink(int arg) 
{
	if(arg) { //GPIO5 bit5: 0 -> on
		led_green_on();
	} else { //GPIO5 bit5: 1 -> off
		led_green_off();
	}
}

static void led_timer_function(unsigned long arg)
{
	led_green_blink(ledflag);
	ledflag = ledflag ? 0 : 1;

	led_timer_register_to_system();
}

static void led_timer_register_to_system()
{
	init_timer(&led_timer);
	led_timer.function = led_timer_function;
	led_timer.data     = 0;
	led_timer.expires  = jiffies + (HZ * cloves_devp->ledfreq / 1000);
	add_timer(&led_timer);
}

void led_timer_init()
{
	led_timer_register_to_system();
}

void led_timer_exit()
{
	del_timer_sync(&led_timer);
	led_green_on();
}

