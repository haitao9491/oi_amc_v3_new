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

		case PSP_MSG_REGISTER_ACK:
			{
				DECODE_8(data, msg->rg.silencetype, dlen);
				DECODE_16(data, msg->rg.indexlimit, dlen);
				DECODE_8(data, msg->rg.revert, dlen);
				DECODE_8(data, msg->rg.tranen, dlen);
			}
			break;
		case PSP_MSG_REGISTER_UNACK:
            break;

		case PSP_MSG_SCAN_CLEAR:
			{
				DECODE_8(data, msg->scan.clear_mask, dlen);
			}
			break;
 
        case PSP_MSG_SCAN_CLEAR_ACK:
            break;

		case PSP_MSG_NOTIFY_ANM:
		case PSP_MSG_NOTIFY_REL:
			{
				DECODE_16(data, msg->scan.index, dlen);
			}
			break;

		case PSP_MSG_SCAN_RESULT:
			{
				DECODE_8(data, msg->scan.scanendflag, dlen);
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
			}
			break;

		case PSP_MSG_CHANNEL_STAT:
			{
			}
			break;

		case PSP_MSG_GET_STAT:
			{
				DECODE_32(data, msg->get.flag, dlen);
			}
			break;

		case PSP_MSG_CLEAR_INDEX:
			{
				DECODE_16(data, msg->clear.index, dlen);
			}
			break;
		case PSP_MSG_SILENCE_RANGE:
			break;
		case PSP_MSG_SILENCE_RESULT:
			break;

		case PSP_MSG_VCHANNEL_START:
		case PSP_MSG_VCHANNEL_STOP:
			{
				DECODE_8(data, msg->tran.chan, dlen);
				DECODE_8(data, msg->tran.port, dlen);
				DECODE_8(data, msg->tran.e1, dlen);
				DECODE_8(data, msg->tran.ts, dlen);
			}
			break;
		case PSP_MSG_TRAN_ENABLE:
			{
				DECODE_8(data, msg->tran_en.revert, dlen);
				DECODE_8(data, msg->tran_en.enable, dlen);
			}

        case PSP_MSG_CLEAR_ALL_INDEX:
            break;

        case PSP_MSG_RESET_POINTER:
            break;

		default:
			LOGERROR("Unkown msg_id: 0x%x", msg->id);
			return -1;
	}

	LGWRDEBUG(ph, pkthdr_get_plen(ph), "PSP message: id %x", msg->id);

	return 0;
}

