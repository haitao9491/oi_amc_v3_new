/*
 *
 * csock.c - Implementation of client socket
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define NLGWR

#include "os.h"
#include "aplog.h"
#include "pkt.h"
#include "pktqueue.h"

#if defined(CSSOCK_TX_PFIFO)
#include "pfifo.h"
#endif

#define CSOCK_IMPLEMENTATION
#include "cssock.h"

static __inline int csock_create_connection(sock_connection_t *conn)
{
	struct sockaddr_in  svr;
	time_t              curr;
#ifdef WIN32
	u_long				iMode;
	int wsaerr;
#else
	int                 flags;
#endif

	time(&curr);
	if((curr - conn->prev) < 3) /* wait 3 seconds before retry */
		return -1;
	conn->prev = curr;

	svr.sin_family      = AF_INET;
	svr.sin_port        = htons(conn->port);
	svr.sin_addr.s_addr = inet_addr(conn->ip);

	conn->fd = socket(AF_INET, SOCK_STREAM, 0);
	if(conn->fd == -1) {
		LOGERROR("Connect to %s@%d, socket", conn->ip, conn->port);
		return -1;
	}
	LOGDEBUG("Socket[%d] -> %s@%d created", conn->fd, conn->ip, conn->port);

#ifndef WIN32
	if((flags = fcntl(conn->fd, F_GETFL, 0)) < 0) {
		LOGERROR("%d %s@%d: F_GETFL.", conn->fd, conn->ip, conn->port);
		close(conn->fd);	conn->fd = -1; return -1;
	}
	flags |= O_NONBLOCK;
	if(fcntl(conn->fd, F_SETFL, flags) == -1) {
		LOGERROR("%d %s@%d: F_SETFL.", conn->fd, conn->ip, conn->port);
		close(conn->fd);	conn->fd = -1; return -1;
	}
	LOGDEBUG("Socket[%d] -> %s@%d NONBLOCK mode",
			conn->fd, conn->ip, conn->port);
#else
	iMode = 1;
	if (ioctlsocket(conn->fd, FIONBIO, &iMode)==SOCKET_ERROR)
	{
		LOGERROR3("%d %s@%d: F_SETFL.", conn->fd, conn->ip, conn->port);
		closesocket(conn->fd);
		conn->fd = -1;
		return -1;
	}
#endif

	if(connect(conn->fd, (struct sockaddr *)&svr, sizeof(svr)) == -1) {
#ifndef WIN32
		if(errno == EINPROGRESS) {
#else
		wsaerr = WSAGetLastError();
		if (wsaerr == WSAEWOULDBLOCK) {
#endif
			cssock_get_name(conn);
			conn->inprogress = 1;
			LOGDEBUG("Connection [%d %s@%d - %s@%d] in progress ...",
					conn->fd,
					conn->localipstr, conn->localport, conn->ip, conn->port);
			return(0);
		}
		LOGERROR("Socket[%d] -> %s@%d unable to connect to server.",
				conn->fd, conn->ip, conn->port);
		CLOSESOCKET(conn->fd);
		conn->fd = -1;
		return -1;
	}

	cssock_get_name(conn);
	conn->inprogress = 0;
	LOG("Connection [%d %s@%d - %s@%d] established immediately.",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	if (conn->conncb) {
		LOG("Connection [%d %s@%d - %s@%d]: (callback) connected.",
				conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);
		conn->conncb(conn->localipstr, conn->localport, conn->ip, conn->port, conn->connarg);
	}

	return 0;
}

static __inline int csock_check_if_established(sock_connection_t *conn)
{
	struct timeval  timeout;
	fd_set          writefd;
	int             rc;
	int             so_error = 0;
	socklen_t       socklen = (socklen_t)sizeof(so_error);

	FD_ZERO(&writefd);
	FD_SET(conn->fd, &writefd);
	timeout.tv_sec  = 0;
	timeout.tv_usec = 1000;	/* 1 milisecond */

	rc = select((int)(conn->fd + 1), NULL, &writefd, NULL, &timeout);
	if(rc == 0) {
		return(0);
	}
	if(rc == -1) {
		LOGERROR("Connection [%d %s@%d - %s@%d] select failed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		CLOSESOCKET(conn->fd);
		conn->fd = -1;
		return -1;
	}

	/* socket is now writable */
	so_error = 0;
#ifndef WIN32
	rc = getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, &so_error, &socklen);
#else
	rc = getsockopt(conn->fd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &socklen);
#endif
	if((rc != 0) || (so_error != 0)) {
		LOGDEBUG("Connection [%d %s@%d - %s@%d] getsockopt %d, so_error %d.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port, rc, so_error);
		CLOSESOCKET(conn->fd);
		conn->fd = -1;
		return -1;
	}

	conn->inprogress = 0;
	LOG("Connection [%d %s@%d - %s@%d] established (deferred).",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);

	if (conn->conncb) {
		LOG("Connection [%d %s@%d - %s@%d]: (callback) connected.",
				conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port);
		conn->conncb(conn->localipstr, conn->localport, conn->ip, conn->port, conn->connarg);
	}

	return(1);
}

static __inline void csock_close_socket(sock_connection_t *conn, char *cause)
{
	cssock_conn_stat(conn);

	if(conn->fd != -1) {
		if (conn->disccb) {
			LOG("Connection [%d %s@%d - %s@%d]: (callback) disconnected: %s",
					conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port, 
					cause ? cause : "");
			conn->disccb(conn->localipstr, conn->localport, conn->ip, conn->port, conn->discarg);
		}

		CLOSESOCKET(conn->fd);
		conn->fd = -1;
	}
	conn->inprogress = 0;

	memset(conn->localipstr, 0, sizeof(conn->localipstr));
	conn->localip = 0;
	conn->localport = 0;

	conn->rsockbytes = 0;
	conn->rlofbytes = 0;
	conn->rqueue = 0;
	conn->rqueuebytes = 0;
	conn->rqueuefull = 0;
	conn->rqueuefullbytes = 0;
	conn->rqueueerr = 0;
	conn->rqueueerrbytes = 0;

	conn->t = 0;
	conn->tbytes = 0;
	conn->tfull = 0;
	conn->tfullbytes = 0;
	conn->terr = 0;
	conn->terrbytes = 0;
	conn->tsock = 0;
	conn->tsockbytes = 0;
	conn->tsockrealbytes = 0;

	conn->rxalignlen = 0;
	conn->prev = (time_t )0;

	conn->txtmpbuflen = 0;
}


void *csock_open(const char *ip, int port, int rbs, int tbs,
		int (*pktsync)(void *buf, int size))
{
	sock_connection_t *conn;
#ifdef WIN32
	if (win_wsa_startup()==0){
		return NULL;
	}
#endif

	if(!ip || (strlen(ip) > 15)) {
		LOGERROR("csock_open: Invalid IP.");
		return NULL;
	}

	if((conn = malloc(sizeof(sock_connection_t))) == NULL) {
		LOGERROR("csock_open: %s@%d: Insufficient memory.", ip, port);
		return NULL;
	}

	/* initialize the connection */
	memset(conn, 0, sizeof(sock_connection_t));
	conn->fd = -1;
	strcpy(conn->ip, ip);
	conn->port = port;
	conn->pktsync = pktsync;

	if(sock_init_rbuf(conn, rbs, tbs) == -1) {
		LOGERROR("csock_open: %s@%d: Init Rx/Tx buffer failed.",
				ip, port);
		free(conn);
		return NULL;
	}

	CSOCK_LOCK_INIT(conn);

	LOGDEBUG("csock_open: %s@%d: Created.", ip, port);

	return conn;
}

int csock_setflag(void *sh, unsigned int flag)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	CSOCK_LOCK(conn);
	conn->flag |= flag;
	CSOCK_UNLOCK(conn);

	return 0;
}

void csock_close(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	sock_exit_rbuf(conn);
	csock_close_socket(conn, "User");
	CSOCK_LOCK_EXIT(conn);

#ifdef WIN32
	win_wsa_cleanup();
#endif

	free(sh);
}

void csock_close_socket_lock(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	CSOCK_LOCK(conn);
	csock_close_socket(conn, "User");
	CSOCK_UNLOCK(conn);
}

char *csock_get_localipstr(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	return conn ? conn->localipstr : NULL;
}

unsigned int csock_get_localip(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	return conn ? conn->localip : 0;
}

int csock_get_localport(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	return conn ? conn->localport : 0;
}


int csock_is_connected(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	if(!conn || (conn->fd == -1) || conn->inprogress)
		return 0;

	return 1;
}


int csock_check_connection(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;
	int      rc;

	if(!conn)
		return -1;

	CSOCK_LOCK(conn);

	/* Client socket hasn't been created yet */
	if(conn->fd == -1) {
		if(csock_create_connection(conn) < 0) {
			CSOCK_UNLOCK(conn);
			return -1;
		}
	}

	/* Client socket has been created, but does the connection to
	 * server established?
	 */
	if(!conn->inprogress) { /* established */
		CSOCK_UNLOCK(conn);
		return 1;
	}

	rc = csock_check_if_established(conn);
	CSOCK_UNLOCK(conn);

	return rc;
}


void *csock_read(void *sh)
{
	return pkt_pop(((sock_connection_t *)sh)->rq);
}


int csock_write(void *sh, char *data, int len)
{
	return sock_write_to_buffer((sock_connection_t *)sh, data, len);
}

int csock_flush_tx(void *sh)
{
	int   rc, ret = 0;
	char *ph;
	sock_connection_t *conn = (sock_connection_t *)sh;

	CSOCK_LOCK(conn);
	if ((conn->fd == -1) || conn->inprogress) {
		CSOCK_UNLOCK(conn);
		return 0;
	}

	while (conn->tq) {
		if (conn->txtmpbuflen > 0) {
			rc = sock_write_buf_socket(conn, conn->txtmpbuf, conn->txtmpbuflen);
			if (rc < 0) {
				/* ERROR occurred on this connection! */
				csock_close_socket(conn, "Socket error (writing)");
				CSOCK_UNLOCK(conn);
				return -1;
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

			break;
		}

#if !defined(CSSOCK_TX_PFIFO)
		ph = pkt_pop(conn->tq);
		if (!ph)
			break;

		ret = 1;

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

		if (conn->txbuflen == sizeof(pkt_hdr))
			break;

		pkthdr_set_sync((pkt_hdr *)conn->txbuf);
		pkthdr_set_plen((pkt_hdr *)conn->txbuf, conn->txbuflen);

		ret = 1;

		rc = sock_write_pkt_socket(conn, conn->txbuf);
#endif

		if (rc < 0) {
			/* ERROR occurred on this connection! */
			csock_close_socket(conn, "Socket error (writing)");
			CSOCK_UNLOCK(conn);
			return -1;
		}
	}

	CSOCK_UNLOCK(conn);
	return ret;
}


int csock_flush_rx(void *sh)
{
	sock_connection_t *conn = (sock_connection_t *)sh;
	int      rc, ret = 0;

	CSOCK_LOCK(conn);
	if((conn->fd == -1) || conn->inprogress) {
		CSOCK_UNLOCK(conn);
		return 0;
	}

	while(conn->rq) {
		rc = sock_read_from_socket(conn);
		if(rc > 0) {
			ret = 1;

			if(rc < 1400) {
				/* An ethernet frame length is 64 - 1518 bytes,
				 * including MAC, ip, tcp headers and FCS. In
				 * our implementation the maximum length of the
				 * payload is 1460 bytes. So, if we didn't
				 * get 1460 bytes of data, we are not in a
				 * hurry and can break out for a little rest.
				 */
				/* 1460 -> 1400 */
				break;
			}

			continue;
		}

		if(rc == 0) {
			break;
		}

		if(rc == -1) {
			/* ERROR occurred on this connection! */
			csock_close_socket(conn, "Socket error (reading)");
			CSOCK_UNLOCK(conn);
			return -1;
		}
	}
	CSOCK_UNLOCK(conn);

	return ret;
}

void csock_stat(void *sh)
{
	cssock_conn_stat((sock_connection_t *)sh);
}

void csock_setconncb(void *sh, CSSOCK_CONN_CB conncb, void *arg)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	if (conn) {
		conn->conncb = conncb;
		conn->connarg = arg;
	}
}

void csock_setdisccb(void *sh, CSSOCK_DISC_CB disccb, void *arg)
{
	sock_connection_t *conn = (sock_connection_t *)sh;

	if (conn) {
		conn->disccb = disccb;
		conn->discarg = arg;
	}
}

