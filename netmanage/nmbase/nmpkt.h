/*
 * (C) Copyright 2015
 * <xiumei.wang@raycores.com>
 *
 * nmpkt.h - A description goes here.
 *
 */

#ifndef _HEAD_NMPKT_46B87591_56BA1C88_4CFC1D34_H
#define _HEAD_NMPKT_46B87591_56BA1C88_4CFC1D34_H

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

#include <arpa/inet.h>

#define NMPKT_HDR_MAGIC      0xaa55

#pragma pack(1)

#if 0
typedef struct {
	unsigned short magic;
	unsigned char  type;
	unsigned char  module;
	unsigned char  cmd;
	unsigned char  seq;
	unsigned short len;
} nmpkt_hdr;
#else
typedef struct {
	unsigned short magic;
	unsigned char  module;
	unsigned char  cmd;
	unsigned short seq;
	unsigned short len;
} nmpkt_hdr;
#endif

typedef struct {
	unsigned char rack;
	unsigned char shelf;
	unsigned char slot;
	unsigned char subslot;
} nm_position;

#pragma pack()

#if defined(__cplusplus)
extern "C" {
#endif

static __inline unsigned short nmpkthdr_get_magic(nmpkt_hdr *nmph)
{
	return ntohs(nmph->magic);
}

static __inline unsigned short nmpkthdr_slow_get_magic(nmpkt_hdr *nmph)
{
	unsigned char *p = (unsigned char *)nmph;

	return (*p << 8) | *(p + 1);
}

static __inline void nmpkthdr_set_magic(nmpkt_hdr *nmph)
{
	nmph->magic = htons(NMPKT_HDR_MAGIC);
}

static __inline void nmpkthdr_slow_set_magic(nmpkt_hdr *nmph)
{
	unsigned char *p = (unsigned char *)nmph;

	*p++ = (NMPKT_HDR_MAGIC >> 8) & 0xff;
	*p++ = (NMPKT_HDR_MAGIC & 0xff);
}

static __inline int nmpkthdr_valid_magic(nmpkt_hdr *nmph)
{
	return nmpkthdr_get_magic(nmph) == NMPKT_HDR_MAGIC;
}

static __inline int nmpkthdr_slow_valid_magic(nmpkt_hdr *nmph)
{
	return nmpkthdr_slow_get_magic(nmph) == NMPKT_HDR_MAGIC;
}

static __inline unsigned char *nmpkthdr_get_data(nmpkt_hdr *nmph)
{
	return (unsigned char *)(nmph + 1);
}

#if 0
static __inline unsigned short nmpkthdr_get_cmd(nmpkt_hdr *nmph)
{
	return ntohs(nmph->cmd);
}

static __inline unsigned short nmpkthdr_slow_get_cmd(nmpkt_hdr *nmph)
{
	unsigned char *p = (unsigned char *)nmph;

	return (*(p + 4) << 8) | *(p + 5);
}

static __inline void nmpkthdr_set_cmd(nmpkt_hdr *nmph, unsigned short cmd)
{
	nmph->cmd = htons(cmd);
}

static __inline void nmpkthdr_slow_set_cmd(nmpkt_hdr *nmph, unsigned short cmd)
{
	unsigned char *p = (unsigned char *)nmph;

	*(p + 4) = (cmd >> 8) & 0xff;
	*(p + 5) = (cmd & 0xff);
}
#else
static __inline unsigned short nmpkthdr_get_seq(nmpkt_hdr *nmph)
{
	return ntohs(nmph->seq);
}

static __inline unsigned short nmpkthdr_slow_get_seq(nmpkt_hdr *nmph)
{
	unsigned char *p = (unsigned char *)nmph;

	return (*(p + 4) << 8) | *(p + 5);
}

static __inline void nmpkthdr_set_seq(nmpkt_hdr *nmph, unsigned short seq)
{
	nmph->seq = htons(seq);
}

static __inline void nmpkthdr_slow_set_seq(nmpkt_hdr *nmph, unsigned short seq)
{
	unsigned char *p = (unsigned char *)nmph;

	*(p + 4) = (seq >> 8) & 0xff;
	*(p + 5) = (seq & 0xff);
}
#endif

static __inline unsigned short nmpkthdr_get_dlen(nmpkt_hdr *nmph)
{
	return ntohs(nmph->len);
}

static __inline unsigned short nmpkthdr_slow_get_dlen(nmpkt_hdr *nmph)
{
	unsigned char *p = (unsigned char *)nmph;

	return (*(p + 6) << 8) | *(p + 7);
}

static __inline void nmpkthdr_set_dlen(nmpkt_hdr *nmph, unsigned short len)
{
	nmph->len = htons(len);
}

static __inline void nmpkthdr_slow_set_dlen(nmpkt_hdr *nmph, unsigned short len)
{
	unsigned char *p = (unsigned char *)nmph;

	*(p + 6) = (len >> 8) & 0xff;
	*(p + 7) = (len & 0xff);
}

static __inline unsigned short nmpkthdr_get_plen(nmpkt_hdr *nmph)
{
	return ntohs(nmph->len) + sizeof(nmpkt_hdr);
}

static __inline unsigned short nmpkthdr_slow_get_plen(nmpkt_hdr *nmph)
{
	unsigned char *p = (unsigned char *)nmph;

	return ((*(p + 6) << 8) | *(p + 7)) + sizeof(nmpkt_hdr);
}

static __inline void nmpkthdr_construct_hdr(nmpkt_hdr *nmph, 
		unsigned char module, unsigned char cmd, unsigned short seq, unsigned short dlen)
{
	nmpkthdr_set_magic(nmph);
	nmph->module = module;
	nmph->cmd    = cmd;
	nmpkthdr_set_seq(nmph, seq);
	nmpkthdr_set_dlen(nmph, dlen);
}

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_NMPKT_46B87591_56BA1C88_4CFC1D34_H */

