/*
 *
 * rbuf.h - Definitions of ring buffer
 *
 */

#ifndef _HEAD_RBUF_7ADAE726_1AAFCF32_451DA554_H
#define _HEAD_RBUF_7ADAE726_1AAFCF32_451DA554_H
#ifndef WIN32
#include <endian.h>
#endif
#define RBUF_ALIGNMENT      8
#define RBUF_ALIGNMENT_MASK ~(RBUF_ALIGNMENT - 1)

#if defined(__BYTE_ORDER)
#if __BYTE_ORDER == __BIG_ENDIAN
#define RBUF_MAGIC 0x7e5a00ff
#else
#define RBUF_MAGIC 0xff005a7e
#endif
#else
#if defined(__BIG_ENDIAN)
#define RBUF_MAGIC 0x7e5a00ff
#else
#define RBUF_MAGIC 0xff005a7e
#endif
#endif

typedef struct {
	int magic;
	int size;
	int r;
	int w;
	int ps;       /* payload size */
	unsigned int err;
	int res0[2];  /* must change if RBUF_ALIGNMENT modified. */
} rbuf_hdr;

typedef struct {
	int (*verify)(void *, int);
} rbuf_op;

#if defined(__cplusplus)
extern "C" {
#endif

extern void *rbuf_attach(char *rbuf, rbuf_op *rop);
extern void *rbuf_init(char *rbuf, int size, rbuf_op *rop);
extern int   rbuf_empty(void *h);
extern int   rbuf_capable(void *h, int len);
extern int   rbuf_push(void *h, char *data, int len);
extern int   rbuf_pop(void *h, char *buf, int bufsize);
extern int   rbuf_pop_stream(void *h, char *buf, int bufsize);
extern int   rbuf_prefetch_stream(void *h, char *buf, int bufsize);
extern void  rbuf_pop_stream_by_count(void *h, int count);
extern void  rbuf_recover(void *h);
extern void  rbuf_reset(void *h);
extern void  rbuf_stats(void *h, int *freelen, int *datalen, int *r, int *w);
extern void  rbuf_exit(void *h);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_RBUF_7ADAE726_1AAFCF32_451DA554_H */
