/*
 * (C) Copyright 2012
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * adapter_udp.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "os.h"
#include "aplog.h"
#include "pkt.h"
#include "pktqueue.h"
#include "mutex.h"
#include "cthread.h"
#include "misc.h"
#include "adapter.h"
#include "adapter_udp.h"

#define ADAPTER_UDP_NAME	"Adapter.UDP"
#define ADAPTER_UDP_QUEUE_SIZE	(8192 * 1024)

struct adapter_udp_ctl {
	int	port;
	int   (*running)(void);

	int	us;
	
	void   *rq;
	void   *tq;
	void   *rqlock;
	void   *tqlock;

	void   *rthread;
	void   *tthread;

	int	opkthdr;

	/* statistics */
	unsigned long long rpkts;
	unsigned long long rdrop;
	unsigned long long rok;
	unsigned long long rfull;

	unsigned long long tpkts;
	unsigned long long tdrop;
	unsigned long long tsent;
	unsigned long long tfull;
};

struct adapter_udp_pkt {
	unsigned int ip;
	int          port;         
	pkt_hdr     *ph;
	int          len;
};


static void *adapter_udp_dorcv(void *arg)
{
	struct thread_arg      *targ = (struct thread_arg *)arg;
	struct adapter_udp_ctl *ctl = (struct adapter_udp_ctl *)targ->arg;
	struct adapter_udp_pkt *upkt;
	struct sockaddr_in      sa;
	unsigned char  buffer[64 * 1024];
	int            rc = 0, addrlen;

	memset(&sa, 0, sizeof(sa));
	addrlen = sizeof(sa);

	LOG("Thread(dorcv): [%u]: Starting ...", ctl->port);
	while (ctl->running() && targ->flag) {
		rc = recvfrom(ctl->us, buffer, sizeof(buffer), 0, (struct sockaddr *)&sa, (socklen_t *)&addrlen);
		if (rc < 0) {
			if (errno == EAGAIN) {
#if defined(__tilegx__)
				SLEEP_US(100);
#else
				SLEEP_US(10);
#endif
				continue;
			}
		}
		else if (rc == 0) {
			continue;
		}

		ctl->rpkts++;

#if 0
		/* Got one packet */
		if (pkt_verify(buffer, rc) <= 0) {
			ctl->rdrop++;
			goto adapter_udp_dorcv_next;
		}
#endif

		upkt = (struct adapter_udp_pkt *)malloc(sizeof(*upkt));
		if (upkt == NULL) {
			ctl->rdrop++;
			goto adapter_udp_dorcv_next;
		}

		if (ctl->opkthdr) {
			rc += sizeof(pkt_hdr);
		}

		upkt->ph = malloc(rc);
		if (upkt->ph == NULL) {
			ctl->rdrop++;
			free(upkt);
			goto adapter_udp_dorcv_next;
		}

		upkt->ip   = sa.sin_addr.s_addr;
		upkt->port = sa.sin_port;
		upkt->len = rc;
		if (ctl->opkthdr) {
			memset((unsigned char *)upkt->ph, 0, sizeof(pkt_hdr));
			pkthdr_set_sync(upkt->ph);
			pkthdr_set_plen(upkt->ph, rc);
			memcpy((unsigned char *)upkt->ph + sizeof(pkt_hdr), buffer, rc - sizeof(pkt_hdr));
		}
		else {
			memcpy((unsigned char *)upkt->ph, buffer, rc);
		}

		mutex_lock(ctl->rqlock);

		if (pkt_push(ctl->rq, upkt, sizeof(*upkt) + rc) <= 0) {
			mutex_unlock(ctl->rqlock);

			ctl->rfull++;
			free(upkt->ph);
			free(upkt);
			goto adapter_udp_dorcv_next;
		}

		mutex_unlock(ctl->rqlock);
		ctl->rok++;

adapter_udp_dorcv_next:
		memset(&sa, 0, sizeof(sa));
	}
	LOG("Thread(dorcv): [%u]: Stopped.", ctl->port);

	return NULL;
}

static void *adapter_udp_dosnd(void *arg)
{
	struct thread_arg      *targ = (struct thread_arg *)arg;
	struct adapter_udp_ctl *ctl = (struct adapter_udp_ctl *)targ->arg;
	struct adapter_udp_pkt *upkt;
	struct sockaddr_in      sa;
	int    rc = 0;

	LOG("Thread(dosnd): [%u]: Starting ...", ctl->port);
	while (ctl->running() && targ->flag) {
		mutex_lock(ctl->tqlock);
		upkt = pkt_pop(ctl->tq);
		mutex_unlock(ctl->tqlock);

		if (upkt == NULL) {
#if defined(__tilegx__)
			SLEEP_US(10);
#else
			SLEEP_MS(1);
#endif
			continue;
		}

		ctl->tpkts++;

		/* Got one upkt */
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port   = upkt->port;
		sa.sin_addr.s_addr = upkt->ip;

		rc = sendto(ctl->us, upkt->ph, upkt->len, 0, (struct sockaddr *)&sa, sizeof(sa));
		if (rc <= 0)
			ctl->tdrop++;
		else
			ctl->tsent++;
		
		free(upkt->ph);
		free(upkt);
	}
	LOG("Thread(dosnd): [%u]: Stopped.", ctl->port);

	return NULL;
}

static void adapter_udp_exit_clear_pktq(void *pktq)
{
	struct adapter_udp_pkt *upkt = NULL;

	for (;;) {
		upkt = (struct adapter_udp_pkt *)pkt_pop(pktq);
		if (upkt == NULL)
			break;

		if (upkt->ph)
			free(upkt->ph);
		free(upkt);
	}

	pkt_queue_close(pktq);
}

static inline void adapter_udp_show_stat(struct adapter_udp_ctl *ctl)
{
	LOG("Adapter(UDP): [%u]: RX: pkts %llu, drop %llu, ok %llu, full %llu",
			ctl->port, ctl->rpkts, ctl->rdrop, ctl->rok, ctl->rfull);
	LOG("Adapter(UDP): [%u]: TX: pkts %llu, drop %llu, sent %llu, full %llu",
			ctl->port, ctl->tpkts, ctl->tdrop, ctl->tsent, ctl->tfull);
}

static void adapter_udp_exit(struct adapter_udp_ctl *ctl)
{
	if (ctl) {
		adapter_udp_show_stat(ctl);

		if (ctl->tthread)
			thread_close(ctl->tthread);
		if (ctl->rthread)
			thread_close(ctl->rthread);

		if (ctl->tqlock)
			mutex_close(ctl->tqlock);
		if (ctl->rqlock)
			mutex_close(ctl->rqlock);

		if (ctl->tq)
			adapter_udp_exit_clear_pktq(ctl->tq);
		if (ctl->rq)
			adapter_udp_exit_clear_pktq(ctl->rq);

		if (ctl->us != -1)
			close(ctl->us);

		free(ctl);
	}
}

static struct adapter_udp_ctl *adapter_udp_init(void *adap, int port, int (*running)(void))
{
	struct adapter_udp_ctl *ctl;
	int    flag;
	struct sockaddr_in sa;

	ctl = (struct adapter_udp_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL)
		return NULL;
	memset(ctl, 0, sizeof(*ctl));

	ctl->port = port;
	ctl->running = running;

	/* socket */
	ctl->us = socket(PF_INET, SOCK_DGRAM, 0);
	if (ctl->us == -1)
		goto adapter_udp_init_failed;

	flag = fcntl(ctl->us, F_GETFL);
	flag |= O_NONBLOCK;
	if (fcntl(ctl->us, F_SETFL, flag) != 0)
		goto adapter_udp_init_failed;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(ctl->port);
	sa.sin_addr.s_addr = INADDR_ANY;

	if (bind(ctl->us, (struct sockaddr *)&sa, sizeof(sa)) < 0)
		goto adapter_udp_init_failed;

	/* queue */
	ctl->rq = pkt_queue_open(PKTQUEUE_NORMAL, "UDP.RxQ");
	if (ctl->rq == NULL)
		goto adapter_udp_init_failed;
	pkt_queue_set_sizelimit(ctl->rq, ADAPTER_UDP_QUEUE_SIZE);

	ctl->tq = pkt_queue_open(PKTQUEUE_NORMAL, "UDP.TxQ");
	if (ctl->tq == NULL)
		goto adapter_udp_init_failed;
	pkt_queue_set_sizelimit(ctl->tq, ADAPTER_UDP_QUEUE_SIZE);

	/* lock */
	ctl->rqlock = mutex_open("UDP.RxQLock");
	if (ctl->rqlock == NULL)
		goto adapter_udp_init_failed;

	ctl->tqlock = mutex_open("UDP.TxQLock");
	if (ctl->tqlock == NULL)
		goto adapter_udp_init_failed;

	return ctl;
	
adapter_udp_init_failed:
	adapter_udp_exit(ctl);
	return NULL;
}

static int adapter_udp_open(void *adap)
{
	struct adapter_udp_ctl *ctl;

	ctl = (struct adapter_udp_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return -1;

	ctl->rthread = thread_open(adapter_udp_dorcv, ctl);
	if (ctl->rthread == NULL)
		return -1;

	ctl->tthread = thread_open(adapter_udp_dosnd, ctl);
	if (ctl->tthread == NULL)
		return -1;

	return 0;
}

static void *adapter_udp_read_from(void *adap, char *ip, int *port)
{
	struct adapter_udp_ctl *ctl;
	struct adapter_udp_pkt *upkt;
	pkt_hdr *ph;

	ctl = (struct adapter_udp_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return NULL;

	mutex_lock(ctl->rqlock);
	upkt = (struct adapter_udp_pkt *)pkt_pop(ctl->rq);
	mutex_unlock(ctl->rqlock);

	if (upkt == NULL)
		return NULL;

	ph = upkt->ph;
	if (ip)
		ip4addr_str(upkt->ip, ip);
	if (port)
		*port = ntohs(upkt->port);

	free(upkt);

	return ph;
}

static void *adapter_udp_read(void *adap)
{
	return adapter_udp_read_from(adap, NULL, NULL);
}

static int adapter_udp_write_to(void *adap, void *data, int len, char *ip, int port)
{
	struct adapter_udp_ctl *ctl;
	struct adapter_udp_pkt *upkt;

	if ((data == NULL) || (len <= 0) || (ip == NULL) || ((port <= 0) || (port > 65535)))
		return -1;

	ctl = (struct adapter_udp_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return -1;

	upkt = (struct adapter_udp_pkt *)malloc(sizeof(*upkt));
	if (upkt == NULL)
		return -1;

	upkt->ph = malloc(len);
	if (upkt->ph == NULL) {
		free(upkt);
		return -1;
	}

	upkt->ip = inet_addr(ip);
	upkt->port = htons(port);
	memcpy((char *)upkt->ph, data, len);
	upkt->len = len;

	mutex_lock(ctl->tqlock);

	if (pkt_push(ctl->tq, upkt, sizeof(*upkt) + len) <= 0) {
		mutex_unlock(ctl->tqlock);

		ctl->tfull++;

		free(upkt->ph);
		free(upkt);
		return -1;
	}

	mutex_unlock(ctl->tqlock);

	return len;
}

static int adapter_udp_write(void *adap, void *data, int len)
{
	/* implemented in future */

	return -1;
}

static void adapter_udp_freebuf(void *adap, void *buf)
{
	if (buf)
		free(buf);
}

static int adapter_udp_ioctl(void *adap, int code, void *arg)
{
	struct adapter_udp_ctl *ctl;
	int rc = -1;

	ctl = (struct adapter_udp_ctl *)adapter_get_data(adap);
	if (ctl && arg) {
		switch (code) {
			case ADAPTER_UDP_IOCTL_GETSTATS:
				{
					struct udp_stats *st = (struct udp_stats *)arg;

					st->rpkts = ctl->rpkts;
					st->rdrop = ctl->rdrop;
					st->rok = ctl->rok;
					st->rfull = ctl->rfull;

					st->tpkts = ctl->tpkts;
					st->tdrop = ctl->tdrop;
					st->tsent = ctl->tsent;
					st->tfull = ctl->tfull;

					rc = 0;
				}
				break;

			case ADAPTER_UDP_IOCTL_SETOPKTHDR:
				{
					ctl->opkthdr = 1;
				}
				break;

			case ADAPTER_UDP_IOCTL_SETMCINTF:
				{
					struct udp_mcinfo *mci = (struct udp_mcinfo *)arg;
					struct in_addr localintf;

					localintf.s_addr = inet_addr(mci->intf);
					if (setsockopt(ctl->us, IPPROTO_IP, IP_MULTICAST_IF, 
								(char *)&localintf, sizeof(localintf)) == 0) {
						rc = 0;
					}
				}
				break;

			case ADAPTER_UDP_IOCTL_SETMCMEMBER:
				{
					struct udp_mcinfo *mci = (struct udp_mcinfo *)arg;
					struct ip_mreq mreq;

					mreq.imr_multiaddr.s_addr = inet_addr(mci->addr);
					mreq.imr_interface.s_addr = inet_addr(mci->intf);
					if (setsockopt(ctl->us, IPPROTO_IP, IP_ADD_MEMBERSHIP,
								(char *)&mreq, sizeof(mreq)) == 0) {
						rc = 0;
					}
				}
				break;

			default:
				LOGWARN("%s: ioctl: invalid code %d.",
						adapter_get_name(adap), code);
				break;
		}
	}

	return rc;
}

static void adapter_udp_close(void *adap)
{
	struct adapter_udp_ctl *ctl;

	ctl = (struct adapter_udp_ctl *)adapter_get_data(adap);
	if (ctl == NULL)
		return;

	adapter_udp_exit(ctl);
}

void *adapter_register_udp(int port, int (*running)(void))
{
	void  *adap;
	struct adapter_udp_ctl *ctl;

	if (((port <= 0) || (port > 65535)) || !running)
		return NULL;

	if ((adap = adapter_allocate()) == NULL)
		return NULL;
	adapter_set_name(adap, ADAPTER_UDP_NAME);

	ctl = adapter_udp_init(adap, port, running);
	if (ctl == NULL) {
		adapter_close(adap);
		return NULL;
	}
	adapter_set_data(adap, ctl);

	adapter_set_open(adap, adapter_udp_open);
	adapter_set_read(adap, adapter_udp_read);
	adapter_set_read_from(adap, adapter_udp_read_from);
	adapter_set_write(adap, adapter_udp_write);
	adapter_set_write_to(adap, adapter_udp_write_to);
	adapter_set_freebuf(adap, adapter_udp_freebuf);
	adapter_set_ioctl(adap, adapter_udp_ioctl);
	adapter_set_close(adap, adapter_udp_close);

	return adap;
}

