/*
 *
 * pktqueue.h - A brief description goes here.
 *
 */

#ifndef _HEAD_PKTQUEUE_32C64338_594FE8E6_5836CE75_H
#define _HEAD_PKTQUEUE_32C64338_594FE8E6_5836CE75_H

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

#define PKTQUEUE_NORMAL        0x00000000
#define PKTQUEUE_MALLOC_BUFFER 0x00000001
#define PKTQUEUE_BUFFER_SORTED 0x00000002

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *pkt_queue_open(unsigned int type, char *name);
extern DLL_APP void  pkt_queue_set_sizelimit(void *hd, unsigned int sizelimit);
extern DLL_APP void  pkt_queue_set_sortlen(void *hd, int sortlen);
extern DLL_APP void  pkt_queue_set_duration(void *hd, int duration);
extern DLL_APP int   pkt_push(void *hd, void *ph, int len);
extern DLL_APP int   pkt_push_nohdr(void *hd, void *ph, int len, unsigned int s, unsigned int ns);
extern DLL_APP void *pkt_pop(void *hd);
extern DLL_APP void *pkt_pop_force(void *hd);
extern DLL_APP void  pkt_queue_get_mem(void *hd, unsigned int *cnt, unsigned int *size);
extern DLL_APP void  pkt_queue_clear(void *hd);
extern DLL_APP void  pkt_queue_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTQUEUE_32C64338_594FE8E6_5836CE75_H */
