/*
 *
 * pktdecode.h - A brief description goes here.
 *
 */

#ifndef _HEAD_PKTDECODE_72494548_6D017D01_36B1DB0B_H
#define _HEAD_PKTDECODE_72494548_6D017D01_36B1DB0B_H

#include "pkt.h"
#include "pstack.h"

#define PVID_PKT_TYPE_SUBTYPE  0x00000001
#define PVID_PKT_PROTOCOL      0x00000002
#define PVID_PKT_SC            0x00000003
#define PVID_PKT_DEVICE        0x00000004
#define PVID_PKT_CHANNEL       0x00000005
#define PVID_PKT_TS_S          0x00000006
#define PVID_PKT_TS_NS         0x00000007

#if defined(__cplusplus)
extern "C" {
#endif

extern int pkt_decode(struct ps_protocol_handle *p,
		struct ps_decoding_result *result, unsigned char *data,
		int len, int *trailerlen);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTDECODE_72494548_6D017D01_36B1DB0B_H */
