/*
 *
 * cssock.h - Public definitions and implementations of server and client
 *            side sockets. Users don't need to include this header file.
 */

#ifndef _HEAD_CSSOCK_61DE4FB4_2063EC7F_72CEFCEE_H
#define _HEAD_CSSOCK_61DE4FB4_2063EC7F_72CEFCEE_H

#ifndef WIN32
#include <sys/time.h>
#else
#pragma warning ( disable : 4996 )
#include <time.h>
#include <winsock2.h>
#include "misc.h"
typedef unsigned int socklen_t;
#endif
#include "os.h"
#if defined(SSOCK_SYNCED) || defined(CSOCK_SYNCED)
#include "mutex.h"
#endif
#include "cssockdef.h"

#if defined(SSOCK_SYNCED)
#define SSOCK_LOCK_INIT(ds) (ds)->lock = mutex_open(NULL)
#define SSOCK_LOCK(ds)      mutex_lock((ds)->lock)
#define SSOCK_UNLOCK(ds)    mutex_unlock((ds)->lock)
#define SSOCK_LOCK_EXIT(ds) mutex_close((ds)->lock)
#else
#define SSOCK_LOCK_INIT(ds) do { } while (0)
#define SSOCK_LOCK(ds)      do { } while (0)
#define SSOCK_UNLOCK(ds)    do { } while (0)
#define SSOCK_LOCK_EXIT(ds) do { } while (0)
#endif

#if defined(CSOCK_SYNCED)
#define CSOCK_LOCK_INIT(ds) (ds)->lock = mutex_open(NULL)
#define CSOCK_LOCK(ds)      mutex_lock((ds)->lock)
#define CSOCK_UNLOCK(ds)    mutex_unlock((ds)->lock)
#define CSOCK_LOCK_EXIT(ds) mutex_close((ds)->lock)
#else
#define CSOCK_LOCK_INIT(ds) do { } while (0)
#define CSOCK_LOCK(ds)      do { } while (0)
#define CSOCK_UNLOCK(ds)    do { } while (0)
#define CSOCK_LOCK_EXIT(ds) do { } while (0)
#endif

typedef struct {
#if defined(CSOCK_IMPLEMENTATION) && defined(CSOCK_SYNCED)
	/* lock - protect the current status: fd, inprogress */
	void *lock;
#endif

	/* connection */
	SOCKET fd;
	int   inprogress;
	char  ip[16];
	int   port;

	char localipstr[16];
	unsigned int localip;
	int localport;

	unsigned int flag;

	/* rbuf */
	void *rq, *tq;

	/* statistics */
	unsigned long long rsockbytes;
	unsigned long long rlofbytes;
	unsigned long long rqueue;
	unsigned long long rqueuebytes;
	unsigned long long rqueuefull;
	unsigned long long rqueuefullbytes;
	unsigned long long rqueueerr;
	unsigned long long rqueueerrbytes;

	unsigned long long t;
	unsigned long long tbytes;
	unsigned long long tfull;
	unsigned long long tfullbytes;
	unsigned long long terr;
	unsigned long long terrbytes;
	unsigned long long tsock;
	unsigned long long tsockbytes;
	unsigned long long tsockrealbytes;

	/* internal */
	char rxalignbuf[CSSOCK_RXALIGN_BUFSZ];
	int  rxalignlen;
	char rxtmpbuf[CSSOCK_RXALIGN_BUFSZ];
	int  rxtmpbuflen;
	time_t  prev;

	int (*locate_customer_pkt)(char *inbuf, int *inlen, char *outbuf, int *outlen);	
	char txtmpbuf[CSSOCK_TXTMPBUF_SIZE];
	int  txtmpbuflen;
#if defined(CSSOCK_TX_PFIFO)
	char txbuf[CSSOCK_TXBUF_SIZE];
	int  txbuflen;
	void *wlock;
#endif

	CSSOCK_CONN_CB conncb;
	void	      *connarg;
	CSSOCK_DISC_CB disccb;
	void	      *discarg;
	int (*pktsync)(void *buf, int size);
} sock_connection_t;

#ifdef WIN32
static __inline int win_wsa_cleanup()
{
	if (WSACleanup() != 0)
	{
		LOGERROR("WinSock2 WSA cleanup failed. Error=%d", WSAGetLastError());
		return 0;
	}
	LOGDEBUG("Winsock2 WSA cleanup succeed.");
	return 1;
}

static __inline int win_wsa_startup()
{
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD( 2, 2 );

	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		LOGERROR("WinSock2 WSA startup failed. Error=%d", WSAGetLastError());
		return 0;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions later    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */

	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		LOGERROR("WinSock2 WSA startup failed. Winsock Version mismatch.");
		win_wsa_cleanup();
		return 0;
	}
	LOGDEBUG("Winsock2 WSA startup succeed.");
	return 1;
}

#endif

/** Return the number of bytes sent or -1 if an error occurred.
 */
static __inline int sock_write_buf_socket(sock_connection_t *conn,
		char *data, int len)
{
	int rc;

	rc = send(conn->fd, data, len, 0);
	if (rc > 0) {
		LOGDEBUG("Sending %d bytes to [%d %s@%d - %s@%d]: %d sent.",
				len, conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port, rc);
	}
	else {
		if ((rc == 0) || 
#ifndef WIN32
				(errno == EAGAIN)
#else
				(WSAGetLastError() == WSAEWOULDBLOCK)
#endif
		   )
		{
			LOGDEBUG("Sending %d bytes to [%d %s@%d - %s@%d]: blocked.",
					len, conn->fd, conn->localipstr, conn->localport,
					conn->ip, conn->port);
			rc = 0;
		}
		else {
			LOGERROR("Sending %d bytes to [%d %s@%d - %s@%d]: error occurred.",
					len, conn->fd, conn->localipstr, conn->localport,
					conn->ip, conn->port);
			rc = -1;
		}
	}

	return rc;
}

static __inline int sock_write_pkt_socket(sock_connection_t *conn, char *data)
{
	int sent, rlen, len;

#if !defined(CSSOCK_TX_PFIFO)
	rlen = len = pkthdr_get_plen((pkt_hdr *)data);

	if (conn->flag & CSSOCK_CUSTOMIZED_PKTSYNC) {
		data += sizeof(pkt_hdr);
		rlen -= sizeof(pkt_hdr);
	}
	else if (conn->flag & CSSOCK_PAYLOAD_ONLY) {
		data += sizeof(pkt_hdr);
		rlen -= sizeof(pkt_hdr);
	}
#else
	rlen = len = pkthdr_get_dlen((pkt_hdr *)data);
	data += sizeof(pkt_hdr);
#endif

	conn->tsock++;
	conn->tsockbytes += len;
	conn->tsockrealbytes += rlen;

	sent = sock_write_buf_socket(conn, data, rlen);
	if (sent < 0)
		return -1;

	if (sent < rlen) {
		int remain = rlen - sent;

		if (CSSOCK_TXTMPBUF_SIZE > remain) {
			memcpy(conn->txtmpbuf, data + sent, remain);
			conn->txtmpbuflen = remain;
			LOGDEBUG("Sending %d bytes to [%d %s@%d - %s@%d]: %d buffered.",
					rlen, conn->fd, conn->localipstr, conn->localport,
					conn->ip, conn->port, remain);
		}
		else {
			LOGERROR("Sending %d bytes to [%d %s@%d - %s@%d]: %d discarded.",
					rlen, conn->fd, conn->localipstr, conn->localport,
					conn->ip, conn->port, remain);
		}
	}

	return 0;
}

static __inline int sock_write_to_buffer(sock_connection_t *conn, char *data, int len)
{
#if !defined(CSSOCK_TX_PFIFO)
	int rc = pkt_push(conn->tq, data, len);
#else
	void *p;
	int rc;

	mutex_lock(conn->wlock);
	p = pfifo_write(conn->tq, len);
	if (p) {
		memcpy(p, data, len);
		pfifo_write_commit(conn->tq);
		rc = len;
	}
	else {
		rc = 0;
	}
	mutex_unlock(conn->wlock);
#endif

	if(rc == len) {
		conn->t++;
		conn->tbytes += len;
		LOGDEBUG("sock_write [%d %s@%d - %s@%d]: %d bytes buffered.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port, rc);
		return len;
	}

	if(rc == 0) {
		conn->tfull++;
		conn->tfullbytes += len;
		LOGDEBUG("sock_write [%d %s@%d - %s@%d]: %d bytes discarded, full.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port, len);
		return 0;
	}

	conn->terr++;
	conn->terrbytes += len;
	LOGDEBUG("sock_write [%d %s@%d - %s@%d]: %d bytes discarded, fatal error.",
			conn->fd, conn->localipstr, conn->localport,
			conn->ip, conn->port, len);
	return -1;
}

static __inline void sock_read_from_socket_statistics(sock_connection_t *conn,
		int rc, int len)
{
	if (rc > 0) {
		conn->rqueue++;
		conn->rqueuebytes += len;
	}
	else if (rc == 0) {
		conn->rqueuefull++;
		conn->rqueuefullbytes += len;
		LOGDEBUG("sock_read: %d bytes from [%d %s@%d - %s@%d] discarded "
				"(pktq full).",
				len, conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);
	}
	else {
		conn->rqueueerr++;
		conn->rqueueerrbytes += len;
		LOGDEBUG("sock_read: %d bytes from [%d %s@%d - %s@%d] discarded "
				"(pktq error).",
				len, conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);
	}
}

/* This function returns the number of bytes we received directly from
 * the socket. In another word, it returns the result of recv.
 */
static __inline int sock_read_from_socket(sock_connection_t *conn)
{
	int i, rc, len, plen, dlen;
	char *p;

	rc = recv(conn->fd, conn->rxalignbuf + conn->rxalignlen,
			CSSOCK_RXALIGN_BUFSZ - conn->rxalignlen, 0);
	len = conn->rxalignlen + rc;

	if (rc > 0) {
		conn->rsockbytes += rc;
		LOGDEBUG("sock_read: %d bytes received from [%d %s@%d - %s@%d].",
				rc, conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);

		/* If CSSOCK_CUSTOMIZED_PKTSYNC flag is set, we prefer delimitation using pktsync(),
		 * if null pktsync, then we consider the received bytes a whole packet.
		 */
		if (conn->flag & (CSSOCK_CUSTOMIZED_PKTSYNC | CSSOCK_PAYLOAD_ONLY)) {
			struct timeval tv;

			gettimeofday(&tv, NULL);
			if(conn->pktsync) {
				p = conn->rxalignbuf;

				while (1) {
					plen = conn->pktsync(p, len);
					if (plen > 0) {
						i = pkt_push_nohdr(conn->rq, p, plen, tv.tv_sec, tv.tv_usec * 1000);
						sock_read_from_socket_statistics(conn, i, plen);
						
						p   += plen;
						len -= plen;

						continue;
					}
					else if (plen < 0) {
						LOGERROR("sock_read: receive from [%d %s@%d - %s@%d] failed,invalid data format.",
							conn->fd, conn->localipstr, conn->localport,
							conn->ip, conn->port);
							return(-1);
					}
					else {
						conn->rxalignlen = len;

						if ((p != conn->rxalignbuf) && (conn->rxalignlen > 0)) {
							memcpy(conn->rxalignbuf, p, conn->rxalignlen);
						}

						break;
					}
				}
			}
			else {
				i = pkt_push_nohdr(conn->rq, conn->rxalignbuf, len,
					tv.tv_sec, tv.tv_usec * 1000);
				sock_read_from_socket_statistics(conn, i, len);

				conn->rxalignlen = 0;
			}

			return rc;
		}

		/* Try to locate a complete packet and push it into the
		 * RxPKTQ. Buf if it meets the criteria of Loss of Packet,
		 * we have to start a resync procedure.
		 */
		for (i = 0, p = conn->rxalignbuf, dlen = len; i < len; ) {
			plen = pkt_verify_slow(p, dlen);
			if (plen > 0) {
				if (pkthdr_slow_get_type((pkt_hdr *)p) != PKT_TYPE_HEARTBEAT) {
					sock_read_from_socket_statistics(conn,
							pkt_push(conn->rq, p, plen), plen);
				}
				else {
					LOGDEBUG("got heartbeat packet from [%d %s@%d - %s@%d].",
							conn->fd, conn->localipstr, conn->localport,
							conn->ip, conn->port);
				}

				i += plen;
				p += plen;
				dlen -= plen;

				continue;
			}

			if ((plen < 0) || (dlen > (CSSOCK_RXALIGN_BUFSZ / 2))) {
				conn->rlofbytes++;
				i++;
				p++;
				dlen--;
				continue;
			}

			/* We should wait for more data to decide whether
			 * we are really out of frame sync.
			 */
			conn->rxalignlen = dlen;

			if ((i > 0) && (conn->rxalignlen > 0)) {
				memcpy(conn->rxalignbuf,
						conn->rxalignbuf + i,
						conn->rxalignlen);
			}

			LOGDEBUG("sock_read: %d bytes directly read from [%d %s@%d - %s@%d]"
					", %d remains in align buffer.",
					rc, conn->fd, conn->localipstr, conn->localport,
					conn->ip, conn->port, dlen);

			return rc;
		}

		/* Should never run to here? NO!
		 *   Case 1: A complete packet ends at the end of rxalignbuf.
		 */
		conn->rxalignlen = 0;

		return rc;
	}

	if(rc == -1) {
#ifndef WIN32
		if(errno == EAGAIN) {
#else
		if(WSAGetLastError() == WSAEWOULDBLOCK) {
#endif
			// no messages are available and O_NONBLOCK is set.
			return(0);
		}

		LOGERROR("sock_read: receive from [%d %s@%d - %s@%d] failed.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);
		return(-1);
	}

#if 0
	LOGERROR("recv returns zero, this is actualy an error, %d %s@%d.",
			conn->fd, conn->ip, conn->port);
#endif
	return -1;
}

static __inline int sock_init_rbuf(sock_connection_t *conn, int rbs, int tbs)
{
	if ((rbs <= 1024) || (tbs <= 1024))
		return -1;

	conn->rq = pkt_queue_open(PKTQUEUE_MALLOC_BUFFER, "Sock.RxPKTQ");
	if (conn->rq == NULL) {
		LOGERROR("Connection [%d %s@%d - %s@%d]: Creating RxPKTQ failed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		return -1;
	}
	pkt_queue_set_sizelimit(conn->rq, rbs);
	LOGINFO("Connection [%d %s@%d - %s@%d]: RxPKTQ (Size limit: %d) created.",
			conn->fd,
			conn->localipstr, conn->localport, conn->ip, conn->port, rbs);

#if !defined(CSSOCK_TX_PFIFO)
	conn->tq = pkt_queue_open(PKTQUEUE_MALLOC_BUFFER, "Sock.TxPKTQ");
	if (conn->tq == NULL) {
		LOGERROR("Connection [%d %s@%d - %s@%d]: Creating TxPKTQ failed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		pkt_queue_close(conn->rq);
		return -1;
	}
	pkt_queue_set_sizelimit(conn->tq, tbs);
	LOGINFO("Connection [%d %s@%d - %s@%d]: TxPKTQ (Size limit: %d) created.",
			conn->fd,
			conn->localipstr, conn->localport, conn->ip, conn->port, tbs);
#else
	int num_entries = (tbs + (CSSOCK_TX_ENTRYSZ - 1)) / CSSOCK_TX_ENTRYSZ;

	conn->tq = pfifo_open(num_entries, "Sock.TxPFIFO", CSSOCK_TX_ENTRYSZ, NULL, NULL);
	if (conn->tq == NULL) {
		LOGERROR("Connection [%d %s@%d - %s@%d]: Creating TxPFIFO failed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		pkt_queue_close(conn->rq);
		return -1;
	}
	LOGINFO("Connection [%d %s@%d - %s@%d]: TxPFIFO (Size limit: %d) created.",
			conn->fd,
			conn->localipstr, conn->localport, conn->ip, conn->port, tbs);

	conn->wlock = mutex_open(NULL);
	if (conn->wlock == NULL) {
		LOGERROR("Connection [%d %s@%d - %s@%d]: Open write lock failed.",
				conn->fd,
				conn->localipstr, conn->localport, conn->ip, conn->port);
		pkt_queue_close(conn->rq);
		pfifo_close(conn->tq);
		return -1;
	}
#endif

	return 0;
}

static __inline void sock_exit_rbuf(sock_connection_t *conn)
{
	if (conn->rq) {
		pkt_queue_close(conn->rq);
		LOGINFO("Connection [%d %s@%d - %s@%d]: RxPKTQ released.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);
	}
#if !defined(CSSOCK_TX_PFIFO)
	if (conn->tq) {
		pkt_queue_close(conn->tq);
		LOGINFO("Connection [%d %s@%d - %s@%d]: TxPKTQ released.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);
	}
#else
	if (conn->wlock) {
		mutex_close(conn->wlock);
	}
	if (conn->tq) {
		LOG("Connection [%d %s@%d - %s@%d]: TxPFIFO: %d cleared.", 
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port,
				pfifo_get_cnt(conn->tq));
		pfifo_close(conn->tq);
		LOGINFO("Connection [%d %s@%d - %s@%d]: TxPFIFO released.",
				conn->fd, conn->localipstr, conn->localport,
				conn->ip, conn->port);
	}
#endif

	conn->rq = NULL;
	conn->tq = NULL;
}

static int cssock_get_name(sock_connection_t *conn)
{
	struct sockaddr_in sa;
	socklen_t salen;

	if (!conn || (conn->fd == -1))
		return -1;

	salen = (socklen_t)sizeof(sa);
	if (getsockname(conn->fd, (struct sockaddr *)(&sa), &salen) == -1)
		return -1;

	sprintf(conn->localipstr, "%s", inet_ntoa(sa.sin_addr));
	conn->localip = (unsigned int)sa.sin_addr.s_addr;
	conn->localport = (int)ntohs(sa.sin_port);
	LOGDEBUG("Connection [%d %s@%d - %s@%d]",
			conn->fd, conn->localipstr, conn->localport,
			conn->ip, conn->port);

	return 0;
}

static int cssock_conn_stat(sock_connection_t *conn)
{
	if (conn->inprogress || ((conn->rsockbytes == 0) && (conn->tbytes == 0)))
		return -1;

	LOG("Connection [%d %s@%d - %s@%d]: Rx -> Socket(%lluB, LOF %lluB) -> "
			"Buffer([%lluP, %lluB], F[%lluP, %lluB], E[%lluP, %lluB])",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port,
			conn->rsockbytes, conn->rlofbytes,
			conn->rqueue, conn->rqueuebytes,
			conn->rqueuefull, conn->rqueuefullbytes,
			conn->rqueueerr, conn->rqueueerrbytes);
	LOG("Connection [%d %s@%d - %s@%d]: "
			"Tx -> Buffer([%lluP, %lluB], F[%lluP, %lluB], E[%lluP, %lluB]) -> "
			"Socket(%lluP, %lluB, %lluB)",
			conn->fd, conn->localipstr, conn->localport, conn->ip, conn->port,
			conn->t, conn->tbytes,
			conn->tfull, conn->tfullbytes,
			conn->terr, conn->terrbytes,
			conn->tsock, conn->tsockbytes, conn->tsockrealbytes);

	return 0;
}

#endif /* #ifndef _HEAD_CSSOCK_61DE4FB4_2063EC7F_72CEFCEE_H */
