/*
 *
 * rawmgr.h - To manage rawdata.
 *
 */

#ifndef _HEAD_RAWMGR_44B71E2A_7C12570F_7C89A5E8_H
#define _HEAD_RAWMGR_44B71E2A_7C12570F_7C89A5E8_H

#include "pkt.h"

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

#if defined(__cplusplus)
extern "C" {
#endif

struct rawmgr_idx_record {
	unsigned int id;
	unsigned int offset;
	unsigned int pkts;
	unsigned int bytes;
};

DLL_APP void *rawmgr_open(char *path, int period, int concurrent, int buckets);
DLL_APP int rawmgr_write(void *hd, unsigned int starttime, unsigned int id,
		pkt_hdr *ph, int endflag);
DLL_APP int rawmgr_read_info(void *hd, unsigned int starttime,
		unsigned int id, struct rawmgr_idx_record *idx);
DLL_APP int rawmgr_read(void *hd, unsigned int starttime, unsigned int id,
		unsigned char *buf, int *size);
DLL_APP void rawmgr_get_mem(void *hd, unsigned long long *cnt,
		unsigned long long *access, unsigned long long *compares);
DLL_APP void rawmgr_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_RAWMGR_44B71E2A_7C12570F_7C89A5E8_H */
