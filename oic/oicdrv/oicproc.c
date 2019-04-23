/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * oicproc.c - A description goes here.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "oicdbg.h"
#include "oic.h"
#include "oicdev.h"
#include "oicproc.h"

#define OIC_PROC_NAME	"driver/oicdrv"

static struct proc_dir_entry *oic_proc_root = NULL;
extern int fpgadev;

int oic_proc_init()
{
    char devname[32];

	if (oic_proc_root == NULL) {
        if (fpgadev > 0)
        {
            memset(devname, 0, sizeof(devname)); 
            sprintf(devname, "%s%d", OIC_PROC_NAME, fpgadev);
        }
        else
        {
            memset(devname, 0, sizeof(devname)); 
            sprintf(devname, "%s", OIC_PROC_NAME);
        }

		oic_proc_root = proc_mkdir(devname, NULL);
		if (oic_proc_root == NULL) {
			OICERR("failed to make proc dir.");
			return -1;
		}
	}

	/* TODO: driver-specific proc initialization */

	return 0;
}

void oic_proc_exit()
{
	/* TODO: */
    char devname[32];

    if (fpgadev > 0)
        {
            memset(devname, 0, sizeof(devname)); 
            sprintf(devname, "%s%d", OIC_PROC_NAME, fpgadev);
        }
        else
        {
            memset(devname, 0, sizeof(devname)); 
            sprintf(devname, "%s", OIC_PROC_NAME);
        }


	if (oic_proc_root) {
		remove_proc_entry(devname, NULL);
		oic_proc_root = NULL;
	}
}

