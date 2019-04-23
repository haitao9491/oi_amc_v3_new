/*
 * swima_process.h -  Software ima process.
 *
 */

#ifndef _HEAD_SWIMA_PROCESS_97CCE340_52CC76EF_79D3D5CF_H
#define _HEAD_SWIMA_PROCESS_97CCE340_52CC76EF_79D3D5CF_H

#include "imacfg.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void          *swima_open(IMA_CFG *imacfg);
extern void           swima_set_bufmgr(void *hd, void *bufmgr);
extern int            swima_set_cellbuf(void *hd, char *buf, int size);
extern void           swima_set_timecb(void *hd, 
				void (*get_ts)(unsigned int *s, unsigned *ns, void *arg), void *arg);
extern unsigned char *swima_proc(void *hd, unsigned char *pdata);
extern void           swima_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SWIMA_PROCESS_97CCE340_52CC76EF_79D3D5CF_H */

