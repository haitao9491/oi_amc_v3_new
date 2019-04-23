/*
 *
 * utimer.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "misc.h"
#include "utimer.h"
#include "rbtree.h"

struct utimer_hd {
	int    type;
	void (*callback)(unsigned int, unsigned int, void *);

	struct rb_root  rbtree;
	struct rb_node *first;

	unsigned int last_s, last_ns;

	unsigned long long num_add;
	unsigned long long num_add_compare;

	unsigned int count;
};

struct utimer_entry {
	struct rb_node node;

	unsigned int expire_s, expire_ns;
	void *data;

	struct utimer_hd *hd;
};

void *utimer_open(int type, void (*callback)(unsigned int, unsigned int,void *))
{
	struct utimer_hd *ut;

	ut = (struct utimer_hd *)malloc(sizeof(struct utimer_hd));
	if (ut == NULL)
		return NULL;
	memset(ut, 0, sizeof(struct utimer_hd));

	ut->type = type;
	ut->callback = callback;

	ut->rbtree.rb_node = NULL;
	ut->first  = NULL;

	return ut;
}

void *utimer_add(void *hd,
		unsigned int expire_s, unsigned int expire_ns, void *data)
{
	struct utimer_hd *ut;
	struct rb_node **where, *parent = NULL;
	struct utimer_entry *entry;
	int leftmost = 1;

	if ((ut = (struct utimer_hd *)hd) == NULL)
		return NULL;

	entry = (struct utimer_entry *)malloc(sizeof(*entry));
	if (entry == NULL)
		return NULL;

	entry->expire_s = expire_s;
	entry->expire_ns = expire_ns;
	entry->data = data;
	entry->hd = ut;

	ut->count++;
	ut->num_add++;

	/* Figure out where to put new node */
	where = &(ut->rbtree.rb_node);
	while (*where) {
		long long diff;
		struct utimer_entry *current;

		current = rb_entry(*where, struct utimer_entry, node);
		parent = *where;

		diff = time_diff(current->expire_s, current->expire_ns,
				expire_s, expire_ns);
		ut->num_add_compare++;

		/* We don't care about collisions. Entries with the same
		 * expiry time stay together.
		 */
		if (diff < 0) {
			where = &((*where)->rb_left);
		}
		else {
			where = &((*where)->rb_right);
			leftmost = 0;
		}
	}

	if (leftmost) {
		ut->first = &(entry->node);
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&(entry->node), parent, where);
	rb_insert_color(&(entry->node), &(ut->rbtree));

	return entry;
}

void *utimer_add_by_offset(void *hd,
		unsigned int expire_s, unsigned int expire_ns, void *data)
{
	struct utimer_hd *ut;
	unsigned int s, ns;

	if ((ut = (struct utimer_hd *)hd) == NULL)
		return NULL;

	s  = ut->last_s + expire_s;
	ns = ut->last_ns + expire_ns;

	while (ns >= 1000000000) {
		s++;
		ns -= 1000000000;
	}

	return utimer_add(hd, s, ns, data);
}

void utimer_delete(void *timer)
{
	struct utimer_entry *entry;
	struct utimer_hd *ut;
	struct rb_node *node;

	entry = (struct utimer_entry *)timer;
	if (entry == NULL)
		return;

	ut = entry->hd;
	node = &(entry->node);

	if (ut->first == node)
		ut->first = rb_next(node);
	rb_erase(node, &(ut->rbtree));

	ut->count--;

	free(entry);
}

void utimer_get_counter(void *hd, unsigned int *count,
		unsigned long long *add, unsigned long long *compare)
{
	struct utimer_hd *ut = (struct utimer_hd *)hd;

	if (ut) {
		*count = ut->count;
		*add = ut->num_add;
		*compare = ut->num_add_compare;
	}
}

void utimer_run(void *hd, unsigned int s, unsigned int ns)
{
	struct utimer_hd *ut;
	struct rb_node *node;
	struct utimer_entry *entry;

	if ((ut = (struct utimer_hd *)hd) == NULL)
		return;

	if ((ut->type & UTIMER_INTERNAL_TIMESOURCE) == UTIMER_INTERNAL_TIMESOURCE) {
		struct timeval tv;

		gettimeofday(&tv, NULL);
		s  = tv.tv_sec;
		ns = tv.tv_usec * 1000;
	}

	if ((ns == ut->last_ns) && (s == ut->last_s))
		return;

	ut->last_s = s;
	ut->last_ns = ns;

	while ((node = ut->first) != NULL) {
		entry = rb_entry(node, struct utimer_entry, node);
		if (time_diff(s, ns, entry->expire_s, entry->expire_ns) > 0)
			break;

		ut->first = rb_next(node);
		rb_erase(node, &(ut->rbtree));

		ut->callback(s, ns, entry->data);
		free(entry);
		ut->count--;
	}
}

void utimer_close(void *hd)
{
	struct utimer_hd *ut;
	struct rb_node *node;
	struct utimer_entry *entry;
	int cnt = 0;

	if ((ut = (struct utimer_hd *)hd) == NULL)
		return;

	while ((node = ut->first) != NULL) {
		entry = rb_entry(node, struct utimer_entry, node);

		ut->first = rb_next(node);
		rb_erase(node, &(ut->rbtree));

		free(entry);
		ut->count--;
		cnt++;
	}

	LOG("Closing timer, %d scheduled tasks removed! [%u, %llu / %llu]",
			cnt, ut->count, ut->num_add_compare, ut->num_add);
	free(ut);
}

