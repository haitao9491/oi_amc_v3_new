#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "seqlst.h"

#define SEQ_LST_TIMEOUT   8

struct seq_lst {
	unsigned int     ipaddr;
	int              port;
	unsigned short   seq;
	unsigned int     stime;
	struct list_head node;
};

struct list_head       seqlst;
static pthread_mutex_t seqlst_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t seq_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned short seq = 0;


int seq_lst_init(void) 
{
	INIT_LIST_HEAD(&seqlst);
	
	return 0;
}

int seq_lst_add(unsigned int ipaddr, int port, unsigned short seq)
{
	struct seq_lst *entry;

	pthread_mutex_lock(&seqlst_mutex);

	entry = malloc(sizeof(*entry));
	if (entry) {
		memset(entry, 0, sizeof(*entry));

		INIT_LIST_HEAD(&entry->node);
		entry->ipaddr = ipaddr;
		entry->port   = port;
		entry->stime  = time(NULL);
		entry->seq    = seq;

		list_add(&(entry->node), &seqlst);

		pthread_mutex_unlock(&seqlst_mutex);

		return 0;
	}

	pthread_mutex_unlock(&seqlst_mutex);
	return -1;
}

int seq_lst_check_timeout(unsigned int *ipaddr, int *port, unsigned short *seq)
{
	struct seq_lst *entry;
	struct list_head *p, *n;
	unsigned int curtime = 0;

	if (!ipaddr || !port || !seq)
		return -1;

	pthread_mutex_lock(&seqlst_mutex);

	list_for_each_safe(p, n, &seqlst) {
		entry = list_entry(p, struct seq_lst, node);
		curtime = time(NULL);
		if ((curtime - entry->stime) > SEQ_LST_TIMEOUT) {
			LOGERROR("seq_lst_check_timeout: ipaddr %u, port %u, seq %u, curtime %u, stime %u timeout\n", 
					entry->ipaddr, entry->port, entry->seq, curtime, entry->stime);
			*ipaddr = entry->ipaddr;
			*port   = entry->port;
			*seq    = entry->seq;
			list_del(p);
			free(entry);
			pthread_mutex_unlock(&seqlst_mutex);
			return 1;
		}
	}

	pthread_mutex_unlock(&seqlst_mutex);
	return -1;
}

#if 0
int seq_lst_del(unsigned int ipaddr, unsigned short port, unsigned short seq)
{
	struct seq_lst   *entry;
	struct list_head *p, *n;
	unsigned int      curtime = 0;
	int		  rc = 0;

	list_for_each_safe(p, n, &seqlst) {
		entry = list_entry(p, struct seq_lst, node);
		if (entry->ipaddr == ipaddr && entry->port == port && entry->seq == seq) {
			curtime = time(NULL);

			if ((curtime - entry->stime) > SEQ_LST_TIMEOUT) {
				LOGERROR("seq_lst_del: ipaddr %u, port %u, seq %u, curtime %u, stime %u timeout\n", 
						entry->ipaddr, entry->port, entry->seq, curtime, entry->stime);
				rc = 1;
			}

			list_del(p);
			free(entry);

			return rc;
		}
	}

	return -1;
}
#else
int seq_lst_del(unsigned int ipaddr, int port, unsigned short seq)
{
	struct seq_lst   *entry;
	struct list_head *p, *n;

	pthread_mutex_lock(&seqlst_mutex);
	list_for_each_safe(p, n, &seqlst) {
		entry = list_entry(p, struct seq_lst, node);
		if (entry->ipaddr == ipaddr && entry->port == port && entry->seq == seq) {
			list_del(p);
			free(entry);
			pthread_mutex_unlock(&seqlst_mutex);

			return 0;
		}
	}

	pthread_mutex_unlock(&seqlst_mutex);
	return -1;
}
#endif

static void seq_lst_del_all()
{
	struct seq_lst   *entry;
	struct list_head *p, *n;

	pthread_mutex_lock(&seqlst_mutex);

	list_for_each_safe(p, n, &seqlst) {
		entry = list_entry(p, struct seq_lst, node);
		list_del(p);
		free(entry);
	}

	pthread_mutex_unlock(&seqlst_mutex);
}

void seq_lst_exit(void) 
{
	if (list_empty(&seqlst))
		return;

	seq_lst_del_all();
}

void seq_lst_get_seqno(unsigned short *sn) 
{
	pthread_mutex_lock(&seq_mutex);

	if (sn)
		*sn = seq;
	seq++;

	pthread_mutex_unlock(&seq_mutex);
}

