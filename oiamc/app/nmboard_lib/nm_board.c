/*
 * (C) Copyright 2013
 *
 * nmboard.c - A description goes here.
 *
 */
#include "nm_board.h"
#include "oiclib.h"

void nm_board_close(void *hd)
{
	LOGDEBUG("OI_AMC_V3 close...");
	nmboard_oiamcv3_close(hd);
}

void *nm_board_open()
{
	LOGDEBUG("OI_AMC_V3 open....");
	return nmboard_oiamcv3_open();
}

pkt_hdr *nm_board_report(void *hd)
{
	LOGDEBUG("OI_AMC_V3 report...");
	return nmboard_oiamcv3_report(hd);
}

pkt_hdr *nm_board_query(void *hd, pkt_hdr *ph)
{
	LOGDEBUG("OI_AMC_V3 query...");
	return nmboard_oiamcv3_query(hd, ph);
}
