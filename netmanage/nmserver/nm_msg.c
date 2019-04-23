/*
 *
 * nm_msg.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <pthread.h>
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "mutex.h"
#include "nm_typdef.h"
#include "pkt.h"
#include "nmpkt.h"
#include "nm_msg.h"

static pthread_mutex_t seq_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned short seq = 0;

void seq_lst_get_seqno(unsigned short *sn) 
{
	pthread_mutex_lock(&seq_mutex);

	if (sn)
		*sn = seq;
	seq++;

	pthread_mutex_unlock(&seq_mutex);
}

void *nmmsg_cmd_glb_info(void)
{
	pkt_hdr *ph = NULL;
	nmpkt_hdr *nmph = NULL;
	unsigned short dlen = 0;
	unsigned short seq = 0;
	int len = 0;

	len = PKT_LEN + NMPKT_LEN;
	ph = (pkt_hdr *)malloc(len);
	if (!ph) {
		LOGERROR("%s: malloc is failed", __func__);
		return NULL;
	}

	memset(ph, 0, len);
	nmph = (nmpkt_hdr *)pkthdr_get_data(ph);
	seq_lst_get_seqno(&seq);
	nmpkthdr_construct_hdr(nmph, NM_MODULE_GLB_INFO, NM_CMD_GET_BOARD_INFO, seq, dlen);
	pkthdr_set_sync(ph);
	pkthdr_set_plen(ph, (dlen + PKT_LEN + NMPKT_LEN));

	LGWRDEBUG(ph, len, "CMD:");
	return ph;
}

void *nmmsg_cmd_set_sw_an(void *data)
{
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	pkt_hdr *oph = NULL;
	unsigned char *p = NULL, *q = NULL;
	int dlen = 0;
	int len = 0;

	if (!data)
		return NULL;

	dlen = 4;  /* id+type+port+an */
	len  = PKT_LEN + NMPKT_LEN + dlen;

	oph = (pkt_hdr *)malloc(len);
	if (!oph) {
		LOGERROR("%s: malloc failed", __func__);
		return NULL;
	}
	memset(oph, 0, len);

	p = (unsigned char *)pkthdr_get_data(oph);
	memcpy(p, nmph, sizeof(*nmph));

	p += NMPKT_LEN;
	q = (unsigned char *)(p + 5);  /* sizeof(lki) = 5 */
	memcpy(p, q, dlen);

	nmph = (nmpkt_hdr *)pkthdr_get_data(oph);
	nmpkthdr_set_dlen(nmph, dlen);
	
	pkthdr_set_sync(oph);
	pkthdr_set_plen(oph, (dlen + NMPKT_LEN + PKT_LEN));

	return oph;
}

void *nmmsg_cmd_set_sw_pwrdn(void *data)
{
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	pkt_hdr *oph = NULL;
	unsigned char *p = NULL, *q = NULL;
	int dlen = 0;
	int len = 0;

	if (!data)
		return NULL;

	dlen = 4;  /* id+type+port+an */
	len  = PKT_LEN + NMPKT_LEN + dlen;

	oph = (pkt_hdr *)malloc(len);
	if (!oph) {
		LOGERROR("%s: malloc failed", __func__);
		return NULL;
	}
	memset(oph, 0, len);

	p = (unsigned char *)pkthdr_get_data(oph);
	memcpy(p, nmph, sizeof(*nmph));

	p += NMPKT_LEN;
	q = (unsigned char *)(p + 5);  /* sizeof(lki) = 5 */
	memcpy(p, q, dlen);

	nmph = (nmpkt_hdr *)pkthdr_get_data(oph);
	nmpkthdr_set_dlen(nmph, dlen);
	
	pkthdr_set_sync(oph);
	pkthdr_set_plen(oph, (dlen + NMPKT_LEN + PKT_LEN));

	return oph;
}

