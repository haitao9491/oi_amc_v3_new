/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * bdtype.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bdtype.h"

struct board_typename {
	unsigned int	type;
	char	       *name;
};

static struct board_typename bdtypename[] = {
	{ BOARD_TYPE_EIPB_V2,   "EIPB_V2" },
	{ BOARD_TYPE_MPCB_V3,   "MPCB_V3" },
	{ BOARD_TYPE_MACB_V2,   "MACB_V2" },
	{ BOARD_TYPE_OI_AMC_V3, "OI_AMC_V3" },
	{ BOARD_TYPE_EI_AMC_V1, "EI_AMC_V1" },
	{ BOARD_TYPE_MACB_V3,   "MACB_V3" },
	{ BOARD_TYPE_OI_AMC_V4,   "OI_AMC_V4" },
	{ 0, NULL },
};

int get_boardtype(char *boardname)
{
	struct board_typename *tn;
	int rc = BOARD_TYPE_UNKNOWN;

	if (!boardname)
		return rc;

	tn = &bdtypename[0];
	while (tn->name) {
		if (strcmp(boardname, tn->name) == 0)
			return tn->type;
		tn++;
	}

	return rc;
}

char *get_boardtype_list()
{
	static char boardtype_list[512];
	int len = 0;
	struct board_typename *tn;

	memset(boardtype_list, 0, sizeof(boardtype_list));
	tn = &bdtypename[0];
	while (tn->name) {
		if (len)
			len += sprintf(boardtype_list + len, " ");

		len += sprintf(boardtype_list + len, "%s", tn->name);
		tn++;
	}

	return boardtype_list;
}

