/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * psp.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "coding.h"
#include "psp.h"

int psp_decode_msg(pkt_hdr *ph, struct psp_msg *msg)
{
	unsigned char *data = (unsigned char *)pkthdr_get_data(ph);
	int dlen = pkthdr_get_dlen(ph);
	
	msg->id = (pkthdr_get_type(ph) << 8) | pkthdr_get_subtype(ph);
	msg->ph = ph;

	switch (msg->id) {
		case PSP_MSG_REGISTER:
			{
				DECODE_8(data, msg->reg.probetype, dlen);
				DECODE_8(data, msg->reg.cardtype, dlen);
				DECODE_8(data, msg->reg.slot, dlen);
				DECODE_8(data, msg->reg.subslot, dlen);
			}
			break;

		case PSP_MSG_SCAN_INIT:
			{
				DECODE_8(data, msg->scan.initmask, dlen);
				DECODE_8(data, msg->scan.scanmode, dlen);
			}
			break;

		case PSP_MSG_SCAN_START:
		case PSP_MSG_SCAN_STOP:
			{
				DECODE_32(data, msg->scan.index, dlen);
				DECODE_8(data, msg->scan.channel, dlen);
			}
			break;

		case PSP_MSG_SCAN_RESULT:
			{
				DECODE_8(data, msg->scan.scanendflag, dlen);
			}
			break;

		case PSP_MSG_PROBE_LINK_ENABLE:
		case PSP_MSG_PROBE_LINK_DISABLE:
			{
				DECODE_8(data, msg->probe.phylink, dlen);
			}
			break;

		case PSP_MSG_PROBE_CHANNEL_START:
		case PSP_MSG_PROBE_CHANNEL_STOP:
			{
				DECODE_8(data, msg->probe.phylink, dlen);
				DECODE_8(data, msg->probe.channel, dlen);
				DECODE_8(data, msg->probe.phylink1, dlen);
				DECODE_8(data, msg->probe.channel1, dlen);
				DECODE_16(data, msg->probe.vpi, dlen);
				DECODE_16(data, msg->probe.vci, dlen);
			}
			break;

		case PSP_MSG_PROBE_LINK_ITFNOTIFY:
			{
				DECODE_8(data, msg->probe.phylink, dlen);
				DECODE_8(data, msg->probe.itftype, dlen);
			}
			break;

		case PSP_MSG_PROBE_ALLCH_START:
		case PSP_MSG_PROBE_ALLCH_STOP:
			{
				DECODE_8(data, msg->probe.channel, dlen);
				DECODE_16(data, msg->probe.vpi, dlen);
				DECODE_16(data, msg->probe.vci, dlen);
			}
			break;

		case PSP_MSG_STM_MAP_ADD:
		case PSP_MSG_STM_MAP_DEL:
		case PSP_MSG_STM_MAP_RESULT:
			{
				DECODE_8(data, msg->map.inport, dlen);
				DECODE_8(data, msg->map.outport, dlen);
			}
			break;
		case PSP_MSG_STM_MAP_GET:
			{
				DECODE_8(data, msg->map.inport, dlen);
			}
			break;
		case PSP_MSG_BLOCK_DATA:
			{
				DECODE_16(data, msg->block.flag, dlen);
				DECODE_16(data, msg->block.type, dlen);
				DECODE_32(data, msg->block.size, dlen);
				msg->block.buf = data;
			}
			break;
		case PSP_MSG_PHY_STAT:
			{
#if 0
				DECODE_8(data, msg->phy.port, dlen);
				DECODE_8(data, msg->phy.loslof, dlen);
				DECODE_8(data, msg->phy.type, dlen);
				DECODE_8(data, msg->phy.e1synccnt, dlen);
				DECODE_16(data, msg->phy.chcnt64k, dlen);
				DECODE_16(data, msg->phy.chcnt2m, dlen);
				DECODE_32(data, msg->phy.frmcnt64k, dlen);
				DECODE_32(data, msg->phy.frmcnt2m, dlen);
				DECODE_32(data, msg->phy.e1validl, dlen);
				DECODE_32(data, msg->phy.e1validh, dlen);
				DECODE_8(data, msg->phy.endflag, dlen);
#endif
			}
			break;
		case PSP_MSG_CHANNEL_STAT:
			{
				DECODE_8(data, msg->hdlc.port, dlen);
				DECODE_8(data, msg->hdlc.type, dlen);
				DECODE_8(data, msg->hdlc.e1, dlen);
				DECODE_8(data, msg->hdlc.ts, dlen);
				DECODE_8(data, msg->hdlc.endflag, dlen);
			}
			break;
		case PSP_MSG_GET_STAT:
			{
				DECODE_32(data, msg->get.flag, dlen);
			}
			break;
		case PSP_MSG_SILENCE_TIMEOUT:
			{
				DECODE_8(data, msg->silence.timeout, dlen);
			}
			break;
		case PSP_MSG_SILENCE_RANGE:
			{
				DECODE_32(data, msg->silence.range, dlen);
			}
			break;
		case PSP_MSG_SILENCE_RESULT:
			{
				DECODE_16(data, msg->silence.count, dlen);
				if (dlen >= (msg->silence.count * 2)) {
					msg->silence.data = data;
				}
				else {
					LOGERROR("Silence(result): decode dlen:%d lower count:%d", dlen, msg->silence.count);
					return -1;
				}
			}
			break;
		default:
			return -1;
	}

	LGWRDEBUG(ph, pkthdr_get_plen(ph), "PSP message: id %x", msg->id);

	return 0;
}

