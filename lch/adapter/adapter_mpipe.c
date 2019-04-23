/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * adapter_mpipe.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __tilegx__
#include <tmc/cpus.h>
#include <tmc/alloc.h>
#include <tmc/sync.h>
#include <tmc/mem.h>
#include <tmc/perf.h>
#include <arch/sim.h>
#include <arch/cycle.h>
#include <arch/atomic.h>
#include <gxio/mpipe.h>
#endif
#include "os.h"
#include "aplog.h"
#include "apfrm.h"
#include "cconfig.h"
#include "cthread.h"
#include "pkt.h"
#include "pfifo.h"
#include "misc.h"
#include "adapter.h"
#include "adapter_mpipe.h"

#ifdef __tilegx__

#define ADAPTER_MPIPE_FLAG_TX	0x00000001

#define ADAPTER_MPIPE_MAXLINK	32
#define ADAPTER_MPIPE_MAXQUEUE	32
#define ADAPTER_MPIPE_NAME	"Adapter.mPIPE"

#define ADAPTER_MPIPE_IRING_ENTRIES	2048
#define ADAPTER_MPIPE_ERING_ENTRIES	2048

#define ADAPTER_MPIPE_BUFFER_HEADROOM_SIZE 32
#define ADAPTER_MPIPE_BUFFER_PKTHDR_SIZE   22

struct adapter_mpipe_ctl;

struct adapter_mpipe_link {
	char linkname[64];
	int flag;

	gxio_mpipe_link_t link;
	int channel;
	int link_openned;
};

struct adapter_mpipe_iqueue {
	int id;
	int cpu;
	gxio_mpipe_iqueue_t *iqueue;
	void *iqueue_mem;

	void *thread;
	void *fifo;

	unsigned long long rpkts;
	unsigned long long rbytes;
	unsigned long long rbad;
	unsigned long long rerr;
	unsigned long long rfull;

	unsigned long long mpkts;
	unsigned long long cpkts;

	unsigned long long rpkts_link[ADAPTER_MPIPE_MAXLINK];
	unsigned long long rbytes_link[ADAPTER_MPIPE_MAXLINK];

	struct adapter_mpipe_ctl *parent;
};

struct adapter_mpipe_equeue {
	gxio_mpipe_equeue_t *equeue;
	void *equeue_mem;

	unsigned long long tpkts;
	unsigned long long tbytes;
	unsigned long long terr;

	struct adapter_mpipe_ctl *parent;
	struct adapter_mpipe_link *link;
};

struct adapter_mpipe_ctl {
	struct adapter_mpipe_link links[ADAPTER_MPIPE_MAXLINK];
	int numlinks;

	struct adapter_mpipe_iqueue iqueues[ADAPTER_MPIPE_MAXQUEUE];
	int numiqueues;
	int start_dpcore;

	struct adapter_mpipe_equeue equeues[ADAPTER_MPIPE_MAXQUEUE];
	int numequeues;

	tmc_sync_barrier_t barrier;
	int openned;
	void *priv;
	adap_pkt_cb pkt_cb;

	/* mpipe internals */
	int instance;
	gxio_mpipe_context_t ctx;
	int ctx_initialized;
	int ring;
	int group;
	int bucket;
	int ering;

	int stack;
	void *pages;
	int numpages;

	gxio_mpipe_stats_t prev_stat, current_stat;

	int rx;
};


static int adapter_mpipe_get_dpcore_num()
{
	cpu_set_t cpuset;

	if (tmc_cpus_get_dataplane_cpus(&cpuset) < 0) {
		LOGERROR("Failed to get dataplane cpus.");
		return -1;
	}

	return tmc_cpus_count(&cpuset);
}

static int adapter_mpipe_dpcore2cpu(int dpcore)
{
	cpu_set_t cpuset;

	if (tmc_cpus_get_dataplane_cpus(&cpuset) < 0) {
		LOGERROR("Failed to get dataplane cpus.");
		return -1;
	}

	return tmc_cpus_find_nth_cpu(&cpuset, dpcore);
}

static int adapter_mpipe_set_affinity(int cpu)
{
	return tmc_cpus_set_my_cpu(cpu);
}

static void adapter_mpipe_exit_mpipe(struct adapter_mpipe_ctl *ctl)
{
	int i;
	size_t iring_entries, iring_size;
	size_t ering_entries, ering_size;
	size_t page_size;

	iring_entries = ADAPTER_MPIPE_IRING_ENTRIES;
	ering_entries = ADAPTER_MPIPE_ERING_ENTRIES;

	if (ctl->pages) {
		page_size = tmc_alloc_get_huge_pagesize();
		tmc_alloc_unmap(ctl->pages, page_size * ctl->numpages);
		ctl->pages = NULL;
	}

	ering_size = ering_entries * sizeof(gxio_mpipe_edesc_t);
	for (i = 0; i < ctl->numequeues; i++) {
		if (ctl->equeues[i].equeue) {
			tmc_alloc_unmap(ctl->equeues[i].equeue, sizeof(gxio_mpipe_equeue_t));
			ctl->equeues[i].equeue = NULL;
		}
		if (ctl->equeues[i].equeue_mem) {
			tmc_alloc_unmap(ctl->equeues[i].equeue_mem, ering_size);
			ctl->equeues[i].equeue_mem = NULL;
		}
	}

	iring_size = iring_entries * sizeof(gxio_mpipe_idesc_t);
	for (i = 0; i < ctl->numiqueues; i++) {
		if (ctl->iqueues[i].iqueue) {
			tmc_alloc_unmap(ctl->iqueues[i].iqueue, sizeof(gxio_mpipe_iqueue_t));
			ctl->iqueues[i].iqueue = NULL;
		}
		if (ctl->iqueues[i].iqueue_mem) {
			tmc_alloc_unmap(ctl->iqueues[i].iqueue_mem, iring_size);
			ctl->iqueues[i].iqueue_mem = NULL;
		}
	}

	for (i = 0; i < ctl->numlinks; i++) {
		if (ctl->links[i].link_openned) {
			gxio_mpipe_link_close(&(ctl->links[i].link));
			ctl->links[i].link_openned = 0;
		}
	}

	if (ctl->ctx_initialized) {
		gxio_mpipe_destroy(&ctl->ctx);
		ctl->ctx_initialized = 0;
	}
}

static void adapter_mpipe_exit(struct adapter_mpipe_ctl *ctl)
{
	int i;

	if (ctl) {
		if (ctl->openned == 0) {
			/* make threads to going */
			tmc_sync_barrier_wait(&ctl->barrier);
		}

		for (i = 0; i < ctl->numiqueues; i++) {
			if (ctl->iqueues[i].thread) {
				thread_close(ctl->iqueues[i].thread);
				ctl->iqueues[i].thread = NULL;
			}
			if (ctl->iqueues[i].fifo) {
				pfifo_close(ctl->iqueues[i].fifo);
				ctl->iqueues[i].fifo = NULL;
			}
		}

		adapter_mpipe_exit_mpipe(ctl);

		free(ctl);
	}
}

static void pkt_cb_default(int tid, void *ph, int len,
		char *ip, int port, void *priv)
{
	struct adapter_mpipe_ctl *ctl = NULL;
	void *data = NULL;

	if ((ph == NULL) || (priv == NULL))
		return;

	ctl = (struct adapter_mpipe_ctl *)priv;
	if (ctl->iqueues[tid].fifo) {
		data = pfifo_write(ctl->iqueues[tid].fifo, len);
		if (data) {
			memcpy(data, ph, len);
			pfifo_write_commit(ctl->iqueues[tid].fifo);
		}
		else {
			++(ctl->iqueues[tid].rfull);
		}
	}
}

static void *adapter_mpipe_thread(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	struct adapter_mpipe_iqueue *q = (struct adapter_mpipe_iqueue *)targ->arg;
	struct adapter_mpipe_ctl *ctl = q->parent;
	gxio_mpipe_idesc_t *idesc, *idescs;
	int numpkts, i;
	unsigned char *buf = NULL;
	int buflen = 0;
	int channel;
	unsigned int s = 0, ns = 0;
	pkt_hdr *ph;

	if (adapter_mpipe_set_affinity(q->cpu) < 0) {
		LOGERROR("Thread #%d: Failed to set affinity [%d]", q->id, q->cpu);
		return NULL;
	}

	tmc_sync_barrier_wait(&ctl->barrier);

	LOG("Thread(iqueue) #%d: Start processing data ...", q->id);
	while (targ->flag) {
		if (ctl->rx == 0) {
			if (ap_is_running())
				continue;
			else
				break;
		}

		numpkts = gxio_mpipe_iqueue_try_peek(q->iqueue, &idescs);
		if (numpkts == GXIO_MPIPE_ERR_IQUEUE_EMPTY) {
			continue;
		}

		numpkts = (numpkts > 16) ? 16 : numpkts;
		tmc_mem_prefetch(idescs, numpkts * sizeof(gxio_mpipe_idesc_t));
		for (i = 0, idesc = idescs; i < numpkts; i++, idesc++) {
			if (gxio_mpipe_idesc_is_bad(idesc)) {
				++q->rbad;
				if (gxio_mpipe_idesc_has_error(idesc)) {
					++q->rerr;
				}
			}
			else {
				get_timestamp(&s, &ns);

				buf = gxio_mpipe_idesc_get_va(idesc);
				buflen = gxio_mpipe_idesc_get_xfer_size(idesc);
				channel = idesc->channel;

				++q->rpkts;
				q->rbytes += buflen;

				++(q->rpkts_link[channel]);
				q->rbytes_link[channel] += buflen;

				ph = (pkt_hdr *)(buf - (sizeof(pkt_hdr) + 2));
				buflen += sizeof(pkt_hdr) + 2;
				pkthdr_slow_set_sync(ph);
				pkthdr_slow_set_plen(ph, buflen);
				pkthdr_slow_set_ts(ph, s, ns);
				pkthdr_slow_set_channel(ph, channel);

				if (gxio_mpipe_idesc_get_ethertype(idesc) == 0x9999) {
					q->mpkts++;
				}
				if (gxio_mpipe_idesc_get_vlan(idesc) == 0xffff) {
					if ((buf[12] == 0x99) && (buf[13] == 0x99))
						q->cpkts++;
				}

				if (ctl->pkt_cb) {
					ctl->pkt_cb(q->id, ph, buflen, NULL, 0, ctl->priv);
				}
			}

			gxio_mpipe_iqueue_drop(q->iqueue, idesc);
			gxio_mpipe_iqueue_consume(q->iqueue, idesc);
		}
	}
	LOG("Thread(iqueue) #%d: Stopped. Received: %llu P, %llu B, %llu D, %llu E, %llu F, %llu M, %llu C",
			q->id, q->rpkts, q->rbytes, q->rbad, q->rerr, q->rfull, q->mpkts, q->cpkts);

	return NULL;
}

static inline void adapter_mpipe_init_pkthdr(void *mem)
{
	/* header: 20 bytes
	 *   flag    : 2B, 0x7e5a
	 *   len     : 2B 
	 *   type    : 1B, 0x00
	 *   subtype : 1B, 0x04
	 *   protocol: 1B
	 *   sc      : 1B
	 *   device  : 2B
	 *   channel : 2B
	 *   ts_s    : 4B
	 *   ts_ns   : 4B
	 *
	 * filler: 2 bytes
	 *
	 */

	unsigned char *p = (unsigned char *)mem;

	p += ADAPTER_MPIPE_BUFFER_HEADROOM_SIZE - ADAPTER_MPIPE_BUFFER_PKTHDR_SIZE;
	memset(p, 0, ADAPTER_MPIPE_BUFFER_PKTHDR_SIZE);

	p[0] = 0x7e;
	p[1] = 0x5a;
	p[4] = 0x00;
	p[5] = 0x04;            /* Ethernet frame */
}

static int adapter_mpipe_init_mpipe(struct adapter_mpipe_ctl *ctl)
{
	int i, cpu, rc;
	int instance, channel;
	int num_buckets, num_buffers;
	size_t iring_entries, iring_size;
	size_t ering_entries, ering_size;
	size_t page_size, stack_bytes, buffer_size, tsize;
	void *mem = NULL;
	tmc_alloc_t alloc;
	gxio_mpipe_buffer_size_enum_t buffer_size_enum;
	gxio_mpipe_bucket_mode_t mode;
	gxio_mpipe_rules_t rules;

	struct adapter_mpipe_link *mlink;

	iring_entries = ADAPTER_MPIPE_IRING_ENTRIES;
	ering_entries = ADAPTER_MPIPE_ERING_ENTRIES;
	num_buckets = ctl->numiqueues;
	num_buffers = ctl->numiqueues * iring_entries + ctl->numequeues * ering_entries;

	for (i = 0; i < ctl->numiqueues; i++) {
		ctl->iqueues[i].parent = ctl;
	}
	for (i = 0; i < ctl->numequeues; i++) {
		ctl->equeues[i].parent = ctl;
	}

	if ((ctl->instance = gxio_mpipe_link_instance(ctl->links[0].linkname)) < 0) {
		LOGERROR("Failed to get instance for link %s", ctl->links[0].linkname);
		return -1;
	}
	for (i = 1; i < ctl->numlinks; i++) {
		if ((instance = gxio_mpipe_link_instance(ctl->links[i].linkname)) < 0) {
			LOGERROR("Failed to get instance for link %s", ctl->links[i].linkname);
			return -1;
		}
		if (instance != ctl->instance) {
			LOGERROR("Different instance: %d@%s <> %d@%s",
					instance, ctl->links[i].linkname, 
					ctl->instance, ctl->links[0].linkname);
			return -1;
		}
	}
	LOGDEBUG("instance: %d", ctl->instance);

	if ((rc = gxio_mpipe_init(&ctl->ctx, ctl->instance)) < 0) {
		LOGERROR("Failed to init mpipe: %d", rc);
		return -1;
	}
	ctl->ctx_initialized = 1;

	for (i = 0; i < ctl->numlinks; i++) {
		if ((rc = gxio_mpipe_link_open(&(ctl->links[i].link), 
						&ctl->ctx, ctl->links[i].linkname, 0)) < 0) {
			LOGERROR("Failed to open link: %s, rc %d", ctl->links[i].linkname, rc);
			return -1;
		}
		ctl->links[i].link_openned = 1;
		ctl->links[i].channel = gxio_mpipe_link_channel(&(ctl->links[i].link));
	}

	/* iqueue */
	if ((ctl->ring = gxio_mpipe_alloc_notif_rings(&ctl->ctx, ctl->numiqueues, 0, 0)) < 0) {
		LOGERROR("Failed to allcoate notif rings.");
		return -1;
	}
	
	for (i = 0; i < ctl->numiqueues; i++) {
		if ((cpu = adapter_mpipe_dpcore2cpu(ctl->start_dpcore + i)) < 0) {
			LOGERROR("Failed to get cpu for dpcore %d", ctl->start_dpcore + i);
			return -1;
		}

		ctl->iqueues[i].id = i;
		ctl->iqueues[i].cpu = cpu;

		tmc_alloc_init(&alloc);
		tmc_alloc_set_home(&alloc, cpu);

		ctl->iqueues[i].iqueue = tmc_alloc_map(&alloc, sizeof(gxio_mpipe_iqueue_t));
		if (ctl->iqueues[i].iqueue == NULL) {
			LOGERROR("# iqueue: %d, failed to allocate iqueue", i);
			return -1;
		}

		iring_size = iring_entries * sizeof(gxio_mpipe_idesc_t);
		if (!tmc_alloc_set_pagesize(&alloc, iring_size)) {
			LOGERROR("# iqueue: %d, failed to set alloc's pagesize: page size %d", i, iring_size);
			return -1;
		}
		ctl->iqueues[i].iqueue_mem = tmc_alloc_map(&alloc, iring_size);
		if (ctl->iqueues[i].iqueue_mem == NULL) {
			LOGERROR("# iqueue: %d, failed to allocate iqueue mem", i);
			return -1;
		}

		if ((rc = gxio_mpipe_iqueue_init(ctl->iqueues[i].iqueue, 
						&ctl->ctx, ctl->ring + i,
						ctl->iqueues[i].iqueue_mem, iring_size, 0)) < 0) {
			LOGERROR("# iqueue: %d, failed to init iqueue: rc %d", i, rc);
			return -1;
		}
	}

	if ((ctl->group = gxio_mpipe_alloc_notif_groups(&ctl->ctx, 1, 0, 0)) < 0) {
		LOGERROR("Failed to allocate notif groups.");
		return -1;
	}
	if ((ctl->bucket = gxio_mpipe_alloc_buckets(&ctl->ctx, num_buckets, 0, 0)) < 0) {
		LOGERROR("Failed to allocate buckets.");
		return -1;
	}
	mode = GXIO_MPIPE_BUCKET_ROUND_ROBIN;
	if ((rc = gxio_mpipe_init_notif_group_and_buckets(&ctl->ctx, ctl->group,
					ctl->ring, ctl->numiqueues,
					ctl->bucket, num_buckets, mode) < 0)) {
		LOGERROR("Failed to init notif group and buckets: rc %d", rc);
		return -1;
	}

	/* equeue */
	if (ctl->numequeues > 0) {
		if ((ctl->ering = gxio_mpipe_alloc_edma_rings(&ctl->ctx,
						ctl->numequeues, 0, 0)) < 0) {
			LOGERROR("Failed to allocate ering.");
			return -1;
		}

		for (i = 0; i < ctl->numequeues; i++) {
			tmc_alloc_init(&alloc);
			tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_HASH);

			ctl->equeues[i].equeue = tmc_alloc_map(&alloc, sizeof(gxio_mpipe_equeue_t));
			if (ctl->equeues[i].equeue == NULL) {
				LOGERROR("# equeue: %d, failed to allocate equeue", i);
				return -1;
			}

			ering_size = ering_entries * sizeof(gxio_mpipe_edesc_t);
			if (!tmc_alloc_set_pagesize(&alloc, ering_size)) {
				LOGERROR("# equeue: %d, failed to set alloc's pagesize: page size %d",
						i, ering_size);
				return -1;
			}
			ctl->equeues[i].equeue_mem = tmc_alloc_map(&alloc, ering_size);
			if (ctl->equeues[i].equeue_mem == NULL) {
				LOGERROR("# equeue: %d, failed to allocate equeue mem", i);
				return -1;
			}

			mlink = ctl->equeues[i].link;
			channel = gxio_mpipe_link_channel(&mlink->link);
			if ((rc = gxio_mpipe_equeue_init(ctl->equeues[i].equeue,
							&ctl->ctx, ctl->ering + i, channel,
							ctl->equeues[i].equeue_mem, ering_size, 0)) < 0) {
				LOGERROR("# equeue: %d, failed to init equeue: rc %d", i, rc);
				return -1;
			}
		}
	}

	/* stack + buffer */
	if ((ctl->stack = gxio_mpipe_alloc_buffer_stacks(&ctl->ctx, 1, 0, 0)) < 0) {
		LOGERROR("Failed to allocate bufer stacks.");
		return -1;
	}

	buffer_size_enum = GXIO_MPIPE_BUFFER_SIZE_1664;
	buffer_size = gxio_mpipe_buffer_size_enum_to_buffer_size(buffer_size_enum);
	stack_bytes = gxio_mpipe_calc_buffer_stack_bytes(num_buffers);
	stack_bytes += -(long)stack_bytes & (128 - 1);

	tsize = stack_bytes + num_buffers * (ADAPTER_MPIPE_BUFFER_HEADROOM_SIZE + buffer_size);
	page_size = tmc_alloc_get_huge_pagesize();
	ctl->numpages = (tsize + (page_size - 1)) / page_size;

	LOG("buffer stack: pagesize %d, numpages %d, tsize %d", page_size, ctl->numpages, tsize);

	tmc_alloc_init(&alloc);
#if 0
	tmc_alloc_set_home(&alloc, TMC_ALLOC_HOME_HASH);
#endif
	tmc_alloc_set_huge(&alloc);

	if ((ctl->pages = tmc_alloc_map(&alloc, page_size * ctl->numpages)) == NULL) {
		LOGERROR("Failed to allocate %d huge pages.", ctl->numpages);
		return -1;
	}
	mem = ctl->pages;

	if ((rc = gxio_mpipe_init_buffer_stack(&ctl->ctx, ctl->stack,
					buffer_size_enum, mem, stack_bytes, 0) < 0)) {
		LOGERROR("Failed to init buffer stack: rc %d", rc);
		return -1;
	}
	
	for (i = 0; i < ctl->numpages; i++) {
		if ((rc = gxio_mpipe_register_page(&ctl->ctx, ctl->stack,
						ctl->pages + i * page_size, page_size, 0)) < 0) {
			LOGERROR("# page: %d, failed to register page: rc %d", i, rc);
			return -1;
		}
	}
	
	mem += stack_bytes;
	for (i = 0; i < num_buffers; i++) {
		adapter_mpipe_init_pkthdr(mem);
		gxio_mpipe_push_buffer(&ctl->ctx, ctl->stack, mem + ADAPTER_MPIPE_BUFFER_HEADROOM_SIZE);
		mem += ADAPTER_MPIPE_BUFFER_HEADROOM_SIZE + buffer_size;
	}
	if (mem > (ctl->pages + page_size * ctl->numpages)) {
		LOGERROR("Huge page mem overflow.");
		return -1;
	}

	gxio_mpipe_rules_init(&rules, &ctl->ctx);
	gxio_mpipe_rules_begin(&rules, ctl->bucket, num_buckets, NULL);
	if ((rc = gxio_mpipe_rules_commit(&rules)) < 0) {
		LOGERROR("Failed to commit rules: rc %d", rc);
		return -1;
	}

	return 0;
}

static struct adapter_mpipe_ctl *adapter_mpipe_init(void *adap,
		unsigned long cfghd, char *sname, int sidx)
{
	struct adapter_mpipe_ctl *ctl;
	int count, i, num, dpcore, numdpcores;
	char value[2048], *p, *q;

	ctl = (struct adapter_mpipe_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(*ctl));
	ctl->rx = 1;

	count = CfgGetCount(cfghd, sname, "port", 1);
	if (count <= 0) {
		LOGERROR("%s: Section %s: port must be defined.",
				adapter_get_name(adap), sname);
		goto adapter_mpipe_init_exit;
	}
	if (count > ADAPTER_MPIPE_MAXLINK) {
		LOGERROR("%s: Section %s: too many ports defined.",
				adapter_get_name(adap), sname);
		goto adapter_mpipe_init_exit;
	}

	LOGDEBUG("%s: Section %s: #port: %d", adapter_get_name(adap), sname, count);

	for (i = 1; i <= count; i++) {
		if (CfgGetValue(cfghd, sname, "port", value, i, sidx) < 0) {
			LOGERROR("%s: Section %s: port[%d]: must defined.",
					adapter_get_name(adap), sname, i);
			goto adapter_mpipe_init_exit;
		}

		p = value;
		
		q = strchr(p, ',');
		if (q == p) {
			LOGERROR("%s: Section %s: port[%d]: No link defined.",
					adapter_get_name(adap), sname, i); 
			goto adapter_mpipe_init_exit;
		}
		if (q) {
			*q = 0;
		}

		strcpy(ctl->links[ctl->numlinks].linkname, p);

		if (q) {
			p = q + 1;
		}
		if (q && *p) {
			if (strcmp(p, "tx") == 0) {
				ctl->links[ctl->numlinks].flag |= ADAPTER_MPIPE_FLAG_TX;
				ctl->equeues[ctl->numequeues].link = &ctl->links[ctl->numlinks];
				++ctl->numequeues;
			}
			else {
				LOGERROR("%s: Section %s: port[%d]: invalid flag defined: %s",
						adapter_get_name(adap), sname, i, p);
				goto adapter_mpipe_init_exit;
			}
		}

		LOGDEBUG("%s: Section %s: port #%d: %s, %08x", 
				adapter_get_name(adap), sname,
				ctl->numlinks,
				ctl->links[ctl->numlinks].linkname, ctl->links[ctl->numlinks].flag);

		++ctl->numlinks;
	}

	if (CfgGetValue(cfghd, sname, "iqueue.number", value, 1, sidx) < 0) {
		LOGERROR("%s: Section %s: iqueue.number must be defined.",
				adapter_get_name(adap), sname);
		goto adapter_mpipe_init_exit;
	}

	p = value;

	q = strchr(p, '@');
	if (q == p) {
		LOGERROR("%s: Section %s: iqueue.number: No number defined.",
				adapter_get_name(adap), sname);
		goto adapter_mpipe_init_exit;
	}
	if (q)
		*q = 0;

	num = atoi(p);
	if (num <= 0) {
		LOGERROR("%s: Section %s: invalid iqueue.number defined: %s",
				adapter_get_name(adap), sname, p);
		goto adapter_mpipe_init_exit;
	}
	ctl->numiqueues = num;

	if (q && *(q + 1)) {
		dpcore = atoi(q + 1);
		if (dpcore < 0) {
			LOGERROR("%s: Section %s: invalid dpcore defined: %s",
					adapter_get_name(adap), sname, q + 1);
			goto adapter_mpipe_init_exit;
		}
		ctl->start_dpcore = dpcore;
	}
	LOGDEBUG("%s: Section %s: iqueue.number %d, start core %d",
			adapter_get_name(adap), sname, ctl->numiqueues, ctl->start_dpcore);

	if ((numdpcores = adapter_mpipe_get_dpcore_num()) < 0) {
		goto adapter_mpipe_init_exit;
	}
	if (ctl->start_dpcore + ctl->numiqueues > numdpcores) {
		LOGERROR("No enough dpcores: start dpcore: %d, dp cores: %d, iqueues: %d",
				ctl->start_dpcore, numdpcores, ctl->numiqueues);
		goto adapter_mpipe_init_exit;
	}
	LOGDEBUG("dp cores: %d", numdpcores);

	if (adapter_mpipe_init_mpipe(ctl) < 0) {
		goto adapter_mpipe_init_exit;
	}
	LOGDEBUG("mpipe initialized.");

	tmc_sync_barrier_init(&ctl->barrier, ctl->numiqueues + 1);

	for (i = 0; i < ctl->numiqueues; i++) {
		ctl->iqueues[i].fifo = pfifo_open(0x10000, "adapter.from.mpipe.fifo", 1600, NULL, NULL);
		if (ctl->iqueues[i].fifo == NULL) {
			LOGERROR("# iqueue: %d, failed to init mpipe fifo.", i);
			goto adapter_mpipe_init_exit;
		}

		ctl->iqueues[i].thread = thread_open(adapter_mpipe_thread,
				(void *)(ctl->iqueues + i));
		if (ctl->iqueues[i].thread == NULL) {
			LOGERROR("# iqueue: %d, failed to init thread.", i);
			goto adapter_mpipe_init_exit;
		}
	}

	return ctl;

adapter_mpipe_init_exit:
	adapter_mpipe_exit(ctl);
	return NULL;
}

static void adapter_mpipe_close(void *adap)
{
	struct adapter_mpipe_ctl *ctl;

	ctl = (struct adapter_mpipe_ctl *)adapter_get_data(adap);
	if (ctl) {
		if (ctl->openned) {
			if (gxio_mpipe_get_stats(&ctl->ctx, &ctl->current_stat) != 0) {
				LOGERROR("failed to get current stat.");
			}
			else {
#define DIFF_VALUE(field)	(ctl->current_stat.field - ctl->prev_stat.field)
				LOG("mpipe: ingress %llu P ( %llu B), "
						"drop %llu (nb %llu, ip %llu, cl %llu), egress %llu P ( %llu B)",
						DIFF_VALUE(ingress_packets),
						DIFF_VALUE(ingress_bytes),
						DIFF_VALUE(ingress_drops),
						DIFF_VALUE(ingress_drops_no_buf),
						DIFF_VALUE(ingress_drops_ipkt),
						DIFF_VALUE(ingress_drops_cls_lb),
						DIFF_VALUE(egress_packets),
						DIFF_VALUE(egress_bytes));
			}
		}

		adapter_mpipe_exit(ctl);
	}
}

static void *adapter_mpipe_read(void *adap)
{
	static int read_commit = 0;
	static int tid = 0;
	struct adapter_mpipe_ctl *ctl;
	void *ph = NULL;
	int i;

	ctl = (struct adapter_mpipe_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return NULL;

	if (read_commit) {
		pfifo_read_commit(ctl->iqueues[tid].fifo);
		read_commit = 0;

		if (++tid == ctl->numiqueues) {
			tid = 0;
		}
	}

	for (i = 0; i < ctl->numiqueues; i++) {
		ph = pfifo_read(ctl->iqueues[tid].fifo);
		if (ph) {
			read_commit = 1;
			return ph;
		}

		if (++tid == ctl->numiqueues) {
			tid = 0;
		}
	}

	return ph;
}

static int adapter_mpipe_write(void *adap, void *data, int len)
{
	static int qid = 0;
	struct adapter_mpipe_ctl *ctl;
	gxio_mpipe_edesc_t edesc = {{ 0 }};
	int64_t slot;
	unsigned char *buf = NULL;
	int buflen = 0;
	int i, lqid = 0;

	ctl = (struct adapter_mpipe_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return 0;

	buf = gxio_mpipe_pop_buffer(&ctl->ctx, ctl->stack);
	if (buf == NULL) {
		LOGERROR("failed to pop buffer.");
		return 0;
	}

	buflen = len - (sizeof(pkt_hdr) + 2);
	memcpy(buf, data + sizeof(pkt_hdr) + 2, buflen);
	__insn_mf();

	edesc.bound = 1;
	edesc.xfer_size = buflen;
	edesc.va = (uintptr_t)buf;
	edesc.stack_idx = ctl->stack;
	edesc.inst = ctl->instance;
	edesc.hwb = 1;
	edesc.size = GXIO_MPIPE_BUFFER_SIZE_1664;

	if (ctl->numequeues > 1) {
		lqid  = arch_atomic_add(&qid, 1);
		lqid %= ctl->numequeues;
	}

	for (i = 0; i < ctl->numequeues; i++) {
		if ((slot = gxio_mpipe_equeue_reserve(ctl->equeues[lqid].equeue, 1)) >= 0) {
			gxio_mpipe_equeue_put_at(ctl->equeues[lqid].equeue, edesc, slot);
			if (ctl->numequeues > 1) {
				arch_atomic_add(&(ctl->equeues[lqid].tpkts), 1);
				arch_atomic_add(&(ctl->equeues[lqid].tbytes), buflen);
			}
			
			return len;
		}

		if (++lqid == ctl->numequeues) {
			lqid = 0;
		}
	}

	return 0;
}

static int adapter_mpipe_open(void *adap)
{
	struct adapter_mpipe_ctl *ctl;

	ctl = (struct adapter_mpipe_ctl *)adapter_get_data(adap);
	if (ctl) {
		if (ctl->openned == 0) {
			if (ctl->pkt_cb == NULL) {
				ctl->priv = ctl;
				ctl->pkt_cb = pkt_cb_default;
			}
			tmc_sync_barrier_wait(&ctl->barrier);
			if (gxio_mpipe_get_stats(&ctl->ctx, &ctl->prev_stat) != 0) {
				LOGERROR("failed to get prev stat.");
			}
			sim_enable_mpipe_links(ctl->instance, -1);
			ctl->openned = 1;
		}
	}

	return 0;
}

static void adapter_mpipe_set_pkt_cb(void *adap, void *priv, adap_pkt_cb pcb)
{
	struct adapter_mpipe_ctl *ctl;

	ctl = (struct adapter_mpipe_ctl *)adapter_get_data(adap);
	if (ctl) {
		ctl->priv = priv;
		ctl->pkt_cb = pcb;
	}
}

static int adapter_mpipe_ioctl(void *adap, int code, void *arg)
{
	struct adapter_mpipe_ctl *ctl;
	char *str;
	int i, j;
	int rc = -1;

	if (!adap || !arg)
		return rc;

	ctl = (struct adapter_mpipe_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return rc;

	switch (code) {
		case ADAPTER_MPIPE_IOCTL_GETIQUEUESTAT:
			str = (char *)arg;
			str += sprintf(str, "iqueue.stat:\n");
			for (i = 0; i < ctl->numiqueues; i++) {
				str += sprintf(str, "iqueue #%d: %llu P, %llu B, %llu E, %llu F\n",
						ctl->iqueues[i].id,
						ctl->iqueues[i].rpkts, ctl->iqueues[i].rbytes,
						ctl->iqueues[i].rerr, ctl->iqueues[i].rfull);
			}
			rc = 0;
			break;

		case ADAPTER_MPIPE_IOCTL_GETILINKSTAT:
			str = (char *)arg;
			str += sprintf(str, "ingress.link.stat:\n");
			for (i = 0; i < ctl->numlinks; i++) {
				unsigned long long rpkts = 0ull, rbytes = 0ull;

				for (j = 0; j < ctl->numiqueues; j++) {
					rpkts += ctl->iqueues[j].rpkts_link[ctl->links[i].channel];
					rbytes += ctl->iqueues[j].rbytes_link[ctl->links[i].channel];
				}

				str += sprintf(str, "link #%d: %s, %llu P, %llu B\n",
						i, ctl->links[i].linkname, rpkts, rbytes);
			}
			rc = 0;
			break;

		case ADAPTER_MPIPE_IOCTL_GETELINKSTAT:
			str = (char *)arg;
			str += sprintf(str, "egress.link.stat:\n");
			for (i = 0; i < ctl->numequeues; i++) {
				str += sprintf(str, "link #%d: %s, %llu P, %llu B\n",
						i, (ctl->equeues[i].link)->linkname,
						ctl->equeues[i].tpkts, ctl->equeues[i].tbytes);
			}
			rc = 0;
			break;

		case ADAPTER_MPIPE_IOCTL_STOPRX:
			ctl->rx = 0;
			rc = 0;
			break;

		case ADAPTER_MPIPE_IOCTL_GETMPIPESTAT:
			str = (char *)arg;
			if (ctl->openned) {
				if (gxio_mpipe_get_stats(&ctl->ctx, &ctl->current_stat) == 0) {
#define DIFF_VALUE2(field)      (unsigned long long)(ctl->current_stat.field - ctl->prev_stat.field)
					sprintf(str, "mpipe: ingress %llu P ( %llu B), "
						"drop %llu (nb %llu, ip %llu, cl %llu), egress %llu P ( %llu B)\n",
						DIFF_VALUE2(ingress_packets),
						DIFF_VALUE2(ingress_bytes),
						DIFF_VALUE2(ingress_drops),
						DIFF_VALUE2(ingress_drops_no_buf),
						DIFF_VALUE2(ingress_drops_ipkt),
						DIFF_VALUE2(ingress_drops_cls_lb),
						DIFF_VALUE2(egress_packets),
						DIFF_VALUE2(egress_bytes));

					rc = 0;
				}
			}
			break;

		default:
			LOGWARN("%s: invalid ioctl code %d", adapter_get_name(adap), code);
			break;
	}

	return rc;
}

static void adapter_mpipe_freebuf(void *adap, void *buf)
{

}

static void *_adapter_register_mpipe(unsigned long cfghd, char *section, int sidx)
{
	void *adap;
	struct adapter_mpipe_ctl *ctl;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_MPIPE_NAME);

	ctl = adapter_mpipe_init(adap, cfghd, 
			section ? section : ADAPTER_MPIPE_NAME, sidx);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_mpipe_open);
	adapter_set_read(adap, adapter_mpipe_read);
	adapter_set_write(adap, adapter_mpipe_write);
	adapter_set_ioctl(adap, adapter_mpipe_ioctl);
	adapter_set_close(adap, adapter_mpipe_close);
	adapter_set_freebuf(adap, adapter_mpipe_freebuf);
	adapter_set_set_pkt_cb(adap, adapter_mpipe_set_pkt_cb);

	return adap;
}

void *adapter_register_mpipe(unsigned long cfghd, char *section, int sidx)
{
	if (cfghd != 0ul) {
		return _adapter_register_mpipe(cfghd, section, sidx);
	}

	return NULL;
}

void *adapter_register_mpipe_cfgfile(char *cfgfile, char *section, int sidx)
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd != 0ul) {
		p = _adapter_register_mpipe(cfghd, section, sidx);
		CfgInvalidate(cfghd);
	}

	return p;
}

void *adapter_register_mpipe_cfgstr(char *cfgstr, char *section, int sidx)
{
	unsigned long cfghd;
	void *p = NULL;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd != 0ul) {
		p = _adapter_register_mpipe(cfghd, section, sidx);
		CfgInvalidate(cfghd);
	}

	return p;
}

#else

void *adapter_register_mpipe(unsigned long cfghd, char *section, int sidx)
{
	LOGERROR("Failed to support mpipe adapter on this architecure.");
	return NULL;
}

void *adapter_register_mpipe_cfgfile(char *cfgfile, char *section, int sidx)
{
	LOGERROR("Failed to support mpipe adapter on this architecure.");
	return NULL;
}

void *adapter_register_mpipe_cfgstr(char *cfgstr, char *section, int sidx)
{
	LOGERROR("Failed to support mpipe adapter on this architecure.");
	return NULL;
}

#endif

