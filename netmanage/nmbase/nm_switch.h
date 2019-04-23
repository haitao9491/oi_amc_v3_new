#ifndef __H_NM_SWITCH_H_
#define __H_NM_SWITCH_H_

#include "list.h"
#include "nm_glb.h"

#pragma pack(1)
struct sw_port_status {     /* status info */
	unsigned char port;     /* port num */
	char portname[12]; /* port name */
	unsigned char autoneg;  /* 1: on , 0: off */
	unsigned char pwrdn;    /* 1: power down, 0: normal */
	unsigned char lks;      /* 1: up, 0: down */
	unsigned char duplex;   /* 0: half 1: full */
	unsigned char speed;    /* 0: unknown 1: 10M 2: 100M 3: 1000M  4:10G */
};
#pragma pack()

struct sw_status {
	int pcnt;   /*port count */
	struct sw_port_status *st;
};

#if defined(__cplusplus)
extern "C" {
#endif

void *nm_switch_open(void);
int nm_switch_add_data(void *hd, void *data);
int nm_switch_get_status(void *hd, struct sw_status *st);
void *nm_sw_get_status_pkt(void *hd, struct nm_lkaddr_info *lki, int *dlen);
int nm_switch_release_data(void *hd);
int nm_switch_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif
