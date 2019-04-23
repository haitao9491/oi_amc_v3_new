/*
 * (C) Copyright 2015
 * Beijing HLYT Technology Co., Ltd.
 *
 * bldinfo.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_BLDINFO_784C49D5_4D8415E9_55B87B3E_H
#define _HEAD_BLDINFO_784C49D5_4D8415E9_55B87B3E_H

#include "bldpath.h"
#include "bldsinfo.h"
#include "bldver.h"

#if defined(__cplusplus)
extern "C" {
#endif

const char *GetBldInfo(void);

const char *GetBldConfig(void);
const char *GetBldPath(void);
const char *GetBldVersion(void);
const char *GetBldUser(void);
const char *GetBldDatetime(void);
const char *GetBldPlatform(void);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_BLDINFO_784C49D5_4D8415E9_55B87B3E_H */

