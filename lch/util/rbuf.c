/*
 *
 * rbuf.c - Implementation of ring buffer
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <endian.h>
#endif
#ifndef DEBUG
#define NLGWR
#endif
#include "aplog.h"
#if defined(RBUF_SYNCED)
#include "mutex.h"
#endif
#include "rbuf.h"

#if defined(__BYTE_ORDER)
#if __BYTE_ORDER == __BIG_ENDIAN
#define RBUF_HANDLE_MAGIC 0x7e5a00ff
#else
#define RBUF_HANDLE_MAGIC 0xff005a7e
#endif
#else
#if defined(__BIG_ENDIAN)
#define RBUF_HANDLE_MAGIC 0x7e5a00ff
#else
#define RBUF_HANDLE_MAGIC 0xff005a7e
#endif
#endif

#if defined(RBUF_SYNCED)
#define RBUF_LOCK_INIT(rh) (rh)->lock = mutex_open(NULL)
#define RBUF_LOCK(rh)      mutex_lock((rh)->lock)
#define RBUF_UNLOCK(rh)    mutex_unlock((rh)->lock)
#define RBUF_LOCK_EXIT(rh) mutex_close((rh)->lock)
#else
#define RBUF_LOCK_INIT(rh) do { } while (0)
#define RBUF_LOCK(rh)      do { } while (0)
#define RBUF_UNLOCK(rh)    do { } while (0)
#define RBUF_LOCK_EXIT(rh) do { } while (0)
#endif

typedef struct {
	int       magic;
	rbuf_hdr *h;
	char     *p;
	rbuf_op   op;
#if defined(RBUF_SYNCED)
	void *lock;
#endif
} rbuf_handle;

static __inline int rbuf_data_len(rbuf_hdr *rhdr, int *l1, int *l2)
{
#if 0
	if(rhdr->r == rhdr->w) {
		*l1 = 0;
		*l2 = 0;
	}
	else {
		if(rhdr->r < rhdr->w) {
			*l1 = rhdr->w - rhdr->r;
			*l2 = 0;
		}
		else {
			*l1 = rhdr->ps - rhdr->r;
			*l2 = rhdr->w;
		}
	}
#else
	int lr, lw;

	lr = rhdr->r;
	lw = rhdr->w;

	if (lr == lw) {
		*l1 = 0;
		*l2 = 0;
	}
	else {
		if (lr < lw) {
			*l1 = lw - lr;
			*l2 = 0;
		}
		else {
			*l1 = rhdr->ps - lr;
			*l2 = lw;
		}
	}
#endif

	return *l1 + *l2;
}

static __inline int rbuf_free_len(rbuf_hdr *rhdr, int *l1, int *l2)
{
#if 0
	if(rhdr->w < rhdr->r) {
		*l1 = rhdr->r - rhdr->w - RBUF_ALIGNMENT;
		*l2 = 0;
	}
	else if(rhdr->r == 0) {
		*l1 = rhdr->ps - rhdr->w - RBUF_ALIGNMENT;
		*l2 = 0;
	}
	else {
		*l1 = rhdr->ps - rhdr->w;
		*l2 = rhdr->r - RBUF_ALIGNMENT;
	}
#else
	int lr, lw;

	lr = rhdr->r;
	lw = rhdr->w;

	if (lw < lr) {
		*l1 = lr - lw - RBUF_ALIGNMENT;
		*l2 = 0;
	}
	else if (lr == 0) {
		*l1 = rhdr->ps - lw - RBUF_ALIGNMENT;
		*l2 = 0;
	}
	else {
		*l1 = rhdr->ps - lw;
		*l2 = lr - RBUF_ALIGNMENT;
	}
#endif

	return *l1 + *l2;
}

static __inline int rbuf_pkt_ahead(rbuf_handle *rh, rbuf_hdr *rhdr)
{
	int  dlen, l1, l2;

	if((dlen = rbuf_data_len(rhdr, &l1, &l2)) == 0)
		return 0;

	return rh->op.verify(rh->p + rhdr->r, dlen);
}

static __inline void rbuf_copy_from_user(rbuf_handle *rh, char *data, int len, int l1, int update)
{
	rbuf_hdr *rhdr = rh->h;

	if(len <= l1) {
		memcpy(rh->p + rhdr->w, data, len);
		LOGDEBUG3("RBUF: push: r %d, w %d, copy %d",
				rhdr->r, rhdr->w, len);
	}
	else {
		memcpy(rh->p + rhdr->w, data, l1);
		LOGDEBUG3("RBUF: push: r %d, w %d, copy %d",
				rhdr->r, rhdr->w, l1);
		memcpy(rh->p, data + l1, len - l1);
		LOGDEBUG1("RBUF: push: copy %d to start of rbuf", len - l1);
	}

	if(!update)
		return;

#if 0
	rhdr->w += ((len + RBUF_ALIGNMENT - 1) & (~(RBUF_ALIGNMENT - 1)));
	if(rhdr->w >= rhdr->ps)
		rhdr->w -= rhdr->ps;
#else
	rhdr->w = (rhdr->w + ((len + RBUF_ALIGNMENT - 1) & (~(RBUF_ALIGNMENT - 1)))) % rhdr->ps;
#endif
	LOGDEBUG2("RBUF: push: r %d, w %d", rhdr->r, rhdr->w);
}

static __inline void rbuf_copy_to_user(rbuf_handle *rh, char *buf, int len, int l1, int update)
{
	rbuf_hdr *rhdr = rh->h;

	if(len <= l1) {
		memcpy(buf, rh->p + rhdr->r, len);
		LOGDEBUG3("RBUF: pop: r %d, w %d, copy %d",
				rhdr->r, rhdr->w, len);
	}
	else {
		memcpy(buf, rh->p + rhdr->r, l1);
		LOGDEBUG3("RBUF: pop: r %d, w %d, copy %d",
				rhdr->r, rhdr->w, l1);
		memcpy(buf + l1, rh->p, len - l1);
		LOGDEBUG1("RBUF: pop: copy %d from start of rbuf", len - l1);
	}

	if(!update)
		return;

#if 0
	rhdr->r += ((len + RBUF_ALIGNMENT - 1) & (~(RBUF_ALIGNMENT - 1)));
	if(rhdr->r >= rhdr->ps)
		rhdr->r -= rhdr->ps;
#else
	rhdr->r = (rhdr->r + ((len + RBUF_ALIGNMENT - 1) & (~(RBUF_ALIGNMENT - 1)))) % rhdr->ps;
#endif
	LOGDEBUG2("RBUF: pop: r %d, w %d", rhdr->r, rhdr->w);
}

static __inline void rbuf_pop_stream_by_count_nolock(void *h, int count)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	rbuf_hdr    *rhdr;
	int          l1, l2;

	if(count & (RBUF_ALIGNMENT - 1))
		return;

	if(!rh || (rh->magic != RBUF_HANDLE_MAGIC))
		return;

	rhdr = rh->h;

	if(rbuf_data_len(rhdr, &l1, &l2) < count)
		return;

#if 0
	rhdr->r += count;
	if(rhdr->r >= rhdr->ps)
		rhdr->r -= rhdr->ps;
#else
	rhdr->r = (rhdr->r + count) % rhdr->ps;
#endif
	LOGDEBUG2("RBUF: pop stream: r %d, w %d", rhdr->r, rhdr->w);
}

static __inline int rbuf_prefetch_stream_nolock(void *h, char *buf, int bufsize)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	rbuf_hdr    *rhdr;
	int          len, l1, l2;

	if(!rh || (rh->magic != RBUF_HANDLE_MAGIC))
		return -1;

	bufsize &= RBUF_ALIGNMENT_MASK;
	if(bufsize <= 0)
		return 0;

	rhdr = rh->h;

	len = rbuf_data_len(rhdr, &l1, &l2);
	if(len == 0)
		return 0;

	if(len > bufsize)
		len = bufsize;

	rbuf_copy_to_user(rh, buf, len, l1, 0);

	return len;
}

void *rbuf_attach(char *buf, rbuf_op *op)
{
	rbuf_handle *rh;
	rbuf_hdr    *rhdr;

	if(!buf) {
		LOGERROR("RBUF: try attaching to a NULL pointer.");
		return NULL;
	}

	if(!op) {
		LOGERROR("RBUF: no op function pointer.");
		return NULL;
	}

	rhdr = (rbuf_hdr *)buf;
	if(rhdr->magic != RBUF_MAGIC) {
		LOGERROR("RBUF: try attaching to an unknown buffer.");
		return NULL;
	}

	if((rhdr->size < sizeof(rbuf_hdr)) ||
	   (rhdr->ps != (rhdr->size - sizeof(rbuf_hdr)))) {
		LOGERROR2("RBUF: invalid hdr, size %d, ps %d.",
				rhdr->size, rhdr->ps);
		return NULL;
	}

	if((rhdr->r < 0) || (rhdr->r >= rhdr->ps) ||
	   (rhdr->w < 0) || (rhdr->w >= rhdr->ps)) {
		LOGERROR3("RBUF: invalid hdr, r %d, w %d, ps %d.",
				rhdr->r, rhdr->w, rhdr->ps);
		return NULL;
	}

	rh = (rbuf_handle *)malloc(sizeof(rbuf_handle));
	if(rh == NULL) {
		LOGERROR("RBUF: insufficient memory.");
		return NULL;
	}

	rh->magic     = RBUF_HANDLE_MAGIC;
	rh->h         = rhdr;
	rh->p         = (char *)(rhdr + 1);
	rh->op.verify = op->verify;
	RBUF_LOCK_INIT(rh);

	return rh;
}

void *rbuf_init(char *buf, int size, rbuf_op *op)
{
	rbuf_hdr    *rhdr;

	size = size & (~(RBUF_ALIGNMENT - 1));
	if(size < sizeof(rbuf_hdr))
		return NULL;

	rhdr = (rbuf_hdr *)buf;
	rhdr->magic = RBUF_MAGIC;
	rhdr->size  = size;
	rhdr->r     = 0;
	rhdr->w     = 0;
	rhdr->ps    = rhdr->size - sizeof(rbuf_hdr);

	return rbuf_attach(buf, op);
}

int rbuf_empty(void *h)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	int flag;

	RBUF_LOCK(rh);
	flag = (rh->h->r == rh->h->w);
	RBUF_UNLOCK(rh);

	return flag;
}

/* Return value:
 *
 * zero - rbuf is not able to receive 'len' bytes of data
 * nonzero - ABLE
 */
int rbuf_capable(void *h, int len)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	int          l1, l2;
	int          freelen;

	if(!rh || (rh->magic != RBUF_HANDLE_MAGIC))
		return 0;

	RBUF_LOCK(rh);
	freelen = rbuf_free_len(rh->h, &l1, &l2);
	RBUF_UNLOCK(rh);

	return (len <= freelen);
}


int rbuf_push(void *h, char *data, int len)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	int          l1, l2;

	if(!data || (len <= 0))
		return -1;

	if(!rh || (rh->magic != RBUF_HANDLE_MAGIC))
		return -2;

	RBUF_LOCK(rh);
	if(len > rbuf_free_len(rh->h, &l1, &l2)) {
		RBUF_UNLOCK(rh);
		return 0;
	}

	rbuf_copy_from_user(rh, data, len, l1, 1);
	RBUF_UNLOCK(rh);

	return len;
}

int rbuf_pop(void *h, char *buf, int bufsize)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	rbuf_hdr    *rhdr;
	int          len, dlen, l1, l2;

	if(!rh || (rh->magic != RBUF_HANDLE_MAGIC)) {
		return 0;
	}

	rhdr = rh->h;

	RBUF_LOCK(rh);
	dlen = rbuf_data_len(rhdr, &l1, &l2);
	if(dlen == 0) {
		RBUF_UNLOCK(rh);
		return 0;
	}

	len = rh->op.verify(rh->p + rhdr->r, dlen);
	if(len > 0) {
		if((len > bufsize) || !buf) {
			/* silently discard this packet */
			len = ((len+RBUF_ALIGNMENT-1) & (~(RBUF_ALIGNMENT-1)));
			rbuf_pop_stream_by_count_nolock(h, len);
			RBUF_UNLOCK(rh);
			LOGERROR1("rbuf_pop: packet(%d) dropped.", len);
			return 0;
		}

		rhdr->err = 0;

		rbuf_copy_to_user(rh, buf, len, l1, 1);
		RBUF_UNLOCK(rh);

		return len;
	}
	RBUF_UNLOCK(rh);

	if(len == 0) {
		rhdr->err = rhdr->err + 1;
		if(rhdr->err == 2000) {
			rhdr->err = 0;
			LOGERROR("error count reached upper limit, recover!");
			rbuf_recover(rh);
		}
	}
	else {
		/* corrupted data in rbuf */
		LOGERROR("corrupted data in rbuf, recover!");
		rbuf_recover(rh);
	}

	return 0;
}

void rbuf_recover(void *h)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	rbuf_hdr    *rhdr;
	int skipped = 0;

	if(!rh || (rh->magic != RBUF_HANDLE_MAGIC))
		return;

	rhdr = rh->h;

	RBUF_LOCK(rh);
	do {
#if 0
		rhdr->r += RBUF_ALIGNMENT;
		if(rhdr->r >= rhdr->ps)
			rhdr->r -= rhdr->ps;
#else
		rhdr->r = (rhdr->r + RBUF_ALIGNMENT) % rhdr->ps;
#endif
		skipped += RBUF_ALIGNMENT;
	} while(rbuf_pkt_ahead(rh, rhdr) < 0);
	RBUF_UNLOCK(rh);

	LOGERROR1("rbuf_recover: %d bytes dropped to recover from error.",
			skipped);
}

int rbuf_prefetch_stream(void *h, char *buf, int bufsize)
{
	int rc;

	RBUF_LOCK((rbuf_handle *)h);
	rc = rbuf_prefetch_stream_nolock(h, buf, bufsize);
	RBUF_UNLOCK((rbuf_handle *)h);

	return rc;
}

void rbuf_pop_stream_by_count(void *h, int count)
{
	RBUF_LOCK((rbuf_handle *)h);
	rbuf_pop_stream_by_count_nolock(h, count);
	RBUF_UNLOCK((rbuf_handle *)h);
}

int rbuf_pop_stream(void *h, char *buf, int bufsize)
{
	int rc;

	RBUF_LOCK((rbuf_handle *)h);
	rc = rbuf_prefetch_stream_nolock(h, buf, bufsize);
	if(rc > 0)
		rbuf_pop_stream_by_count_nolock(h, rc);
	RBUF_UNLOCK((rbuf_handle *)h);

	return rc;
}

void rbuf_reset(void *h)
{
	rbuf_handle *rh = (rbuf_handle *)h;

	if(!rh)
		return;

	if(rh->magic != RBUF_HANDLE_MAGIC)
		return;

	RBUF_LOCK(rh);
	rh->h->r = 0;
	rh->h->w = 0;
	RBUF_UNLOCK(rh);
}

void rbuf_stats(void *h, int *freelen, int *datalen, int *r, int *w)
{
	rbuf_handle *rh = (rbuf_handle *)h;
	int l1, l2;

	if(!rh)
		return;

	if(rh->magic != RBUF_HANDLE_MAGIC)
		return;

	RBUF_LOCK(rh);
	*r = rh->h->r;
	*w = rh->h->w;
	*freelen = rbuf_free_len(rh->h, &l1, &l2);
	*datalen = rbuf_data_len(rh->h, &l1, &l2);
	RBUF_UNLOCK(rh);
}

void rbuf_exit(void *h)
{
	LOGINFODECL(int dummy1;)
	LOGINFODECL(int dummy2;)
	rbuf_handle *rh = (rbuf_handle *)h;

	if(!rh)
		return;

	if(rh->magic != RBUF_HANDLE_MAGIC)
		return;

	LOGINFO1("RBUF: Exit with %d bytes of data still in the buffer.",
			rbuf_data_len(rh->h, &dummy1, &dummy2));

	RBUF_LOCK_EXIT(rh);

	free(rh);
}
