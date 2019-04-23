/*
 * (C) Copyright 2013
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * nm_board_phy.h - A description goes here.
 *
 */



#ifndef _HEAD_NMTYPE_46B87591_56BA1C88_4CFC1D34_H
#define _HEAD_NMTYPE_46B87591_56BA1C88_4CFC1D34_H
#include "nm_board.h"

typedef struct {
	/* amc oiv3 devfd */
	void *hd;

} nmboard_handle;

struct nmpkt_hdr_impl {
	nmpkt_hdr nmpkt_h;
	char data[0];
};

#define NM_OI_PORT_COUNT     4
typedef struct {
	unsigned char los;
	unsigned char lof;
	unsigned char stm1;
	unsigned char e1cnt;
	unsigned int  sig64kchcnt;
	unsigned int  sig64kfrcnt;
	unsigned int  sig2mchcnt;
	unsigned int  sig2mfrcnt;
} nm_oisdh_port_status;

typedef struct {
	nm_oisdh_port_status oisdh[NM_OI_PORT_COUNT];
} nm_oisdh_port_status_t;


#define NM_OI_FLOW_PORT_COUNT  1
typedef struct {
	unsigned int flow[NM_OI_FLOW_PORT_COUNT];
} nm_oisdh_port_flow_t;

void *nmboard_oiamcv3_open();
pkt_hdr *nmboard_oiamcv3_report(void *hd);
void nmboard_oiamcv3_close(void *hd);
pkt_hdr *nmboard_oiamcv3_query(void *hd, pkt_hdr *ph);

#endif
