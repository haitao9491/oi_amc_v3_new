/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * event.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "coding.h"
#include "pevent.h"

#define CODING_LINK(ptr, val) { \
	        unsigned char *coding_ptr = (unsigned char *)(ptr); \
	        *coding_ptr++ = ((val) >> 40) & 0xff; \
	        *coding_ptr++ = ((val) >> 32) & 0xff; \
	        *coding_ptr++ = ((val) >> 24) & 0xff; \
	        *coding_ptr++ = ((val) >> 16) & 0xff; \
	        *coding_ptr++ = ((val) >>  8) & 0xff; \
	        *coding_ptr++ = ((val) >>  0) & 0xff; \
	        ptr += 6; \
}

#define DECODE_LINK(ptr, val, len) { \
	        unsigned char *decoding_ptr = (unsigned char *)(ptr); \
	        if ((len) < 6) \
	                return -1; \
	        (val) = ((unsigned long long)*decoding_ptr << 40) | \
	                ((unsigned long long)*(decoding_ptr + 1) << 32) | \
	                ((unsigned long long)*(decoding_ptr + 2) << 24) | \
	                ((unsigned long long)*(decoding_ptr + 3) << 16) | \
	                ((unsigned long long)*(decoding_ptr + 4) <<  8) | \
	                ((unsigned long long)*(decoding_ptr + 5) <<  0); \
	        ptr += 6; \
	        (len) = (len) - 6; \
}

int event_decode_msg(pkt_hdr *ph, struct event_msg *msg)
{
	unsigned char *data = (unsigned char *)pkthdr_get_data(ph);
	int dlen = pkthdr_get_dlen(ph);
	
	msg->id = (pkthdr_get_type(ph) << 8) | pkthdr_get_subtype(ph);

	switch (msg->id) {
		case EVENT_MSG_CONNECT:
		case EVENT_MSG_DISCONNECT:
		case EVENT_MSG_HANDOVER_IN:
		case EVENT_MSG_HANDOVER_OUT:
			{
				DECODE_LINK(data, msg->slinkid1, dlen);
				DECODE_LINK(data, msg->slinkid2, dlen);
				DECODE_32(data, msg->opc, dlen);
				DECODE_32(data, msg->dpc, dlen);
				DECODE_32(data, msg->vlink, dlen);
				DECODE_8(data, msg->vchannel, dlen);
				DECODE_LINK(data, msg->r_slinkid1, dlen);
				DECODE_LINK(data, msg->r_slinkid2, dlen);
				DECODE_32(data, msg->r_opc, dlen);
				DECODE_32(data, msg->r_dpc, dlen);
			}
			break;

		default:
			return -1;
	}

	LGWRDEBUG(ph, pkthdr_get_plen(ph), "EVENT message: id %x", msg->id);

	return 0;
}

