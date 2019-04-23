/*
 *
 * nm_e1phy.c - A brief description goes here.
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
#include "os.h"
#include "aplog.h"
#include "mutex.h"
#include "pkt.h"
#include "nmpkt.h"
#include "nm_typdef.h"
#include "phyadapt.h"
#include "nm_e1phy.h"

typedef struct {
	int phycnt;   /* e1 phy count */
	int pcnt;     /* port count of each phy */
	unsigned char *data;  /* nm_port_alarm*phycnt*pcnt + nm_port_stat*phycnt*pcnt */
} nm_e1phy_component;

void *nm_e1phy_open(void)
{
	nm_e1phy_component *e1hd = NULL;

	e1hd = malloc(sizeof(*e1hd));
	if (!e1hd) {
		LOGERROR("%s: malloc failed", __func__);
		return NULL;
	}

	memset(e1hd, 0, sizeof(*e1hd));
	
	return e1hd;
}

int nm_e1phy_add_data(void *hd, int phycnt, int pcnt, void *data)
{
	nm_e1phy_component *e1hd = (nm_e1phy_component *)hd;
	unsigned char *p = (unsigned char *)data;
	int len = 0;

	if (!e1hd || !data)
		return -1;

	if (e1hd->data != NULL) {
		if (e1hd->phycnt == phycnt && e1hd->pcnt == pcnt) {
			len = phycnt * pcnt * (sizeof(nm_port_alarm) + sizeof(nm_port_stat));
			memcpy(e1hd->data, p, len);
		}
		else {
			LOGERROR("e1phy_add_data: phycnt%d:%d, pcnt%d:%d is not equal, release e1hd->data", 
					e1hd->phycnt, phycnt, e1hd->pcnt, pcnt);
			free(e1hd->data);
			e1hd->data = NULL;
		}
	}
	else {
		e1hd->phycnt  = phycnt;
		e1hd->pcnt    = pcnt;
		len = phycnt * pcnt * (sizeof(nm_port_alarm) + sizeof(nm_port_stat));
		e1hd->data = (unsigned char *)malloc(len);
		if (e1hd->data != NULL)
			memcpy(e1hd->data, p, len);
	}

	return 0;
}

void *nm_e1phy_get_data(void *hd, int *phycnt, int *pcnt) 
{
	nm_e1phy_component *e1hd = (nm_e1phy_component *)hd;

	if (!e1hd || !phycnt || !pcnt)
		return NULL;

	*phycnt = e1hd->phycnt;
	*pcnt   = e1hd->pcnt;

	return e1hd->data;
}

void *nm_e1phy_get_data_pkt(void *hd, struct nm_lkaddr_info *lki, int *dlen)
{
	nm_e1phy_component *e1hd = (nm_e1phy_component *)hd;
	unsigned char *p, *oph = NULL;
	int length = 0;
	int len = 0;

	if (!e1hd || !lki || !dlen)
		return NULL;

	length = e1hd->phycnt * e1hd->pcnt * (sizeof(nm_port_alarm) + sizeof(nm_port_stat));
	*dlen = sizeof(*lki) + 1 + 1 + length;
	len = PKT_LEN + NMPKT_LEN + *dlen;
	oph = malloc(len);
	if (!oph) {
		LOGERROR("%s: malloc failed", __func__);
		return NULL;
	}

	memset(oph, 0, len);
	p = oph + PKT_LEN + NMPKT_LEN;
	memcpy(p, lki, sizeof(*lki));
	p += sizeof(*lki);
	*p++ = e1hd->phycnt;
	*p++ = e1hd->pcnt;
	memcpy(p, e1hd->data, length);
	
	return oph;
}

int nm_e1phy_release_data(void *hd)
{
	nm_e1phy_component *e1hd = (nm_e1phy_component *)hd;

	if (!e1hd)
		return -1;

	if (e1hd->data) {
		free(e1hd->data);
		e1hd->data   = NULL;
		e1hd->phycnt = 0;
		e1hd->pcnt   = 0;
	}

	return 0;
}

int nm_e1phy_close(void *hd)
{
	nm_e1phy_component *e1hd  = (nm_e1phy_component *)hd;
	
	if (!e1hd)
		return -1;

	nm_e1phy_release_data(e1hd);
	free(e1hd);
	e1hd = NULL;

	return 0;
}

