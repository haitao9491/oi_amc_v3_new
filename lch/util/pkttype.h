/*
 *
 * pkttype.h - Define packet type, subtype and protocol.
 *
 */

#ifndef _HEAD_PKTTYPE_5B0D06A5_42B8C75C_525AF682_H
#define _HEAD_PKTTYPE_5B0D06A5_42B8C75C_525AF682_H

/* Type and subtype */
#define PKT_TYPE_DATA          0x00
#define PKT_SUBTYPE_MTP        0x00  /* Link/Network Layer Protocol */
#define PKT_SUBTYPE_LAPD       0x01
#define PKT_SUBTYPE_PPP        0x02
#define PKT_SUBTYPE_ATM        0x03
#define PKT_SUBTYPE_ETHERNET   0x04
#define PKT_SUBTYPE_FRAMERELAY 0x05
#define PKT_SUBTYPE_TC         0x06
/* for rawdatasvr */
#define PKT_SUBTYPE_MAP				0x11
#define PKT_SUBTYPE_ABIS			0x12
#define PKT_SUBTYPE_BSSAP			0x13
#define PKT_SUBTYPE_GB				0x14

#define PKT_TYPE_HEARTBEAT     0x01
#define PKT_SUBTYPE_HEARTBEAT_SOCKET  0x01

#define PKT_TYPE_CDR			0x10
#define PKT_SUBTYPE_MAP				0x11
#define PKT_SUBTYPE_ABIS			0x12
#define PKT_SUBTYPE_BSSAP			0x13
#define PKT_SUBTYPE_GB				0x14

#define PKT_TYPE_CALLTRACE      0x11
#define PKT_SUBTYPE_MAP				0x11
#define PKT_SUBTYPE_ABIS			0x12
#define PKT_SUBTYPE_BSSAP			0x13
#define PKT_SUBTYPE_GB				0x14

#define PKT_TYPE_RAWQUERY		0x12
#define PKT_SUBTYPE_MAP				0x11
#define PKT_SUBTYPE_ABIS			0x12
#define PKT_SUBTYPE_BSSAP			0x13
#define PKT_SUBTYPE_GB				0x14

#define PKT_TYPE_CDRQUERY     	0x13
#define PKT_SUBTYPE_MAP				0x11
#define PKT_SUBTYPE_ABIS			0x12
#define PKT_SUBTYPE_BSSAP			0x13
#define PKT_SUBTYPE_GB				0x14

/* Type 0x20 to 0x2f are all reserved for TYPE CMD */
#define PKT_TYPE_CMD_MIN  0x20
#define PKT_TYPE_CMD_MAX  0x2b
#define PKT_TYPE_CMD      PKT_TYPE_CMD_MIN
#define PKT_TYPE_CMDACK   0x08
#define PKT_TYPE_CMDNACK  0x04
#define PKT_SUBTYPE_CMD_GETDEVINFO 0x7d
#define PKT_SUBTYPE_CMD_QUIT       0x7e

#define PKT_TYPE_KEEPALIVE     0xFE

#define PKT_TYPE_UNKNOWN       0xFF
#define PKT_SUBTYPE_UNKNOWN    0xFF

/* Protocol */
#define PKT_PT_AUTO            0x00
#define PKT_PT_ABIS            0x01
#define PKT_PT_MOBIS           0x02
#define PKT_PT_SS7             0x03
#define PKT_PT_GB              0x04

#endif /* #ifndef _HEAD_PKTTYPE_5B0D06A5_42B8C75C_525AF682_H */
