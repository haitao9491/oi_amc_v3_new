/*
 * (C) Copyright 2015
 * <xiumei.wang@raycores.com>
 *
 * nmmaptbl.h - A description goes here.
 *
 */

#ifndef _HEAD_NMMAPTBL_H
#define _HEAD_NMMAPTBL_H

/* shelf physical slot map to logic slot table define */
typedef struct {
	unsigned char physlot;    /* HA change slot address */
	unsigned char logicslot;  /* shelf paint slot address */
} nmshelfslotmap;

typedef struct {
	int shelftype;
	nmshelfslotmap *nmssmap;
	int content;
} nmshelfslotmap_t;

#if defined(__cplusplus)
extern "C" {
#endif

nmshelfslotmap_t *nmmap_get_shelfslot_mapt(int shelftype);
int nmmap_shelf_phy_to_logic_slot(nmshelfslotmap_t *nmmapt, unsigned char phy, unsigned char *logic);
int nmmap_shelf_logic_to_phy_slot(nmshelfslotmap_t *nmmapt, unsigned char logic, unsigned char *phy);

#if defined(__cplusplus)
}
#endif

#endif
