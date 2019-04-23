/*
 *
 * swtc.h - Header file for software TC layer functinality.
 *
 */

#ifndef _HEAD_SWTC_6B58A018_2B332759_5FBFE739_H
#define _HEAD_SWTC_6B58A018_2B332759_5FBFE739_H

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

extern DLL_APP void    *swtc_open(int max_channel, int alpha, int beta);
extern DLL_APP void     swtc_set_bufmgr(void *handle, void *bufmgr);
extern DLL_APP void     swtc_set_descramble(void *handle);
extern DLL_APP int      swtc_push(void *handle, pkt_hdr *ph);
extern DLL_APP pkt_hdr *swtc_proc(void *handle);
extern DLL_APP void     swtc_close(void *handle);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SWTC_6B58A018_2B332759_5FBFE739_H */
