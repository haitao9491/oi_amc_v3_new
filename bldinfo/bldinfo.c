/*
 * (C) Copyright 2015
 * Beijing HLYT Technology Co., Ltd.
 *
 * bldinfo.c - A brief introduction goes here.
 *
 */

#include "bldinfo.h"

const char *GetBldInfo(void)
{
	return BLD_CONFIG " " BLD_PATH " " BLD_VERSION " " BLD_USER " " BLD_DATETIME " " BLD_PLATFORM;
}

const char *GetBldConfig(void)
{
	return BLD_CONFIG;
}

const char *GetBldPath(void)
{
	return BLD_PATH;
}

const char *GetBldVersion(void)
{
	return BLD_VERSION;
}

const char *GetBldUser(void)
{
	return BLD_USER;
}

const char *GetBldDatetime(void)
{
	return BLD_DATETIME;
}

const char *GetBldPlatform(void)
{
	return BLD_PLATFORM;
}

