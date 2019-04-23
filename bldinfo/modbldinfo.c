/*
 * (C) Copyright 2015
 *
 * modbldinfo.c - A brief introduction goes here.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include "bldpath.h"
#include "bldver.h"
#include "bldsinfo.h"

MODULE_VERSION(BLD_CONFIG " " BLD_PATH " " BLD_VERSION " " BLD_USER " " BLD_DATETIME " " BLD_PLATFORM);

