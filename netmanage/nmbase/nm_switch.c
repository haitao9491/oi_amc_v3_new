/*
 *
 * nm_switch.c - A brief description goes here.
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
#include "list.h"
#include "mutex.h"
#include "pkt.h"
#include "nmpkt.h"
#include "nm_typdef.h"
#include "nm_switch.h"

struct sw_component_ctl {
	struct sw_status st;
};

void *nm_switch_open(void)
{
	struct sw_component_ctl *ctl = NULL;

	ctl = (struct sw_component_ctl *)malloc(sizeof(*ctl));
	if (!ctl) 
		return NULL;

	memset(ctl, 0, sizeof(*ctl));

	return ctl;
}

int nm_switch_add_data(void *hd, void *data)
{
	struct sw_component_ctl *ctl = (struct sw_component_ctl *)hd;
	unsigned char *da = (unsigned char *)data;
	int pcnt = 0;
	int len = 0;

	if (!ctl || !data)
		return -1;

	pcnt = *da++;
	len = pcnt * sizeof(struct sw_port_status);
	if (ctl->st.pcnt == pcnt && ctl->st.st != NULL) {
		memcpy(ctl->st.st, da, len);
	}
	else {
		if (ctl->st.st) {
			free(ctl->st.st);
			ctl->st.st = NULL;
		}
		ctl->st.pcnt = pcnt;
		ctl->st.st = malloc(len);
		if (ctl->st.st) {
			memcpy(ctl->st.st, da, len);
		}
	}

	return 0;
}

int nm_switch_get_status(void *hd, struct sw_status *st)
{
	struct sw_component_ctl *ctl = (struct sw_component_ctl *)hd;

	if (!ctl || !st)
		return -1;

	st->pcnt = ctl->st.pcnt;
	st->st   = ctl->st.st;

	return 0;
}

void *nm_sw_get_status_pkt(void *hd, struct nm_lkaddr_info *lki, int *dlen)
{
	struct sw_component_ctl *ctl = (struct sw_component_ctl *)hd;
	unsigned char *op, *oph = NULL;
	int len = 0;

	if (!ctl || !lki || !dlen)
		return NULL;

	*dlen = sizeof(*lki) + 1 + ctl->st.pcnt * sizeof(*(ctl->st.st));
	len = PKT_LEN + NMPKT_LEN + *dlen;
	oph = malloc(len);
	if (!oph) {
		LOGERROR("%s: malloc failed", __func__);
		return NULL;
	}

	memset(oph, 0, len);
	op = oph + PKT_LEN + NMPKT_LEN;
	memcpy(op, lki, sizeof(*lki));
	op += sizeof(*lki);
	*op++ = ctl->st.pcnt;
	memcpy(op, ctl->st.st, ctl->st.pcnt * sizeof(*(ctl->st.st)));

	return oph;
}

int nm_switch_release_data(void *hd)
{
	struct sw_component_ctl *ctl = (struct sw_component_ctl *)hd;

	if (!ctl)
		return -1;

	if (ctl->st.st) 
		free(ctl->st.st);
	ctl->st.pcnt = 0;

	return 0;
}

int nm_switch_close(void *hd)
{
	struct sw_component_ctl *ctl = (struct sw_component_ctl *)hd;

	if (!ctl)
		return -1;

	nm_switch_release_data(ctl);
	free(ctl);
	ctl = NULL;

	return 0;
}

