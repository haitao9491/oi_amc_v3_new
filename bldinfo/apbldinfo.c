/*
 * (C) Copyright 2015
 * Beijing HLYT Technology Co., Ltd.
 *
 * apbldinfo.c - A brief introduction goes here.
 *
 */

#include "apbldinfo.h"

const char *ApGetBldInfo(void)
{
	return BLD_CONFIG " " BLD_PATH " " BLD_VERSION " " BLD_USER " " BLD_DATETIME " " BLD_PLATFORM;
}

const char *ApGetBldConfig(void)
{
	return BLD_CONFIG;
}

const char *ApGetBldPath(void)
{
	return BLD_PATH;
}

const char *ApGetBldVersion(void)
{
	return BLD_VERSION;
}

const char *ApGetBldUser(void)
{
	return BLD_USER;
}

const char *ApGetBldDatetime(void)
{
	return BLD_DATETIME;
}

const char *ApGetBldPlatform(void)
{
	return BLD_PLATFORM;
}

