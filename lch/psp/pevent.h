/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * event.h - A description goes here.
 *
 */

#ifndef _HEAD_EVENT_05624582_03D211D0_7B9051DE_H
#define _HEAD_EVENT_05624582_03D211D0_7B9051DE_H

#include "pkt.h"

#define EVENT_MSG_CONNECT	0x2380
#define EVENT_MSG_DISCONNECT	0x2381
#define EVENT_MSG_HANDOVER_IN	0x2382
#define EVENT_MSG_HANDOVER_OUT	0x2383

#define SIGMSG_MSG_ID 0x2400

struct event_msg {
	unsigned short id;
	unsigned long long slinkid1;
	unsigned long long slinkid2;
	unsigned int opc;
	unsigned int dpc;
	unsigned int vlink;
	unsigned char vchannel;
	unsigned long long r_slinkid1;
	unsigned long long r_slinkid2;
	unsigned int r_opc;
	unsigned int r_dpc;
};

#if defined(__cplusplus)
extern "C" {
#endif

int event_decode_msg(pkt_hdr *ph, struct event_msg *msg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_EVENT_05624582_03D211D0_7B9051DE_H */
