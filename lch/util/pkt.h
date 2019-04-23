/*
 *
 * pkt.h - Packet structure
 *
 */

#ifndef _HEAD_PKT_21FE8CA9_3EA2D25B_19FC4711_H
#define _HEAD_PKT_21FE8CA9_3EA2D25B_19FC4711_H

#include "pkttype.h"

#ifndef WIN32
#if !defined(_MY_DRIVER_)
#include <arpa/inet.h>
#else
#include <asm/byteorder.h>
#endif
#else
#include <winsock2.h>
#endif

#ifndef DLL_APP
#ifdef WIN32
#ifdef _USRDLL
#define DLL_APP _declspec(dllexport)
#else
#define DLL_APP _declspec(dllimport)
#endif
#else
#define DLL_APP
#endif
#endif

#if !defined(_MY_DRIVER_)
#define PKT_NTOHS ntohs
#define PKT_NTOHL ntohl
#define PKT_HTONS htons
#define PKT_HTONL htonl
#else
#define PKT_NTOHS __be16_to_cpu
#define PKT_NTOHL __be32_to_cpu
#define PKT_HTONS __cpu_to_be16
#define PKT_HTONL __cpu_to_be32
#endif



/* NOTE: THIS PACKET HEADER IS ALWAYS REPRESENTED IN BIG ENDIAN MODE.
 *       YOU HAVE TO CONVERT IT TO NATIVE MODE FIRSTLY.
 *
 * |<----    Total length: pkt_hdr.len * 4    ---->|
 *
 * +-----------+-------------------------------+---+
 * |  pkt_hdr  | payload                       |   |
 * +-----------+-------------------------------+---+
 *                                               ^
 *                                               |
 *   Filler bytes, to ensure 4-byte alignment ---+
 */
typedef struct {
	/* byte 0 */
	unsigned short  sync;    /* synchronization                           */
	unsigned short  len;     /* number of words (4-byte), including hdr   */

	/* byte 4 */
	unsigned char   type;    /* packet type                               */
	unsigned char   subtype; /* packet subtype                            */
	unsigned char   protocol;/* protocol                                  */
	unsigned char   sc;      /* Status & Control: MSB 0 1 2 3 4 5 6 7 LSB */
	                         /* Bit 6-7: Number of filler bytes, 0-3      */
	                         /* Bit 5  : Set if it belongs to a 2M Channel*/
	                         /* Bit 2-4: Usage counter                    */
	                         /* Bit 0-1: Reserved                         */

	/* byte 8 */
	unsigned short  device;  /* unique device identifier                  */
	unsigned short  channel; /* logical channel sequence number in a dev  */

	/* byte 12 */
	unsigned int    ts_s;    /* timestamp, seconds since Epoch            */
	unsigned int    ts_ns;   /* timestamp, nanoseconds, [0, 999,999,999]  */

	/* byte 20 */
} pkt_hdr;

#if defined(__cplusplus)
extern "C" {
#endif

/* Sync */
#define PKT_SYNC_FLAG 0x7e5a

static __inline unsigned short pkthdr_get_sync(pkt_hdr *ph)
{
	return PKT_NTOHS(ph->sync);
}
static __inline unsigned short pkthdr_slow_get_sync(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	return (*p << 8) | *(p + 1);
}
static __inline void pkthdr_set_sync(pkt_hdr *ph)
{
	ph->sync = PKT_HTONS(PKT_SYNC_FLAG);
}
static __inline void pkthdr_slow_set_sync(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	*p++ = 0x7e;
	*p++ = 0x5a;
}
static __inline int pkthdr_valid_sync(pkt_hdr *ph)
{
	return pkthdr_get_sync(ph) == PKT_SYNC_FLAG;
}
static __inline int pkthdr_slow_valid_sync(pkt_hdr *ph)
{
	return pkthdr_slow_get_sync(ph) == PKT_SYNC_FLAG;
}

/* Status & Control */
#define PKT_2M_CHANNEL 0x04
#define PKT_SC_UCHAR_POINTER(ph) (((unsigned char *)(ph)) + 7)
static __inline unsigned char pkthdr_get_sc(pkt_hdr *ph)
{
	return ph->sc;
}
static __inline unsigned char pkthdr_slow_get_sc(pkt_hdr *ph)
{
	return *PKT_SC_UCHAR_POINTER(ph);
}
static __inline int pkthdr_sc_is_2m_channel(pkt_hdr *ph)
{
	return (ph->sc & PKT_2M_CHANNEL) ? 1 : 0;
}
static __inline int pkthdr_slow_sc_is_2m_channel(pkt_hdr *ph)
{
	return (*PKT_SC_UCHAR_POINTER(ph) & PKT_2M_CHANNEL) ? 1 : 0;
}
static __inline void pkthdr_sc_set_2m_channel(pkt_hdr *ph)
{
	ph->sc |= PKT_2M_CHANNEL;
}
static __inline void pkthdr_slow_sc_set_2m_channel(pkt_hdr *ph)
{
	*PKT_SC_UCHAR_POINTER(ph) |= PKT_2M_CHANNEL;
}
static __inline void pkthdr_sc_clr_2m_channel(pkt_hdr *ph)
{
	ph->sc &= ~PKT_2M_CHANNEL;
}
static __inline void pkthdr_slow_sc_clr_2m_channel(pkt_hdr *ph)
{
	*PKT_SC_UCHAR_POINTER(ph) &= ~PKT_2M_CHANNEL;
}
static __inline int pkthdr_sc_get_flen(pkt_hdr *ph)
{
	return (int)(ph->sc & 0x03);
}
static __inline int pkthdr_slow_sc_get_flen(pkt_hdr *ph)
{
	return (int)(*PKT_SC_UCHAR_POINTER(ph) & 0x03);
}
static __inline void pkthdr_sc_clr_flen(pkt_hdr *ph)
{
	ph->sc &= 0xFC;
}
static __inline void pkthdr_slow_sc_clr_flen(pkt_hdr *ph)
{
	*PKT_SC_UCHAR_POINTER(ph) &= 0xFC;
}
static __inline void pkthdr_sc_set_flen(pkt_hdr *ph, int flen)
{
	pkthdr_sc_clr_flen(ph);
	ph->sc |= ((unsigned char)flen & 0x03);
}
static __inline void pkthdr_slow_sc_set_flen(pkt_hdr *ph, int flen)
{
	pkthdr_slow_sc_clr_flen(ph);
	*PKT_SC_UCHAR_POINTER(ph) |= ((unsigned char)flen & 0x03);
}

/* Length */
static __inline unsigned short _pkthdr_get_len(pkt_hdr *ph)
{
	return PKT_NTOHS((ph)->len);
}
static __inline unsigned short _pkthdr_slow_get_len(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	return (*(p + 2) << 8) | *(p + 3);
}
static __inline void _pkthdr_set_len(pkt_hdr *ph, unsigned short len)
{
	ph->len = PKT_HTONS(len);
}
static __inline void _pkthdr_slow_set_len(pkt_hdr *ph, unsigned short len)
{
	unsigned char *p = (unsigned char *)ph;

	*(p + 2) = (len >> 8) & 0xff;
	*(p + 3) = len & 0xff;
}
static __inline int pkthdr_get_len(pkt_hdr *ph)
{
	return (int)_pkthdr_get_len(ph) * 4;
}
static __inline int pkthdr_slow_get_len(pkt_hdr *ph)
{
	return (int)_pkthdr_slow_get_len(ph) * 4;
}
static __inline int pkthdr_get_plen(pkt_hdr *ph)
{
	return pkthdr_get_len(ph) - pkthdr_sc_get_flen(ph);
}
static __inline int pkthdr_slow_get_plen(pkt_hdr *ph)
{
	return pkthdr_slow_get_len(ph) - pkthdr_slow_sc_get_flen(ph);
}
static __inline void pkthdr_set_plen(pkt_hdr *ph, int plen)
{
	int filler;

	if (plen >= (int)sizeof(pkt_hdr)) {
		filler = 4 - ((plen) % 4);
		if (filler >= 4)
			filler = 0;
		pkthdr_sc_set_flen(ph, filler);
		_pkthdr_set_len(ph, (unsigned short)((plen + filler) / 4));
	}
}
static __inline void pkthdr_slow_set_plen(pkt_hdr *ph, int plen)
{
	int filler;

	if (plen >= (int)sizeof(pkt_hdr)) {
		filler = 4 - ((plen) % 4);
		if (filler >= 4)
			filler = 0;
		pkthdr_slow_sc_set_flen(ph, filler);
		_pkthdr_slow_set_len(ph, (unsigned short)((plen + filler) / 4));
	}
}
static __inline int pkthdr_get_dlen(pkt_hdr *ph)
{
	return pkthdr_get_plen(ph) - sizeof(pkt_hdr);
}
static __inline int pkthdr_slow_get_dlen(pkt_hdr *ph)
{
	return pkthdr_slow_get_plen(ph) - sizeof(pkt_hdr);
}
static __inline void pkthdr_set_dlen(pkt_hdr *ph, int datalen)
{
	pkthdr_set_plen(ph, sizeof(pkt_hdr) + datalen);
}
static __inline void pkthdr_slow_set_dlen(pkt_hdr *ph, int datalen)
{
	pkthdr_slow_set_plen(ph, sizeof(pkt_hdr) + datalen);
}

/* Type and subtype */
#include "pkttype.h"
static __inline unsigned char pkthdr_get_type(pkt_hdr *ph)
{
	return ph->type;
}
static __inline unsigned char pkthdr_slow_get_type(pkt_hdr *ph)
{
	return *((unsigned char *)ph + 4);
}
static __inline void pkthdr_set_type(pkt_hdr *ph, unsigned char t)
{
	ph->type = t;
}
static __inline void pkthdr_slow_set_type(pkt_hdr *ph, unsigned char t)
{
	*((unsigned char *)ph + 4) = t;
}
static __inline unsigned char pkthdr_get_subtype(pkt_hdr *ph)
{
	return ph->subtype;
}
static __inline unsigned char pkthdr_slow_get_subtype(pkt_hdr *ph)
{
	return *((unsigned char *)ph + 5);
}
static __inline void pkthdr_set_subtype(pkt_hdr *ph, unsigned char st)
{
	ph->subtype = st;
}
static __inline void pkthdr_slow_set_subtype(pkt_hdr *ph, unsigned char st)
{
	*((unsigned char *)ph + 5) = st;
}
static __inline int pkthdr_type_is_cmd(pkt_hdr *ph)
{
	return (pkthdr_get_type(ph) == PKT_TYPE_CMD);
}
static __inline int pkthdr_slow_type_is_cmd(pkt_hdr *ph)
{
	return (pkthdr_slow_get_type(ph) == PKT_TYPE_CMD);
}
#define TYPE_IS_CMD(type) \
	(((type) >= PKT_TYPE_CMD_MIN) && ((type) <= PKT_TYPE_CMD_MAX))

/* Protocol */
static __inline unsigned char pkthdr_get_protocol(pkt_hdr *ph)
{
	return ph->protocol;
}
static __inline unsigned char pkthdr_slow_get_protocol(pkt_hdr *ph)
{
	return *((unsigned char *)ph + 6);
}
static __inline void pkthdr_set_protocol(pkt_hdr *ph, unsigned char p)
{
	ph->protocol = p;
}
static __inline void pkthdr_slow_set_protocol(pkt_hdr *ph, unsigned char p)
{
	*((unsigned char *)ph + 6) = p;
}

/* Device and channel */
static __inline unsigned short pkthdr_get_device(pkt_hdr *ph)
{
	return PKT_NTOHS(ph->device);
}
static __inline unsigned short pkthdr_slow_get_device(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	return (*(p + 8) << 8) | *(p + 9);
}
static __inline void pkthdr_set_device(pkt_hdr *ph, unsigned short dev)
{
	ph->device = PKT_HTONS(dev);
}
static __inline void pkthdr_slow_set_device(pkt_hdr *ph, unsigned short dev)
{
	unsigned char *p = (unsigned char *)ph;

	*(p + 8) = (dev >> 8) & 0xff;
	*(p + 9) = dev & 0xff;
}
static __inline unsigned short pkthdr_get_channel(pkt_hdr *ph)
{
	return PKT_NTOHS(ph->channel);
}
static __inline unsigned short pkthdr_slow_get_channel(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	return (*(p + 10) << 8) | *(p + 11);
}
static __inline void pkthdr_set_channel(pkt_hdr *ph, unsigned short channel)
{
	ph->channel = PKT_HTONS(channel);
}
static __inline void pkthdr_slow_set_channel(pkt_hdr *ph, unsigned short channel)
{
	unsigned char *p = (unsigned char *)ph;

	*(p + 10) = (channel >> 8) & 0xff;
	*(p + 11) = channel & 0xff;
}

/* Timestamp */
static __inline unsigned int pkthdr_get_ts_s(pkt_hdr *ph)
{
	return PKT_NTOHL(ph->ts_s);
}
static __inline unsigned int pkthdr_slow_get_ts_s(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	return (*(p+12) << 8) | (*(p+13) << 8) | (*(p+14) << 8) | *(p+15);
}
static __inline unsigned int pkthdr_get_ts_ns(pkt_hdr *ph)
{
	return PKT_NTOHL(ph->ts_ns);
}
static __inline unsigned int pkthdr_slow_get_ts_ns(pkt_hdr *ph)
{
	unsigned char *p = (unsigned char *)ph;

	return (*(p+16) << 8) | (*(p+17) << 8) | (*(p+18) << 8) | *(p+19);
}
static __inline void pkthdr_get_ts(pkt_hdr *ph, unsigned int *s, unsigned int *ns)
{
	*s  = pkthdr_get_ts_s(ph);
	*ns = pkthdr_get_ts_ns(ph);
}
static __inline void pkthdr_slow_get_ts(pkt_hdr *ph, unsigned int *s, unsigned int *ns)
{
	*s  = pkthdr_slow_get_ts_s(ph);
	*ns = pkthdr_slow_get_ts_ns(ph);
}
static __inline void pkthdr_set_ts_s(pkt_hdr *ph, unsigned int s)
{
	ph->ts_s = PKT_HTONL(s);
}
static __inline void pkthdr_slow_set_ts_s(pkt_hdr *ph, unsigned int s)
{
	unsigned char *p = (unsigned char *)ph;

	*(p + 12) = (s  >> 24) & 0xff;
	*(p + 13) = (s  >> 16) & 0xff;
	*(p + 14) = (s  >> 8 ) & 0xff;
	*(p + 15) = (s  >> 0 ) & 0xff;
}
static __inline void pkthdr_set_ts_ns(pkt_hdr *ph, unsigned int ns)
{
	ph->ts_ns = PKT_HTONL(ns);
}
static __inline void pkthdr_slow_set_ts_ns(pkt_hdr *ph, unsigned int ns)
{
	unsigned char *p = (unsigned char *)ph;

	*(p + 16) = (ns >> 24) & 0xff;
	*(p + 17) = (ns >> 16) & 0xff;
	*(p + 18) = (ns >> 8 ) & 0xff;
	*(p + 19) = (ns >> 0 ) & 0xff;
}
static __inline void pkthdr_set_ts(pkt_hdr *ph, unsigned int s, unsigned int ns)
{
	pkthdr_set_ts_s(ph, s);
	pkthdr_set_ts_ns(ph, ns);
}
static __inline void pkthdr_slow_set_ts(pkt_hdr *ph, unsigned int s, unsigned int ns)
{
	pkthdr_slow_set_ts_s(ph, s);
	pkthdr_slow_set_ts_ns(ph, ns);
}

/* Pointer */
static __inline unsigned char *pkthdr_get_data(pkt_hdr *ph)
{
	return (unsigned char *)(ph + 1);
}
static __inline pkt_hdr *pkthdr_next(pkt_hdr *ph)
{
	return (pkt_hdr *)((unsigned char *)ph + pkthdr_get_plen(ph));
}

/* Others */
static __inline int pkt_verify(void *buf, int size)
{
	pkt_hdr *ph = (pkt_hdr *)buf;
	int      i;

	if (!ph) {
		return -1;
	}

	if (size < (int)sizeof(pkt_hdr))
		return 0;

	i = pkthdr_valid_sync(ph);
	if (!i) {
		return -1;
	}

	i = pkthdr_get_plen(ph);
	if(i < (int)sizeof(pkt_hdr)) {
		return -1;
	}

	if ((size > 0) && (i > size)) {
		return 0;
	}

	return i;
}
static __inline int pkt_verify_slow(void *buf, int size)
{
	pkt_hdr *ph = (pkt_hdr *)buf;
	int      i;

	if (!ph) {
		return -1;
	}

	if (size < (int)sizeof(pkt_hdr))
		return 0;

	i = pkthdr_slow_valid_sync(ph);
	if (!i) {
		return -1;
	}

	i = pkthdr_slow_get_plen(ph);
	if(i < (int)sizeof(pkt_hdr)) {
		return -1;
	}

	if ((size > 0) && (i > size)) {
		return 0;
	}

	return i;
}

#if !defined(_MY_DRIVER_)
extern void *pkt_open_file(char *filename, char *mode, int compression);
extern void *pkt_read_file(void *pfp);
extern int pkt_write_file(void *pfp, char *data, int len);
extern void pkt_close_file(void *pfp);
extern void pkt_dump(char *file, char *data, int len);
extern char *display_time(unsigned int s, unsigned int ns, char *usrbuf);
#endif

extern int pkt_generate(unsigned char *buf, int size);

DLL_APP int pkthdr_get_pkt_len(pkt_hdr *ph);


#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKT_21FE8CA9_3EA2D25B_19FC4711_H */
