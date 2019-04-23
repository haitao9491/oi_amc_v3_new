/*
 *
 * adapter_z006.c - Implementation of adapter interface of ip probe.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include "os.h"
#include "aplog.h"
#include "pkt.h"
#include "pkttype.h"
#include "pktqueue.h"
#include "cthread.h"
#include "adapter.h"
#include "adapter_z006.h"

#define PAE_MAX_DEVICE 16

#define PAE_DEV_NAME   "pae"
#define PAE_DEV_BUFSZ  (16 * 1024)

#define PAE_IOC_MAGIC     'P'
#define PAE_IOC_GETSTAT   _IOR(PAE_IOC_MAGIC, 0x0, void *)

struct pae_stat {
	unsigned long long rpkts, rbytes;

	/* Too many skb pending (haven't been received by ap) */
	unsigned long long rfulpkts, rfulbytes;

	/* Non-linear skb */
	unsigned long long rnlpkts, rnlbytes;
};

struct adapter_z006_dev {
	char devname[128];

	char buf[PAE_DEV_BUFSZ];
	int  pkts;
	int  offset;

	void *rxthread;
	int   fd;

	void *rq;
	int (*check_running)(void);
};

struct adapter_z006_ctl {
	int   startdevnum;
	int   devnum;
	struct adapter_z006_dev dev[PAE_MAX_DEVICE];

	void *rq;
	int (*check_running)(void);
};

static void adapter_z006_exit(struct adapter_z006_ctl *ctl)
{
	struct adapter_z006_dev *dev;
	int i;

	if (ctl) {
		if (ctl->rq) {
			pkt_queue_close(ctl->rq);
			ctl->rq = NULL;
		}

		for (i = ctl->startdevnum, dev = &(ctl->dev[ctl->startdevnum]); i < ctl->devnum; i++, dev++) {
			if (dev->fd != -1) {
				close(dev->fd);
				dev->fd = -1;
			}
		}

		free(ctl);
	}
}

static struct adapter_z006_ctl *adapter_z006_init(int startdevnum, int devnum,
		int (*check_running)(void))
{
	struct adapter_z006_ctl *ctl;
	struct adapter_z006_dev *dev;
	int i;

	if ((startdevnum < 0) || (startdevnum >= PAE_MAX_DEVICE) || (devnum < 1) || ((startdevnum + devnum) > PAE_MAX_DEVICE))
		return NULL;

	ctl = (struct adapter_z006_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(struct adapter_z006_ctl));
	for (i = 0; i < PAE_MAX_DEVICE; i++)
		ctl->dev[i].fd = -1;

	ctl->check_running = check_running;
	ctl->startdevnum = startdevnum;
	ctl->devnum = devnum;

	ctl->rq = pkt_queue_open(PKTQUEUE_MALLOC_BUFFER, (char *)"Adapter.Z006.Rx");
	if (ctl->rq == NULL) {
		LOGERROR("Adapter.Z006: Failed to open packet queue.");
		adapter_z006_exit(ctl);
		return NULL;
	}

	for (i = ctl->startdevnum, dev = &(ctl->dev[ctl->startdevnum]); i < ctl->devnum; i++, dev++) {
		sprintf(dev->devname, "/dev/%s%d", PAE_DEV_NAME, i);
		if ((dev->fd = open(dev->devname, O_RDWR)) == -1) {
			LOGERROR("Adapter.Z006: Failed to open device %s", dev->devname);
			adapter_z006_exit(ctl);
			return NULL;
		}
		dev->rq = ctl->rq;
		dev->check_running = ctl->check_running;
	}

	return ctl;
}

void *adapter_z006_dorcv(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	struct adapter_z006_dev *dev = (struct adapter_z006_dev *)(targ->arg);

	while (dev->check_running() && targ->flag) {
		if (dev->pkts <= 0) {
			dev->offset = 0;
			dev->pkts = read(dev->fd, dev->buf, PAE_DEV_BUFSZ);
			if (dev->pkts <= 0) {
				SLEEP_US(1000);
			}
		}
		else {
			pkt_hdr *ph;

			ph = (pkt_hdr *)(dev->buf + dev->offset);
			if (pkt_push(dev->rq, ph, pkthdr_get_plen(ph)) <= 0) {
			}

			dev->offset += pkthdr_get_len(ph);
			dev->pkts--;
		}
	}

	return NULL;
}

static void adapter_z006_close(void *adap)
{
	struct adapter_z006_ctl *ctl;
	struct adapter_z006_dev *dev;
	int i;

	ctl = (struct adapter_z006_ctl *)adapter_get_data(adap);
	if (!ctl)
		return;

	for (i = ctl->startdevnum, dev = &(ctl->dev[ctl->startdevnum]); i < ctl->devnum; i++, dev++) {
		if (dev->rxthread != NULL) {
			thread_close(dev->rxthread);
			dev->rxthread = NULL;
			LOGINFO("Adapter.Z006: device %s: receive thread terminated.",
					dev->devname);
		}
	}

	adapter_z006_exit(ctl);
}

static int adapter_z006_open(void *adap)
{
	struct adapter_z006_ctl *ctl;
	struct adapter_z006_dev *dev;
	int i;

	ctl = (struct adapter_z006_ctl *)adapter_get_data(adap);
	if (!ctl)
		return -1;

	for (i = ctl->startdevnum, dev = &(ctl->dev[ctl->startdevnum]); i < ctl->devnum; i++, dev++) {
		dev->rxthread = thread_open(
				(LPTHREAD_START_ROUTINE)adapter_z006_dorcv, dev);
		if (dev->rxthread == NULL) {
			LOGERROR("Adapter.Z006: Failed to open thread for device %s",
					dev->devname);
			return -1;
		}
	}

	return 0;
}

static void *adapter_z006_read(void *adap)
{
	struct adapter_z006_ctl *ctl;

	ctl = (struct adapter_z006_ctl *)adapter_get_data(adap);
	if (!ctl || !ctl->rq)
		return NULL;

	return pkt_pop(ctl->rq);
}

void adapter_z006_freebuf(void *adap, void *buf)
{
	if (buf)
		free(buf);
}

void *adapter_register_z006s(int startdevnum, int devnum, int (*check_running)(void))
{
	void *adap;
	struct adapter_z006_ctl *ctl;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;

	ctl = adapter_z006_init(startdevnum, devnum, check_running);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_name(adap, "Adapter.Z006");
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_z006_open);
	adapter_set_read(adap, adapter_z006_read);
	adapter_set_close(adap, adapter_z006_close);
	adapter_set_freebuf(adap, adapter_z006_freebuf);

	return adap;
}

void *adapter_register_z006(int devnum, int (*check_running)(void))
{
	void *adap;
	struct adapter_z006_ctl *ctl;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;

	ctl = adapter_z006_init(0, devnum, check_running);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_name(adap, "Adapter.Z006");
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_z006_open);
	adapter_set_read(adap, adapter_z006_read);
	adapter_set_close(adap, adapter_z006_close);
	adapter_set_freebuf(adap, adapter_z006_freebuf);

	return adap;
}
