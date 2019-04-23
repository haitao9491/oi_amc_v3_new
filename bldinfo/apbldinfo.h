/*
 * (C) Copyright 2015
 * Beijing HLYT Technology Co., Ltd.
 *
 * apbldinfo.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_APBLDINFO_21A3DA19_1E38A470_2A6D2F9B_H
#define _HEAD_APBLDINFO_21A3DA19_1E38A470_2A6D2F9B_H

#include "bldpath.h"
#include "bldsinfo.h"
#include "bldver.h"

#if defined(__cplusplus)
extern "C" {
#endif

const char *ApGetBldInfo(void);

const char *ApGetBldConfig(void);
const char *ApGetBldPath(void);
const char *ApGetBldVersion(void);
const char *ApGetBldUser(void);
const char *ApGetBldDatetime(void);
const char *ApGetBldPlatform(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_APBLDINFO_21A3DA19_1E38A470_2A6D2F9B_H */
