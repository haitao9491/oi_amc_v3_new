/*
 * (C) Copyright 2010
 * Beijing HLYT Technology Co., Ltd.
 *
 * timermgr.c - A brief description to describe this file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <sys/time.h>
#endif
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "misc.h"
#include "rbtree.h"
#include "mutex.h"
#include "pktqueue.h"
#include "hashtbl.h"
#include "timermgr.h"

#ifdef _USRDLL
//void *__timer_mgr_handle = NULL;
#endif

struct timer_mgr_list {
	struct list_head list;

	int             timesource;
	timer_callback  callback;
	void           *arg;
	const char     *name;

	unsigned int last_s, last_ns;

	unsigned int count;

	void *lock;
	struct rb_root  rbtree;
	struct rb_node *first;
	unsigned long long num_add;
	unsigned long long num_add_compare;
	unsigned long long num_add_rotate;
	unsigned long long num_del;
	unsigned long long num_del_rotate;
};

struct timer_mgr_hd {
	struct list_head  timerlists[TIMERMGR_SOURCE_MAXIMUM];
	void             *locks[TIMERMGR_SOURCE_MAXIMUM];

	/* Redundant pointer for easy access to predefined timerlists. */
	struct timer_mgr_list *pktq;
	struct timer_mgr_list *hashtbl;
};

struct timer_mgr_timer {
	struct rb_node node;

	unsigned int interval;
	unsigned int expire_s, expire_ns;
	void *data;

	struct timer_mgr_list *timerlist;

	struct list_head list;
};

void timer_mgr_timerlist_log(void *timerlist)
{
	struct timer_mgr_list *tlist = (struct timer_mgr_list *)timerlist;

	if (timerlist == NULL)
		return;

	LOG("TimerMGR(%s): %u running timers, ADD(%llu %llu %llu), DEL(%llu %llu)",
			tlist->name, tlist->count,
			tlist->num_add, tlist->num_add_compare, tlist->num_add_rotate,
			tlist->num_del, tlist->num_del_rotate);
}

static void timer_mgr_timerlist_cb_timerstatistics(
		unsigned int s, unsigned int ns, void *data, void *arg)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)data;

	if (mgr) {
		int i;

		for (i = 0; i < TIMERMGR_SOURCE_MAXIMUM; i++) {
			struct timer_mgr_list *timerlist;

			if (i != TIMERMGR_SOURCE_LOCAL)
				mutex_lock(mgr->locks[i]);
			//list_for_each_entry(timerlist, &mgr->timerlists[i], list) {
			for (timerlist = list_entry((&mgr->timerlists[i])->next,
				struct timer_mgr_list, list);
				&timerlist->list != (&mgr->timerlists[i]);
				timerlist = list_entry(timerlist->list.next,
				struct timer_mgr_list, list))
			{
				timer_mgr_timerlist_log(timerlist);
			}
			if (i != TIMERMGR_SOURCE_LOCAL)
				mutex_unlock(mgr->locks[i]);
		}
	}
}

static void timer_mgr_timerlist_cb_pktqstatistics(
		unsigned int s, unsigned int ns, void *data, void *arg)
{
	char buf[1024];

	buf[0] = buf[1023] = 0;
//	LOG("TimerMGR(PktQStatistics): %s", pkt_queue_log(data, buf, 1023));
}

static void timer_mgr_timerlist_cb_hashtblstatistics(
		unsigned int s, unsigned int ns, void *data, void *arg)
{
	char buf[1024];

	buf[0] = buf[1023] = 0;
//	LOG("TimerMGR(HashTblStatistics): %s", hashtbl_log(data, buf, 1023));
}

void *timer_mgr_open(int interval)
{
	struct timer_mgr_hd *mgr = NULL;
	void *timerlist;
	int i, failed = 0;

	if ((mgr = (struct timer_mgr_hd *)malloc(sizeof(*mgr))) == NULL) {
		LOGERROR("TimerMGR: Insufficient memory.");
		return NULL;
	}

	for (i = 0; i < TIMERMGR_SOURCE_MAXIMUM; i++) {
		INIT_LIST_HEAD(&mgr->timerlists[i]);
		if ((mgr->locks[i] = mutex_open(NULL)) == NULL)
			failed = 1;
	}
	mgr->pktq = NULL;
	mgr->hashtbl = NULL;

	if (failed) {
		timer_mgr_close(mgr);
		return NULL;
	}
	LOGDEBUG("TimerMGR: %d timer groups created.", i);

	/* Predefined timerlist for the statistics of all the timers. */
	timerlist = timer_mgr_create_timerlist(mgr, TIMERMGR_SOURCE_LOCAL,
			timer_mgr_timerlist_cb_timerstatistics, NULL, "TimerStatistics");
	if (timerlist) {
		timer_mgr_add_timer_periodic(timerlist, interval, mgr);
	}

	/* Predefined timerlists. */
	mgr->pktq = timer_mgr_create_timerlist(mgr, TIMERMGR_SOURCE_LOCAL,
			timer_mgr_timerlist_cb_pktqstatistics, NULL, "PktQStatistics");
	mgr->hashtbl = timer_mgr_create_timerlist(mgr, TIMERMGR_SOURCE_LOCAL,
			timer_mgr_timerlist_cb_hashtblstatistics, NULL, "HashTblStatistics");

	return mgr;
}

void timer_mgr_run(void *hd)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;
	struct timer_mgr_list *timerlist;

	mutex_lock(mgr->locks[TIMERMGR_SOURCE_LOCAL]);
	//list_for_each_entry(timerlist,&mgr->timerlists[TIMERMGR_SOURCE_LOCAL],list){
	timerlist = list_entry((&mgr->timerlists[TIMERMGR_SOURCE_LOCAL])->next,
		struct timer_mgr_list, list);
	for (; &timerlist->list != (&mgr->timerlists[TIMERMGR_SOURCE_LOCAL]);
		timerlist = list_entry(timerlist->list.next,
		struct timer_mgr_list, list))
	{
		timer_mgr_run_timerlist(NULL, timerlist, 0, 0);
	}
	mutex_unlock(mgr->locks[TIMERMGR_SOURCE_LOCAL]);
}

void timer_mgr_close(void *hd)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;
	struct list_head *pos, *n;
	int i;

	if (!mgr)
		return;

	for (i = 0; i < TIMERMGR_SOURCE_MAXIMUM; i++) {
		list_for_each_safe(pos, n, &mgr->timerlists[i]) {
			struct timer_mgr_list *timerlist;

			timerlist = list_entry(pos, struct timer_mgr_list, list);
			timer_mgr_release_timerlist(hd, timerlist);
		}
		mutex_close(mgr->locks[i]);
	}
	free(mgr);
	LOGDEBUG("TimerMGR: closed.");
}

static struct timer_mgr_list *timer_mgr_allocate_timerlist(
		int timesource, timer_callback callback, void *arg, const char *name)
{
	struct timer_mgr_list *timerlist;

	timerlist = (struct timer_mgr_list *)malloc(sizeof(*timerlist));
	if (timerlist) {
		INIT_LIST_HEAD(&timerlist->list);

		timerlist->timesource = timesource;
		timerlist->callback = callback;
		timerlist->arg = arg;
		timerlist->name = name;

		timerlist->last_s = 0;
		timerlist->last_ns = 0;
		if (timesource == TIMERMGR_SOURCE_LOCAL) {
			get_sys_time_s(&timerlist->last_s, &timerlist->last_ns);
			timerlist->last_ns *= 1000;
		}

		timerlist->count = 0;

		timerlist->lock = mutex_open(NULL);
		if (timerlist->lock == NULL) {
			free(timerlist);
			return NULL;
		}
		timerlist->rbtree.rb_node = NULL;
		timerlist->first = NULL;
		timerlist->num_add = 0;
		timerlist->num_add_compare = 0;
		timerlist->num_add_rotate = 0;
		timerlist->num_del = 0;
		timerlist->num_del_rotate = 0;
	}

	return timerlist;
}

static void timer_mgr_free_timers(struct timer_mgr_list *timerlist)
{
	struct rb_node *node = NULL;
	int cnt = 0;

	mutex_lock(timerlist->lock);
	while ((node = timerlist->first) != NULL) {
		struct timer_mgr_timer *entry;

		entry = rb_entry(node, struct timer_mgr_timer, node);
		timerlist->first = rb_next(node);
		timerlist->num_del++;
		rb_erase(node, &(timerlist->rbtree));
		free(entry);
		timerlist->count--;

		cnt++;
	}
	mutex_unlock(timerlist->lock);
	LOG("TimerMGR(%s): %d scheduled timers removed!", timerlist->name, cnt);
}

static void timer_mgr_free_timerlist(struct timer_mgr_list *timerlist)
{
	if (timerlist) {
		LOG("TimerMGR(%s): deregistered: callback at %p",
				timerlist->name, timerlist->callback);
		if (timerlist->lock)
			mutex_close(timerlist->lock);
		free(timerlist);
	}
}

void *timer_mgr_create_timerlist(void *hd, int timesource,
		timer_callback callback, void *arg, const char *name)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;
	struct timer_mgr_list *timerlist, *entry;

	if (mgr && ((timesource < 0) || (timesource >= TIMERMGR_SOURCE_MAXIMUM))) {
		LOGERROR("TimerMGR: Invalid time source: %d.", timesource);
		return NULL;
	}

	timerlist = timer_mgr_allocate_timerlist(timesource, callback, arg, name);
	if (timerlist == NULL) {
		LOGERROR("TimerMGR: Insufficient memory: unable to create timerlist");
		return NULL;
	}

	if (mgr) {
		mutex_lock(mgr->locks[timesource]);
		//list_for_each_entry(entry, &mgr->timerlists[timesource], list) {
		for (entry = list_entry((&mgr->timerlists[timesource])->next,
			struct timer_mgr_list, list);
			&entry->list != (&mgr->timerlists[timesource]);
			entry = list_entry(entry->list.next,
			struct timer_mgr_list, list))
		{
			if ((entry->callback == callback) &&
					(strcmp(entry->name, name) == 0)) {
				mutex_unlock(mgr->locks[timesource]);
				timer_mgr_free_timerlist(timerlist);
				LOGERROR("TimerMGR(%s): Already registered: callback at %p",
						name, callback);
				return NULL;
			}
		}
		list_add(&timerlist->list, &mgr->timerlists[timesource]);
		mutex_unlock(mgr->locks[timesource]);
	}
	LOG("TimerMGR(%s): registered: callback at %p", name, callback);

	return timerlist;
}

void timer_mgr_release_timerlist(void *hd, void *timerlist)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;
	struct timer_mgr_list *tlist = (struct timer_mgr_list *)timerlist;

	if (tlist) {
		timer_mgr_free_timers(tlist);

		if (mgr) {
			mutex_lock(mgr->locks[tlist->timesource]);
			list_del(&tlist->list);
			mutex_unlock(mgr->locks[tlist->timesource]);
		}

		timer_mgr_free_timerlist(tlist);
	}
}

static struct timer_mgr_timer *timer_mgr_allocate_timer(unsigned int interval,
		unsigned int expire_s, unsigned int expire_ns, void *data)
{
	struct timer_mgr_timer *entry;

	entry = (struct timer_mgr_timer *)malloc(sizeof(*entry));
	if (entry) {
		memset(entry, 0, sizeof(*entry));

		entry->interval = interval;
		entry->expire_s = expire_s;
		entry->expire_ns = expire_ns;
		entry->data = data;

		INIT_LIST_HEAD(&entry->list);
	}

	return entry;
}

static void timer_mgr_insert_timer(struct timer_mgr_list *timerlist,
		struct timer_mgr_timer *entry)
{
	struct rb_node **where, *parent = NULL;
	int leftmost = 1;

	entry->timerlist = timerlist;

	/* Figure out where to put new node */
	where = &(timerlist->rbtree.rb_node);
	while (*where) {
		long long diff;
		struct timer_mgr_timer *current;

		current = rb_entry(*where, struct timer_mgr_timer, node);
		parent = *where;

		diff = time_diff(current->expire_s, current->expire_ns,
				entry->expire_s, entry->expire_ns);
		timerlist->num_add_compare++;

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
		timerlist->first = &(entry->node);
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&(entry->node), parent, where);
	timerlist->num_add++;
	rb_insert_color(&(entry->node),
			&(timerlist->rbtree));

	timerlist->count++;
}

static void *_timer_mgr_add_timer_periodic(void *timerlist,
		unsigned int interval, void *data, int lock)
{
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct timer_mgr_timer *entry = NULL;

	if (tlist) {
		unsigned int s = tlist->last_s;

		if (interval < 1) {
			LOGERROR("TimerMGR(%s): Adding periodic timer: Invalid interval %u",
					tlist->name, interval);
			return NULL;
		}

		s = (s / interval) * interval;
		s += interval;

		entry = timer_mgr_allocate_timer(interval, s, 0, data);
		if (entry) {
			if (lock)
				mutex_lock(tlist->lock);
			timer_mgr_insert_timer(tlist, entry);
			if (lock)
				mutex_unlock(tlist->lock);
			LOGDEBUG("TimerMGR(%s): Periodic timer added: interval %u seconds",
					tlist->name, interval);
		}
	}

	return entry;
}

void *timer_mgr_add_timer_periodic(void *timerlist,
		unsigned int interval, void *data)
{
	return _timer_mgr_add_timer_periodic(timerlist, interval, data, 1);
}

void *timer_mgr_add_timer_periodic_nolock(void *timerlist,
		unsigned int interval, void *data)
{
	return _timer_mgr_add_timer_periodic(timerlist, interval, data, 0);
}

static void *_timer_mgr_add_timer(void *timerlist,
		unsigned int expire_s, unsigned int expire_ns, void *data, int lock)
{
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct timer_mgr_timer *entry = NULL;

	if (tlist) {
		entry = timer_mgr_allocate_timer(0, expire_s, expire_ns, data);
		if (entry) {
			if (lock)
				mutex_lock(tlist->lock);
			timer_mgr_insert_timer(tlist, entry);
			if (lock)
				mutex_unlock(tlist->lock);
			LOGDEBUG("TimerMGR(%s): Timer added: expire at %u.%u",
					tlist->name, expire_s, expire_ns);
		}
	}

	return entry;
}

void *timer_mgr_add_timer(void *timerlist,
		unsigned int expire_s, unsigned int expire_ns, void *data)
{
	return _timer_mgr_add_timer(timerlist, expire_s, expire_ns, data, 1);
}

void *timer_mgr_add_timer_nolock(void *timerlist,
		unsigned int expire_s, unsigned int expire_ns, void *data)
{
	return _timer_mgr_add_timer(timerlist, expire_s, expire_ns, data, 0);
}

static void *_timer_mgr_add_timer_by_offset(void *timerlist,
		unsigned int expire_offset_s, unsigned int expire_offset_ns,
		void *data, int lock)
{
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct timer_mgr_timer *entry = NULL;

	if (tlist) {
		unsigned int s, ns;

		s  = tlist->last_s + expire_offset_s;
		ns = tlist->last_ns + expire_offset_ns;

		while (ns >= 1000000000) {
			s++;
			ns -= 1000000000;
		}

		entry = timer_mgr_allocate_timer(0, s, ns, data);
		if (entry) {
			if (lock)
				mutex_lock(tlist->lock);
			timer_mgr_insert_timer(tlist, entry);
			if (lock)
				mutex_unlock(tlist->lock);
			LOGDEBUG("TimerMGR(%s): Timer added: expire after %u.%u second(s)",
					tlist->name, expire_offset_s, expire_offset_ns);
		}
	}

	return entry;
}

void *timer_mgr_add_timer_by_offset(void *timerlist,
		unsigned int expire_offset_s, unsigned int expire_offset_ns,
		void *data)
{
	return _timer_mgr_add_timer_by_offset(timerlist,
			expire_offset_s, expire_offset_ns, data, 1);
}

void *timer_mgr_add_timer_by_offset_nolock(void *timerlist,
		unsigned int expire_offset_s, unsigned int expire_offset_ns,
		void *data)
{
	return _timer_mgr_add_timer_by_offset(timerlist,
			expire_offset_s, expire_offset_ns, data, 0);
}

void _timer_mgr_delete_timer(void *timerlist, void *timer, int lock, int release)
{
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct timer_mgr_timer *entry = (struct timer_mgr_timer *)timer;
	struct rb_node *node = NULL;

	if (tlist) {
		if (lock)
			mutex_lock(tlist->lock);
		if (entry) {
			node = &(entry->node);
			if (tlist->first == node)
				tlist->first = rb_next(node);
			tlist->num_del++;
			rb_erase(node, &(tlist->rbtree));
		}
		tlist->count--;
		if (lock)
			mutex_unlock(tlist->lock);

		LOGDEBUG("TimerMGR(%s): Timer deleted: interval %u, expire %u.%u",
				tlist->name,
				entry->interval, entry->expire_s, entry->expire_ns);
		if (release)
			free(entry);
	}
}

void timer_mgr_delete_timer(void *timerlist, void *timer)
{
	_timer_mgr_delete_timer(timerlist, timer, 1, 1);
}

void timer_mgr_delete_timer_nolock(void *timerlist, void *timer)
{
	_timer_mgr_delete_timer(timerlist, timer, 0, 1);
}

void *_timer_mgr_update_timer(void *timerlist, void *timer,
		unsigned int expire_s, unsigned int expire_ns, int lock)
{
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct timer_mgr_timer *entry = (struct timer_mgr_timer *)timer;

	if (tlist && entry) {
		if (lock)
			mutex_lock(tlist->lock);
		_timer_mgr_delete_timer(timerlist, timer, 0, 0);
		entry->expire_s = expire_s;
		entry->expire_ns = expire_ns;
		timer_mgr_insert_timer(tlist, entry);
		if (lock)
			mutex_unlock(tlist->lock);
		LOGDEBUG("TimerMGR(%s): Timer updated: expire at %u.%u",
				tlist->name, expire_s, expire_ns);
	}

	return entry;
}

void *timer_mgr_update_timer(void *timerlist, void *timer,
		unsigned int expire_s, unsigned int expire_ns)
{
	return _timer_mgr_update_timer(timerlist, timer, expire_s, expire_ns, 1);
}

void *timer_mgr_update_timer_nolock(void *timerlist, void *timer,
		unsigned int expire_s, unsigned int expire_ns)
{
	return _timer_mgr_update_timer(timerlist, timer, expire_s, expire_ns, 0);
}

void timer_mgr_run_timerlist(void *hd, void *timerlist,
		unsigned int s, unsigned int ns)
{
	struct timer_mgr_hd    *mgr = (struct timer_mgr_hd *)hd;
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct rb_node *node;

	if (!tlist)
		return;

	if (mgr) {
		mutex_lock(mgr->locks[tlist->timesource]);
	}
	if (tlist->timesource == TIMERMGR_SOURCE_LOCAL) {
		struct timeval tv;

		gettimeofday(&tv, NULL);
		s  = tv.tv_sec;
		ns = tv.tv_usec * 1000;
	}

	if ((ns == tlist->last_ns) && (s == tlist->last_s)) {
		if (mgr) {
			mutex_unlock(mgr->locks[tlist->timesource]);
		}
		return;
	}

	tlist->last_s = s;
	tlist->last_ns = ns;

	mutex_lock(tlist->lock);
	while ((node = tlist->first) != NULL) {
		struct timer_mgr_timer *entry;

		entry = rb_entry(node, struct timer_mgr_timer, node);
		if (time_diff(s, ns, entry->expire_s, entry->expire_ns) > 0)
			break;

		tlist->first = rb_next(node);
		tlist->num_del++;
		rb_erase(node, &(tlist->rbtree));
		tlist->count--;

		/* If entry->expire_s equals entry->interval, it has to be the first
		 * running of a timer whose container timer list has external time
		 * source. In this case we don't run this periodical timer, and
		 * re-initialize it's expire time.
		 */
		if ((entry->interval == 0) || (entry->expire_s > entry->interval))
			tlist->callback(s, ns, entry->data, tlist->arg);

		if (entry->interval > 0) {
			if (entry->expire_s > entry->interval) {
				entry->expire_s += entry->interval;
			}
			else {
				entry->expire_s = ((s + entry->interval - 1) / entry->interval)
					* entry->interval;
			}
			timer_mgr_insert_timer(tlist, entry);
		}
		else {
			free(entry);
		}
	}
	mutex_unlock(tlist->lock);

	if (mgr) {
		mutex_unlock(mgr->locks[tlist->timesource]);
	}
}

void timer_mgr_force_timeout(void *hd, void *timerlist,
		unsigned int s, unsigned int ns, timer_callback callback)
{
	struct timer_mgr_hd    *mgr = (struct timer_mgr_hd *)hd;
	struct timer_mgr_list  *tlist = (struct timer_mgr_list *)timerlist;
	struct rb_node *node;

	if (!tlist)
		return;

	if (mgr) {
		mutex_lock(mgr->locks[tlist->timesource]);
	}
	if (tlist->timesource == TIMERMGR_SOURCE_LOCAL) {
		struct timeval tv;

		gettimeofday(&tv, NULL);
		s  = tv.tv_sec;
		ns = tv.tv_usec * 1000;
	}

	mutex_lock(tlist->lock);
	if ((node = tlist->first) != NULL) {
		struct timer_mgr_timer *entry;

		entry = rb_entry(node, struct timer_mgr_timer, node);

		tlist->first = rb_next(node);
		tlist->num_del++;
		rb_erase(node, &(tlist->rbtree));
		tlist->count--;

		if (callback)
			callback(s, ns, entry->data, tlist->arg);
		else
			tlist->callback(s, ns, entry->data, tlist->arg);

		free(entry);
	}
	mutex_unlock(tlist->lock);

	if (mgr) {
		mutex_unlock(mgr->locks[tlist->timesource]);
	}
}

/* Wrappers to predefined timerlists */
void *timer_mgr_add_pktq_timer(void *hd, unsigned int interval, void *data)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;
	void *timer = NULL;

	if (mgr) {
		timer = timer_mgr_add_timer_periodic(mgr->pktq, interval, data);
	}

	return timer;
}

void timer_mgr_del_pktq_timer(void *hd, void *timer)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;

	if (mgr) {
		timer_mgr_delete_timer(mgr->pktq, timer);
	}
}

void *timer_mgr_add_hashtbl_timer(void *hd, unsigned int interval, void *data)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;
	void *timer = NULL;

	if (mgr) {
		timer = timer_mgr_add_timer_periodic(mgr->hashtbl, interval, data);
	}

	return timer;
}

void timer_mgr_del_hashtbl_timer(void *hd, void *timer)
{
	struct timer_mgr_hd *mgr = (struct timer_mgr_hd *)hd;

	if (mgr) {
		timer_mgr_delete_timer(mgr->hashtbl, timer);
	}
}

