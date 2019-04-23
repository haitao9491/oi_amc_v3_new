/*
 * (C) Copyright 2019
 * liye <ye.li@raycores.com>
 *
 * gfplib.h - A description goes here.
 *
 */

#ifndef _HEAD_GFPLIB_55F31CA0_2EDE315B_0D2F4387_H
#define _HEAD_GFPLIB_55F31CA0_2EDE315B_0D2F4387_H

#include <stdint.h>
#include "gfpcommon.h"

#if defined(__cplusplus)
extern "C" {
#endif

void *gfplib_open(uint8_t fpgaid);
int   gfplib_get_au4status(void *hd, uint8_t au4, struct gfp_au4status *status);
int   gfplib_get_linkinfo_begin(void *hd);
int   gfplib_get_linkinfo(void *hd, uint8_t au4, uint8_t e1, struct gfp_linkinfo *info);
int   gfplib_get_linkinfo_end(void *hd);
int   gfplib_set_trial_groups(void *hd, struct gfp_groups *groups, struct gfp_trial_results *results);
int   gfplib_set_groups(void *hd, struct gfp_groups *groups);
int   gfplib_channel_local2global(void *hd, uint8_t local_au4, uint8_t local_e1, struct gfp_linkinfo *info);
int   gfplib_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_GFPLIB_55F31CA0_2EDE315B_0D2F4387_H */
