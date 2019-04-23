/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * nm_board_itf.h - A description goes here.
 *
 */

#ifndef _HEAD_NM_BOARD_ITF_2E234A66_49CB6724_50116A90_H
#define _HEAD_NM_BOARD_ITF_2E234A66_49CB6724_50116A90_H

#include "pkt.h"

#if defined(__cplusplus)
extern "C" {
#endif

void    *nm_board_open();
pkt_hdr *nm_board_report(void *hd);
pkt_hdr *nm_board_query(void *hd, pkt_hdr *ph);
void     nm_board_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_NM_BOARD_ITF_2E234A66_49CB6724_50116A90_H */
