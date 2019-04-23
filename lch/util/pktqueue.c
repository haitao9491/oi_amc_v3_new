/*
 *
 * pktqueue.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "mutex.h"
#include "list.h"
#include "pkt.h"
#include "pktqueue.h"
#include "misc.h"
#include "rbtree.h"

struct pktq_handle {
	struct list_head  list;
	void             *lock;
	int               type;

	unsigned int cnt;
	unsigned int size;
	unsigned int sizelimit;

	unsigned int sortlen;
	long long    duration;

	char *name;

	struct rb_root  rbtree;
	struct rb_node *first;
	struct rb_node *last;
};

struct pktq_list_entry {
	struct list_head  list;
	struct rb_node    node;

	void             *ph;
	int               len;

	unsigned int s, ns;
};

void *pkt_queue_open(unsigned int type, char *name)
{
	struct pktq_handle *h;

	h = malloc(sizeof(struct pktq_handle));
	if (h == NULL)
		return NULL;

	INIT_LIST_HEAD(&(h->list));
	h->lock = mutex_open(NULL);
	if (h->lock == NULL) {
		free(h);
		return NULL;
	}
	h->type = type;
	h->sizelimit = 0x40000000;
	h->size = 0;
	h->sortlen = 0;
	h->duration = 0;
	h->cnt = 0;
	h->name = name ? strdup(name) : NULL;

	h->rbtree.rb_node = NULL;
	h->first = NULL;
	h->last = NULL;

	return h;
}

void pkt_queue_set_sizelimit(void *hd, unsigned int sizelimit)
{
	struct pktq_handle *h = (struct pktq_handle *)hd;

	if (h)
		h->sizelimit = sizelimit;
}

void pkt_queue_set_sortlen(void *hd, int sortlen)
{
	struct pktq_handle *h = (struct pktq_handle *)hd;

	if (h && (h->type & PKTQUEUE_BUFFER_SORTED))
		h->sortlen = (unsigned int)sortlen;
}

void  pkt_queue_set_duration(void *hd, int duration)
{
	struct pktq_handle *h = (struct pktq_handle *)hd;

	if (h && (h->type & PKTQUEUE_BUFFER_SORTED))
		h->duration = (long long)duration * 1000000;
}

static void _pkt_push_sorted(struct pktq_handle *h,
		struct pktq_list_entry *entry)
{
	struct rb_node **where, *parent = NULL;
	int leftmost = 1;
	int rightmost = 1;

	entry->s  = pkthdr_get_ts_s((pkt_hdr *)(entry->ph));
	entry->ns = pkthdr_get_ts_ns((pkt_hdr *)(entry->ph));

	where = &(h->rbtree.rb_node);
	while (*where) {
		struct pktq_list_entry *current;

		current = rb_entry(*where, struct pktq_list_entry, node);
		parent = *where;

		/* We don't care about collisions. Entries with the same
		 * timestamp stay together.
		 */
		if ((entry->s < current->s) ||
				((entry->s == current->s) && (entry->ns < current->ns))) {
			where = &((*where)->rb_left);
			rightmost = 0;
		}
		else {
			where = &((*where)->rb_right);
			leftmost = 0;
		}
	}

	if (leftmost) {
		h->first = &(entry->node);
	}
	if (rightmost) {
		h->last = &(entry->node);
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&(entry->node), parent, where);
	rb_insert_color(&(entry->node), &(h->rbtree));
}

static void _pkt_push(struct pktq_handle *h,
		struct pktq_list_entry *entry, unsigned int sz)
{
	mutex_lock(h->lock);
	if (h->type & PKTQUEUE_BUFFER_SORTED) {
		_pkt_push_sorted(h, entry);
	}
	else {
		list_add(&(entry->list), h->list.prev);
	}
	h->cnt++;
	h->size += sz;
	mutex_unlock(h->lock);
}

int pkt_push(void *hd, void *ph, int len)
{
	struct pktq_handle     *h = (struct pktq_handle *)hd;
	struct pktq_list_entry *entry;
	unsigned int sz;

	sz = sizeof(struct pktq_list_entry) + len;
	if ((h->size + sz) > h->sizelimit)
		return 0;

	entry = malloc(sizeof(struct pktq_list_entry));
	if (entry == NULL)
		return -1;

	if (h->type & PKTQUEUE_MALLOC_BUFFER) {
		if ((entry->ph = malloc(len)) == NULL) {
			free(entry);
			return -1;
		}
		memcpy(entry->ph, ph, len);
	}
	else {
		entry->ph = ph;
	}
	entry->len = len;

	_pkt_push(h, entry, sz);

	return len;
}

int pkt_push_nohdr(void *hd, void *ph, int len, unsigned int s, unsigned int ns)
{
	struct pktq_handle     *h = (struct pktq_handle *)hd;
	struct pktq_list_entry *entry;
	pkt_hdr *hdr;
	unsigned int sz;

	if ((h->type & PKTQUEUE_MALLOC_BUFFER) == 0)
		return -1;

	if (len < 0)
		return -1;

	sz = sizeof(struct pktq_list_entry) + sizeof(pkt_hdr) + len;
	if ((h->size + sz) > h->sizelimit)
		return 0;

	entry = malloc(sizeof(struct pktq_list_entry));
	if (entry == NULL)
		return -1;

	entry->len = sizeof(pkt_hdr) + len;
	if ((entry->ph = malloc(entry->len)) == NULL) {
		free(entry);
		return -1;
	}

	hdr = (pkt_hdr *)(entry->ph);
	memset(hdr, 0, sizeof(pkt_hdr));

	pkthdr_set_sync(hdr);
	pkthdr_set_dlen(hdr, len);
	pkthdr_set_ts(hdr, s, ns);

	if (len > 0)
		memcpy(hdr + 1, ph, len);

	_pkt_push(h, entry, sz);

	return len;
}

static void *do_pkt_pop_sorted(struct pktq_handle *h, int force)
{
	struct pktq_list_entry *entry;
	struct rb_node         *node;
	void                   *ph;

	if (h->cnt <= 0)
		return NULL;

	if (!force && (h->sortlen > 0)) {
		if (h->cnt <= h->sortlen)
			return NULL;
	}

	if (!force && (h->duration > 0)) {
		struct pktq_list_entry *first, *last;
		unsigned int      s, ns;
		unsigned int      ls, lns;

		if (h->cnt == 1)
			return NULL;

		first = rb_entry(h->first, struct pktq_list_entry, node);
		last = rb_entry(h->last, struct pktq_list_entry, node);

		pkthdr_get_ts((pkt_hdr *)(first->ph), &s, &ns);
		pkthdr_get_ts((pkt_hdr *)(last->ph), &ls, &lns);
		if (time_diff(s, ns, ls, lns) <= h->duration)
			return NULL;
	}

	node = h->first;
	h->first = rb_next(node);
	rb_erase(node, &(h->rbtree));
	if (node == h->last)
		h->last = NULL;
	h->cnt--;

	entry = rb_entry(node, struct pktq_list_entry, node);
	h->size -= (sizeof(struct pktq_list_entry) + entry->len);

	ph = entry->ph;
	free(entry);

	return ph;
}

static void *do_pkt_pop_list(struct pktq_handle *h)
{
	struct list_head       *pos;
	struct pktq_list_entry *entry;
	void *ph;

	if (h->cnt <= 0)
		return NULL;

	pos = h->list.next;
	list_del(pos);
	h->cnt--;

	entry = list_entry(pos, struct pktq_list_entry, list);
	h->size -= (sizeof(struct pktq_list_entry) + entry->len);

	ph = entry->ph;
	free(entry);

	return ph;
}

static void *do_pkt_pop(struct pktq_handle *h, int force)
{
	void *rc;

	mutex_lock(h->lock);
	if (h->type & PKTQUEUE_BUFFER_SORTED) {
		rc = do_pkt_pop_sorted(h, force);
	}
	else {
		rc = do_pkt_pop_list(h);
	}
	mutex_unlock(h->lock);

	return rc;
}

void *pkt_pop(void *hd)
{
	return do_pkt_pop((struct pktq_handle *)hd, 0);
}

void *pkt_pop_force(void *hd)
{
	return do_pkt_pop((struct pktq_handle *)hd, 1);
}

void pkt_queue_get_mem(void *hd, unsigned int *cnt, unsigned int *size)
{
	struct pktq_handle *h = (struct pktq_handle *)hd;

	if (h) {
		if (cnt)
			*cnt = h->cnt;
		if (size)
			*size = h->size;
	}
}

static void pkt_queue_free_entry(struct pktq_handle *h,
		struct pktq_list_entry *entry)
{
	if (entry->ph) {
		if (h->type & PKTQUEUE_MALLOC_BUFFER) {
			free(entry->ph);
		}
	}
	free(entry);
}

void pkt_queue_clear(void *hd)
{
	struct pktq_handle     *h = (struct pktq_handle *)hd;
	struct pktq_list_entry *entry;

	if (!h)
		return;

	mutex_lock(h->lock);
	if (h->type & PKTQUEUE_BUFFER_SORTED) {
		struct rb_node *node;

		while ((node = h->first) != NULL) {
			entry = rb_entry(node, struct pktq_list_entry, node);

			h->first = rb_next(node);
			rb_erase(node, &(h->rbtree));

			pkt_queue_free_entry(h, entry);
		}
	}
	else {
		struct list_head *pos, *n;

		list_for_each_safe(pos, n, &(h->list)) {
			list_del(pos);
			entry = list_entry(pos, struct pktq_list_entry, list);

			pkt_queue_free_entry(h, entry);
		}
	}
	LOG("pkt_queue(%s): Cleared %u pkts (%uB: payload %u, overhead %u).",
			h->name ? h->name : "null", h->cnt, h->size,
			h->size - h->cnt * sizeof(struct pktq_list_entry),
			h->cnt * sizeof(struct pktq_list_entry));

	h->size = 0;
	h->cnt = 0;
	h->rbtree.rb_node = NULL;
	h->first = NULL;
	h->last = NULL;

	mutex_unlock(h->lock);
}

void pkt_queue_close(void *hd)
{
	struct pktq_handle     *h = (struct pktq_handle *)hd;
	struct pktq_list_entry *entry;

	if (!h)
		return;

	mutex_lock(h->lock);
	if (h->type & PKTQUEUE_BUFFER_SORTED) {
		struct rb_node *node;

		while ((node = h->first) != NULL) {
			entry = rb_entry(node, struct pktq_list_entry, node);

			h->first = rb_next(node);
			rb_erase(node, &(h->rbtree));

			pkt_queue_free_entry(h, entry);
		}
	}
	else {
		struct list_head *pos, *n;

		list_for_each_safe(pos, n, &(h->list)) {
			list_del(pos);
			entry = list_entry(pos, struct pktq_list_entry, list);

			pkt_queue_free_entry(h, entry);
		}
	}
	LOG("pkt_queue(%s): Released %u pkts (%uB: payload %u, overhead %u).",
			h->name ? h->name : "null", h->cnt, h->size,
			h->size - h->cnt * sizeof(struct pktq_list_entry),
			h->cnt * sizeof(struct pktq_list_entry));
	mutex_unlock(h->lock);

	mutex_close(h->lock);
	if (h->name)
		free(h->name);
	free(h);
}
