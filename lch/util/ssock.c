/*
 *
 * ssock.c - Socket interface (server) with data buffer support
 */

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define NLGWR

#include "os.h"
#include "aplog.h"
#if defined(SSOCK_SYNCED)
#include "mutex.h"
#endif
#include "pkt.h"
#include "pktqueue.h"
#include "list.h"
#include "cthread.h"

#if defined(CSSOCK_TX_PFIFO)
#include "pfifo.h"
#endif

#define SSOCK_IMPLEMENTATION
#include "cssock.h"

typedef struct {
	SOCKET fd;
	unsigned int ip;
	int  port;
	int  backlog;

	unsigned int flag;

#if defined(SSOCK_SYNCED)
	/* lock - protect the connection list */
	void *lock;
#endif
	struct list_head peers;

	int rbs, tbs;

	void *doaccept;

	int (*check_running)(void);
	int (*pktsync)(void *buf, int size);

	CSSOCK_CONN_CB conncb;
	void	      *connarg;
	CSSOCK_DISC_CB disccb;
	void	      *discarg;
} sock_svc_t;

typedef struct {
	sock_svc_t *svc;

	int error;

	sock_connection_t *conn;

	void *docheck;
	void *doflush_rx;
	void *doflush_tx;

	struct list_head list;
} sock_peer;

static __inline void ssock_close_connection(sock_connection_t *conn)
{
	cssock_conn_stat(conn);

	if(conn->fd != INVALID_SOCKET) {
		if (conn->disccb) {
			LOG("Connection [%d %s@%d - %s@%d]: (callback) disconnected: Socket error (I/O)",
					conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);
			conn->disccb(conn->localipstr, conn->localport, conn->ip, conn->port, conn->discarg);
		}
		CLOSESOCKET(conn->fd);
	}

	sock_exit_rbuf(conn);
	free(conn);
}

static __inline void ssock_release_peer(sock_peer *peer)
{
	sock_connection_t *conn;

	if (!peer)
		return;

	conn = peer->conn;

	LOGDEBUG("ssock: Connection [%d %s@%d - %s@%d]: releasing",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	if (peer->docheck) {
		thread_close(peer->docheck);
		LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (check) closed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
	}

	if (peer->doflush_rx) {
		thread_close(peer->doflush_rx);
		LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (flush_rx) closed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
	}

	if (peer->doflush_tx) {
		thread_close(peer->doflush_tx);
		LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (flush_tx) closed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
	}

	if (peer->conn) {
		ssock_close_connection(peer->conn);
	}

	free(peer);
}

static void *ssock_peer_do_check(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	sock_peer *peer = (sock_peer *)targ->arg;
	pkt_hdr hb;
	int i = 0;

	memset(&hb, 0, sizeof(hb));
	pkthdr_set_sync(&hb);
	pkthdr_set_dlen(&hb, 0);
	pkthdr_set_type(&hb, PKT_TYPE_HEARTBEAT);
	pkthdr_set_subtype(&hb, PKT_SUBTYPE_HEARTBEAT_SOCKET);

	while (peer->svc->check_running() && (peer->error == 0) && targ->flag) {
		SLEEP_MS(200);
		++i;

		if ((i & 0x001f) == 0 && 
			((peer->svc->flag & CSSOCK_NO_HEARTBEAT) != CSSOCK_NO_HEARTBEAT)) {
			/* Send heartbeat packet */
			sock_write_to_buffer(peer->conn, (char *)&hb, sizeof(pkt_hdr));
		}

		if ((i & 0x07ff) == 0) {
			/* Output the socket level statistics about every 410 seconds */
			cssock_conn_stat(peer->conn);
		}
	}

	LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (check) terminated.",
			peer->conn->fd,
			peer->conn->localipstr, peer->conn->localport,
			peer->conn->ip, peer->conn->port);
	return NULL;
}

static void *ssock_peer_do_flush_rx(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	sock_peer *peer = (sock_peer *)targ->arg;
	int rc;

	while (peer->svc->check_running() && (peer->error == 0) && targ->flag) {
		rc = sock_read_from_socket(peer->conn);
		if (rc > 0) {
			continue;
		}
		else if (rc == 0) {
			SLEEP_MS(5);
		}
		else {
			/* ERROR occurred on this connection! */
			peer->error = 1;
		}
	}

	LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (flush_rx) terminated.",
			peer->conn->fd,
			peer->conn->localipstr, peer->conn->localport,
			peer->conn->ip, peer->conn->port);
	return NULL;
}

static void *ssock_peer_do_flush_tx(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	sock_peer *peer = (sock_peer *)targ->arg;
	sock_connection_t *conn = peer->conn;
	char *ph;
	int rc;

	while (peer->svc->check_running() && (peer->error == 0) && targ->flag) {
		if (conn->txtmpbuflen > 0) {
			rc = sock_write_buf_socket(conn, conn->txtmpbuf, conn->txtmpbuflen);
			if (rc < 0) {
				/* ERROR occurred! */
				peer->error = 1;
				continue;
			}

			if (rc > 0) {
				if (rc < conn->txtmpbuflen) {
					int remain = conn->txtmpbuflen - rc;

					memcpy(conn->txtmpbuf, conn->txtmpbuf + rc, remain);
					conn->txtmpbuflen = remain;
				}
				else {
					conn->txtmpbuflen = 0;
				}
			}

			SLEEP_MS(1);
			continue;
		}

#if !defined(CSSOCK_TX_PFIFO)
		ph = pkt_pop(conn->tq);
		if (!ph) {
			SLEEP_MS(5);
			continue;
		}

		rc = sock_write_pkt_socket(conn, ph);
		free(ph);
#else
		memset(conn->txbuf, 0, sizeof(pkt_hdr));
		conn->txbuflen = sizeof(pkt_hdr);

		mutex_lock(conn->wlock);
		while (conn->txbuflen < (sizeof(pkt_hdr) + CSSOCK_TX_ENTRYSZ)) {
			ph = pfifo_read(conn->tq);
			if (!ph)
				break;

			rc = pkthdr_get_plen((pkt_hdr *)ph);
			if (conn->flag & (CSSOCK_CUSTOMIZED_PKTSYNC | CSSOCK_PAYLOAD_ONLY)) {
				ph += sizeof(pkt_hdr);
				rc -= sizeof(pkt_hdr);
			}

			if (rc == 0) {
				pfifo_read_commit(conn->tq);
				continue;
			}

			if (rc > (CSSOCK_TXBUF_SIZE - sizeof(pkt_hdr))) {
				/* we can't support packet with length more than 65516 bytes. */
				pfifo_read_commit(conn->tq);
				continue;
			}

			if (conn->txbuflen + rc <= CSSOCK_TXBUF_SIZE) {
				memcpy(conn->txbuf + conn->txbuflen, ph, rc);
				conn->txbuflen += rc;
				pfifo_read_commit(conn->tq);
			}

			if (rc >= CSSOCK_TX_ENTRYSZ)
				break;
		}
		mutex_unlock(conn->wlock);

		if (conn->txbuflen == sizeof(pkt_hdr)) {
			SLEEP_MS(1);
			continue;
		}

		pkthdr_set_sync((pkt_hdr *)conn->txbuf);
		pkthdr_set_plen((pkt_hdr *)conn->txbuf, conn->txbuflen);

		rc = sock_write_pkt_socket(conn, conn->txbuf);
#endif

		if (rc < 0) {
			/* ERROR occurred! */
			peer->error = 1;
		}
	}

	LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (flush_tx) terminated.",
			conn->fd,
			conn->localipstr, conn->localport, conn->ip, conn->port);
	return NULL;
}

static __inline sock_peer *ssock_add_peer(sock_svc_t *svc,
		sock_connection_t *conn)
{
	sock_peer *peer;

	if (!svc || !conn)
		return NULL;

	peer = (sock_peer *)malloc(sizeof(*peer));
	if (peer == NULL)
		return NULL;

	memset(peer, 0, sizeof(*peer));
	peer->svc = svc;
	peer->conn = conn;
	INIT_LIST_HEAD(&(peer->list));

	peer->docheck = thread_open(
			(LPTHREAD_START_ROUTINE)ssock_peer_do_check, (void *)peer);
	if (peer->docheck == NULL) {
		LOGERROR("ssock: Connection [%d %s@%d - %s@%d]: "
				"Failed starting thread (check).",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		goto ssock_add_peer_error_free;
	}
	LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (check) started.",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	peer->doflush_rx = thread_open(
			(LPTHREAD_START_ROUTINE)ssock_peer_do_flush_rx, (void *)peer);
	if (peer->doflush_rx == NULL) {
		LOGERROR("ssock: Connection [%d %s@%d - %s@%d]: "
				"Failed starting thread (flush_rx).",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		goto ssock_add_peer_error_close_docheck;
	}
	LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (flush_rx) started.",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	peer->doflush_tx = thread_open(
			(LPTHREAD_START_ROUTINE)ssock_peer_do_flush_tx, (void *)peer);
	if (peer->doflush_tx == NULL) {
		LOGERROR("ssock: Connection [%d %s@%d - %s@%d]: "
				"Failed starting thread (flush_tx).",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		goto ssock_add_peer_error_close_doflush_rx;
	}
	LOG("ssock: Connection [%d %s@%d - %s@%d]: thread (flush_tx) started.",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	return peer;

ssock_add_peer_error_close_doflush_rx:
	thread_close(peer->doflush_rx);

ssock_add_peer_error_close_docheck:
	thread_close(peer->docheck);

ssock_add_peer_error_free:
	free(peer);

	return NULL;
}

static int ssock_try_bind(sock_svc_t *svc)
{
	int port;
	struct sockaddr_in myaddr;
	unsigned int len;

	len = sizeof(myaddr);
	memset(&myaddr, 0, len);
	myaddr.sin_family      = AF_INET;
	myaddr.sin_addr.s_addr = svc->ip;

	if (svc->port != 0) {
		myaddr.sin_port = htons(svc->port);
		if (bind(svc->fd, (struct sockaddr *)&myaddr, len) < 0) {
			LOGERROR("socket [%d]: bind to port %d failed.",
					svc->fd, svc->port);
			return -1;
		}
		LOGINFO("socket [%d]: bound to user specified port %d.",
				svc->fd, svc->port);

		return 0;
	}

	for (port = MIN_SOCKET_PORT; port < MAX_SOCKET_PORT; port++) {
		myaddr.sin_port = htons(port);
		if (bind(svc->fd, (struct sockaddr *)&myaddr, len) < 0) {
			LOGDEBUG("socket [%d]: bind to port %d failed.", svc->fd, port);
			continue;
		}
		svc->port = port;
		LOGINFO("socket [%d]: bound to port %d.", svc->fd, svc->port);

		return 0;
	}
	LOGERROR("socket [%d]: no port available between [%d, %d)",
			svc->fd, MIN_SOCKET_PORT, MAX_SOCKET_PORT);

	return -1;
}

static sock_svc_t *_ssock_open(unsigned int ip, int port, int backlog,
		int (*running)(void), int (*pktsync)(void *buf, int size))
{
	sock_svc_t         *svc;
#ifdef WIN32
	u_long				iMode;
#else
	int                 flags;
#endif
	int                 re_useaddr = 1;

#ifdef WIN32
	if (win_wsa_startup()==0){
		return NULL;
	}
#endif

	if((svc = malloc(sizeof(sock_svc_t))) == NULL)
		return NULL;
	memset(svc, 0, sizeof(sock_svc_t));

	svc->fd      = INVALID_SOCKET;
	svc->ip      = ip;
	svc->port    = port;
	svc->backlog = backlog;
	svc->flag    = 0;
	SSOCK_LOCK_INIT(svc);
	INIT_LIST_HEAD(&(svc->peers));
	svc->check_running = running;
	svc->pktsync = pktsync;
	svc->conncb = NULL;
	svc->disccb = NULL;

	svc->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(svc->fd == INVALID_SOCKET) {
		LOGERROR("ssock_open: create socket fail.");
		goto sock_init_svc_free;
	}
	LOGDEBUG("ssock_open: socket %d created", svc->fd);

	if(setsockopt(svc->fd, SOL_SOCKET, SO_REUSEADDR,
			(void *)&re_useaddr, sizeof(int)) == -1) {
		LOGERROR("ssock_open: set socket %d reuseaddr fail.", svc->fd);
		goto sock_init_svc_close;
	}

	if (ssock_try_bind(svc) == -1) {
		LOGERROR("ssock_open: bind socket %d fail.", svc->fd);
		goto sock_init_svc_close;
	}

	if(listen(svc->fd, svc->backlog) == -1) {
		LOGERROR("ssock_open: listen socket %d fail.", svc->fd);
		goto sock_init_svc_close;
	}

#ifndef WIN32
	if((flags = fcntl(svc->fd, F_GETFL, 0)) < 0) {
		LOGERROR("ssock_open: get socket flags fail.");
		goto sock_init_svc_close;
	}
	flags |= O_NONBLOCK;
	if(fcntl(svc->fd, F_SETFL, flags) == -1) {
		LOGERROR("ssock_open: set socket %d to O_NONBLOCK fail.", svc->fd);
		goto sock_init_svc_close;
	}
#else
	iMode = 1;
	if (ioctlsocket(svc->fd, FIONBIO, &iMode) == SOCKET_ERROR) {
		LOGERROR("ssock_open: set socket %d to O_NONBLOCK fail.", svc->fd);
		goto sock_init_svc_close;
	}
#endif

	LOGDEBUG("ssock_open: created, %d, listening on %d", svc->fd, svc->port);

	return svc;

sock_init_svc_close:
	CLOSESOCKET(svc->fd);

sock_init_svc_free:
	SSOCK_LOCK_EXIT(svc);
	free(svc);

	return NULL;
}

static void ssock_accept(sock_svc_t *svc, int rbs, int tbs)
{
	struct sockaddr_in  client;
	socklen_t           len;
#ifndef WIN32
	int					nsock;
	int                 flags;
#else
	SOCKET				nsock;
	u_long				iMode;
#endif

	sock_peer *peer;
	sock_connection_t  *conn;

	len = sizeof(client);
	nsock = accept(svc->fd, (struct sockaddr *)&client, &len);
	if (nsock == -1) {
#ifndef WIN32
		if (errno == EAGAIN)
#else
		if (WSAGetLastError() == WSAEWOULDBLOCK)
#endif
			return;

		LOGDEBUG("ssock_accept: accept[%d] failed, ignore it", svc->fd);
		return;
	}

#ifndef WIN32
	if((flags = fcntl(nsock, F_GETFL, 0)) < 0) {
		LOGERROR("ssock_accept: get socket flags fail.");
		goto sock_accept_close;
	}
	flags |= O_NONBLOCK;
	if(fcntl(nsock, F_SETFL, flags) == -1) {
		LOGERROR("ssock_accept: set socket %d to O_NONBLOCK fail.", nsock);
		goto sock_accept_close;
	}
#else
	iMode = 1;
	if (ioctlsocket(nsock, FIONBIO, &iMode) == SOCKET_ERROR) {
		LOGERROR("ssock_accept: set socket %d to O_NONBLOCK fail.", nsock);
		goto sock_accept_close;
	}
#endif

	if ((conn = malloc(sizeof(sock_connection_t))) == NULL) {
		goto sock_accept_close;
	}
	memset(conn, 0, sizeof(sock_connection_t));

	conn->fd = nsock;
	strcpy(conn->ip, inet_ntoa(client.sin_addr));
	conn->port = ntohs(client.sin_port);

	conn->flag = svc->flag;
	conn->conncb = svc->conncb;
	conn->connarg = svc->connarg;
	conn->disccb = svc->disccb;
	conn->discarg = svc->discarg;
	conn->pktsync = svc->pktsync;
	/**/
	cssock_get_name(conn);

	if (sock_init_rbuf(conn, rbs, tbs) == -1) {
		goto sock_accept_buf;
	}

	if ((peer = ssock_add_peer(svc, conn)) == NULL) {
		goto sock_accept_rbuf;
	}
	LOG("Connection [%d %s@%d - %s@%d] accepted.",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	SSOCK_LOCK(svc);
	list_add(&(peer->list), &(svc->peers));
	SSOCK_UNLOCK(svc);

	if( (svc->conncb) && (conn) ){
		LOG("Connection [%d %s@%d - %s@%d]: (callback) connected.",
				conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);
		svc->conncb(conn->localipstr, conn->localport, conn->ip, conn->port, svc->connarg);
	}

	return;

sock_accept_rbuf:
	sock_exit_rbuf(conn);

sock_accept_buf:
	free(conn);

sock_accept_close:
	CLOSESOCKET(nsock);
}

/*before call this function, svc must be locked:SSOCK_LOCK(svc)
  and after the calling, unlock it.

  SSOCK_LOCK();
  ssock_get_conn();
  (do something)
  ....
  SSOCK_UNLOCK();
*/
extern void *ssock_get_conn(sock_svc_t *svc, char *ip, int port) {
	sock_peer         *peer;
	struct list_head  *pos, *next;

	list_for_each_safe(pos, next, &(svc->peers)) {
		peer = list_entry(pos, sock_peer, list);
		if (peer->conn) {
			if ((strcmp(peer->conn->ip, ip) == 0) && (peer->conn->port == port) ) {
				return peer->conn;
			}
		}
	}
	return NULL;
}

static void ssock_check(sock_svc_t *svc)
{
	sock_peer         *peer;
	struct list_head  *pos, *next;

	SSOCK_LOCK(svc);

	list_for_each_safe(pos, next, &(svc->peers)) {
		peer = list_entry(pos, sock_peer, list);
		if (peer->error) {
			list_del(pos);
			ssock_release_peer(peer);
		}
	}

	SSOCK_UNLOCK(svc);
}

/**/
void ssock_set_conncb(void *sh, CSSOCK_CONN_CB conncb, void *arg)
{
	sock_svc_t *svc = (sock_svc_t *)sh;

	if (svc) {
		svc->conncb = conncb;
		svc->connarg = arg;
	}
}

void ssock_set_disccb(void *sh, CSSOCK_DISC_CB disccb, void *arg)
{
	sock_svc_t *svc = (sock_svc_t*)sh;

	if (svc) {
		svc->disccb = disccb;
		svc->discarg = arg;
	}
}

/**/
/**/
void ssock_disconnect_peer(void *sh, char *ip, int port)
{
	sock_svc_t *svc;
	sock_peer         *peer;
	struct list_head  *pos, *next;

	if(!sh || !ip) {
		return;
	}
	svc = (sock_svc_t*)sh;

	SSOCK_LOCK(svc);

	list_for_each_safe(pos, next, &(svc->peers)) {
		peer = list_entry(pos, sock_peer, list);
		if( (peer) && (peer->conn) ) {
			if( (strcmp(ip,peer->conn->ip) == 0)  && port == peer->conn->port) {
				/**/
				list_del(pos);
				ssock_release_peer(peer);
			}
		}
	}

	SSOCK_UNLOCK(svc);
}

/**/

static void *ssock_do_accept(void *arg)
{
	struct thread_arg *targ = (struct thread_arg *)arg;
	sock_svc_t *svc = (sock_svc_t *)targ->arg;
	int i = 0;

	while (svc->check_running() && targ->flag) {
		SLEEP_MS(200);
		++i;

		ssock_accept(svc, svc->rbs, svc->tbs);

		if ((i & 0x000f) == 0) {
			ssock_check(svc);
		}
	}

	LOG("ssock: %d: thread (accept) terminated.", svc->port);
	return NULL;
}

static int ssock_open_thread(sock_svc_t *svc)
{
	svc->doaccept = thread_open(
			(LPTHREAD_START_ROUTINE)ssock_do_accept, (void *)svc);
	if (svc->doaccept == NULL) {
		LOGERROR("ssock: %d: Failed starting thread (accept).", svc->port);
		return -1;
	}
	LOG("ssock: %d: thread (accept) started.", svc->port);

	return 0;
}

void ssock_close(void *sh)
{
	sock_svc_t        *svc = (sock_svc_t *)sh;
	sock_peer         *peer;
	struct list_head  *pos, *next;

	if (!svc || (svc->fd == INVALID_SOCKET))
		return;

	if (svc->doaccept) {
		thread_close(svc->doaccept);
		LOG("ssock: %d: thread (accept) closed.", svc->port);
	}

	SSOCK_LOCK(svc);
	list_for_each_safe(pos, next, &(svc->peers)) {
		peer = list_entry(pos, sock_peer, list);
		list_del(pos);
		ssock_release_peer(peer);
	}
	SSOCK_UNLOCK(svc);

	SSOCK_LOCK_EXIT(svc);
	CLOSESOCKET(svc->fd);
	free(svc);

#ifdef WIN32
	win_wsa_cleanup();
#endif

	LOGDEBUG("ssock: closed");
}

void *ssock_open(int port, int backlog, int (*running)(void), int (*pktsync)(void *buf, int size))
{
	sock_svc_t *svc;

	svc = _ssock_open(htonl(INADDR_ANY), port, backlog, running, pktsync);
	if (svc) {
		if (ssock_open_thread(svc) < 0) {
			ssock_close(svc);
			svc = NULL;
		}
	}

	return svc;
}

void *ssock_open_f(unsigned int ip, int port, int backlog, int (*running)(void), int (*pktsync)(void *buf, int size))
{
	sock_svc_t *svc;

	svc = _ssock_open(ip, port, backlog, running, pktsync);
	if (svc) {
		if (ssock_open_thread(svc) < 0) {
			ssock_close(svc);
			svc = NULL;
		}
	}

	return svc;
}

int ssock_get_port(void *sh)
{
	sock_svc_t *svc = (sock_svc_t *)sh;

	return svc ? svc->port : -1;
}

void ssock_setflag(void *sh, unsigned int flag)
{
	sock_svc_t *svc = (sock_svc_t *)sh;

	if (svc)
		svc->flag |= flag;
}

void ssock_set_bufsize(void *sh, int rbs, int tbs)
{
	sock_svc_t *svc = (sock_svc_t *)sh;

	if (svc) {
		svc->rbs = rbs;
		svc->tbs = tbs;
	}
}

int ssock_peers(void *sh)
{
	sock_svc_t        *svc = (sock_svc_t *)sh;
	struct list_head  *pos;
	int count = 0;

	if(!svc || (svc->fd == INVALID_SOCKET))
		return count;

	SSOCK_LOCK(svc);
	list_for_each(pos, &(svc->peers)) {
		count++;
	}
	SSOCK_UNLOCK(svc);

	return count;
}

int ssock_write_to(void *sh, char *data, int len, char *ip, int port)
{
	sock_svc_t        *svc = (sock_svc_t *)sh;
	sock_peer         *peer;
	struct list_head  *pos;
	int bcast_cnt = 0;

	if(!svc || (svc->fd == INVALID_SOCKET))
		return -1;

	SSOCK_LOCK(svc);

	if (list_empty(&(svc->peers))) {
		SSOCK_UNLOCK(svc);
		return -1;
	}

	list_for_each(pos, &(svc->peers)) {
		peer = list_entry(pos, sock_peer, list);

		if (peer->error)
			continue;

		if (port && (port != peer->conn->port))
			continue;

		if (ip && strcmp(ip, peer->conn->ip))
			continue;

		LOGDEBUG("Connection [%d %s@%d - %s@%d]: writing %d bytes.",
				peer->conn->fd,
				peer->conn->localipstr, peer->conn->localport,
				peer->conn->ip, peer->conn->port, len);
		sock_write_to_buffer(peer->conn, data, len);
		bcast_cnt++;
	}

	SSOCK_UNLOCK(svc);

	if (port && ip && (bcast_cnt == 0))
		return -1;

	return len;
}

int ssock_broadcast(void *sh, char *data, int len)
{
	return ssock_write_to(sh, data, len, NULL, 0);
}

void *ssock_read_from(void *sh, char *ip, int *port)
{
	sock_svc_t        *svc = (sock_svc_t *)sh;
	sock_peer         *peer;
	struct list_head  *pos;
	void *ph;

	if (!svc || (svc->fd == INVALID_SOCKET))
		return 0;

	SSOCK_LOCK(svc);

	list_for_each(pos, &(svc->peers)) {
		peer = list_entry(pos, sock_peer, list);

		if ((ph = pkt_pop(peer->conn->rq)) != NULL) {
			if (ip)
				strcpy(ip, peer->conn->ip);
			if (port)
				*port = peer->conn->port;
			SSOCK_UNLOCK(svc);
			return ph;
		}
	}

	SSOCK_UNLOCK(svc);

	return NULL;
}

void *ssock_read(void *sh)
{
	return ssock_read_from(sh, NULL, NULL);
}

