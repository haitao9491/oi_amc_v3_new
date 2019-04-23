/*
 *
 * nmmaptbl.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#else
#pragma warning ( disable : 4312 )
#include <time.h>
#endif
#include "os.h"
#include "aplog.h"
#include "nm_typdef.h"
#include "nmmaptbl.h"

/* shelf physical slot map to logic slot table define */
nmshelfslotmap shelf3u2smap_t[] = {
	{2, 1},
	{4, 2}
};
#define NM_SHELF_3U2S_MAP_CNT    (sizeof(shelf3u2smap_t)/sizeof(nmshelfslotmap))

nmshelfslotmap shelf4u4smap_t[] = {
	{1, 1},
	{3, 2},
	{4, 3},
	{2, 4}
};
#define NM_SHELF_4U4S_MAP_CNT     (sizeof(shelf4u4smap_t)/sizeof(nmshelfslotmap))

nmshelfslotmap shelf6u6smap_t[] = {
	{1, 1},
	{2, 2},
	{3, 3},
	{4, 4},
	{5, 5},
	{6, 6}
};
#define NM_SHELF_6U6S_MAP_CNT     (sizeof(shelf6u6smap_t)/sizeof(nmshelfslotmap))

nmshelfslotmap_t nmshelfmap_t[] = {
	{NM_SHELF_3U_2SLOT, shelf3u2smap_t, NM_SHELF_3U2S_MAP_CNT},
	{NM_SHELF_4U_4SLOT, shelf4u4smap_t, NM_SHELF_4U4S_MAP_CNT},
	{NM_SHELF_6U_6SLOT, shelf6u6smap_t, NM_SHELF_6U6S_MAP_CNT}
};
#define NM_SHELF_MAP_CNT          (sizeof(nmshelfmap_t)/sizeof(nmshelfslotmap_t))

nmshelfslotmap_t *nmmap_get_shelfslot_mapt(int shelftype)
{
	int i;
	nmshelfslotmap_t *nmmapt = nmshelfmap_t;

	for (i = 0; i < NM_SHELF_MAP_CNT; i++) {
		if (shelftype == nmmapt->shelftype) {
			return nmmapt;
		}
		nmmapt++;
	}

	return NULL;
}

int nmmap_shelf_phy_to_logic_slot(nmshelfslotmap_t *nmmapt, unsigned char phy, unsigned char *logic)
{
	nmshelfslotmap *nmmap;
	int i;

	if (!nmmapt)
		return -1;

	nmmap = nmmapt->nmssmap;
	for (i = 0; i < nmmapt->content; i++) {
		if (phy == nmmap->physlot) {
			*logic = nmmap->logicslot;
			return 0;
		}
		nmmap++;
	}

	return -1;
}

int nmmap_shelf_logic_to_phy_slot(nmshelfslotmap_t *nmmapt, unsigned char logic, unsigned char *phy)
{
	nmshelfslotmap *nmmap;
	int i;

	if (!nmmapt)
		return -1;

	nmmap = nmmapt->nmssmap;
	for (i = 0; i < nmmapt->content; i++) {
		if (logic == nmmap->logicslot) {
			*phy = nmmap->physlot;
			return 0;
		}
		nmmap++;
	}

	return -1;
}

