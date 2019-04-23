/*
 * (C) Copyright 2017
 * Xu Ronghui <glory.hsu@gmail.com>
 *
 * grpsm.h - A description goes here.
 *
 */

#ifndef _HEAD_GRPSM_23B1205A_6741418E_6ABA3A04_H
#define _HEAD_GRPSM_23B1205A_6741418E_6ABA3A04_H

/* Fiber rate */
#define GRPSM_FIBERRATE_155	0
#define GRPSM_FIBERRATE_622	1
#define GRPSM_FIBERRATE_2500	2
#define GRPSM_FIBERRATE_10000	3

/* Channel rate */
#define GRPSM_CHANNELRATE_C4	1
#define GRPSM_CHANNELRATE_C12	2
#define GRPSM_CHANNELRATE_C11	3
#define GRPSM_CHANNELRATE_MASK	0x0f

/* Service type */
#define GRPSM_SVCTYPE_DEFAULT	0
#define GRPSM_SVCTYPE_GFP	1
#define GRPSM_SVCTYPE_LAPS	2
#define GRPSM_SVCTYPE_PPP	3
#define GRPSM_SVCTYPE_ATM	4
#define GRPSM_SVCTYPE_MASK	0x0f

/* Order */
#define GRPSM_ORDER_UNKNOWN	0
#define GRPSM_ORDER_HO		1
#define GRPSM_ORDER_LO		2

/* Result of detect */
#define GRPSM_DETECTRC_UNKNOWN	0
#define GRPSM_DETECTRC_SUCCESS	1
#define GRPSM_DETECTRC_FAILURE	2

struct link_info {
	unsigned char	fiber;		/* No. of fiber */
	unsigned char	fiber_rate;	/* Fiber rate */
	unsigned short	channel;	/* No. of channel */
	unsigned char	channel_rate;	/* Channel rate */
	unsigned char	vc_valid;
	unsigned char	vc_order;	/* Order of channel */
	unsigned char	svc_type;	/* Service type */
	unsigned char	is_lcas;
	unsigned char	is_member;
	unsigned char	is_last_member;
	unsigned short	mfi;
	unsigned char	sq;
	unsigned int	pre_gid;
	unsigned int	cur_gid;
};

struct group_channel {
	unsigned char	fiber;
	unsigned char	fiber_rate;
	unsigned short	channel;
	unsigned char	channel_rate;
};

struct group_cfg {
	struct group_channel *chs;
	unsigned int	num_chs;
	unsigned char	result;
};

struct group_stat {
	struct link_info link;
	unsigned char	 grouped;
	unsigned int	 groupid;	/* Generated internally */
};

#if defined(__cplusplus)
extern "C" {
#endif

void *grpsm_open();
int   grpsm_push(void *hd, struct link_info *linkinfo, int num_links, struct group_cfg *groupcfg, int num_groups);
int   grpsm_get(void *hd, struct group_cfg *pre_groupcfg, int num_pregroups, int num_trygroups, struct group_cfg **groupcfg);
int   grpsm_stat(void *hd, struct group_stat **groupstat);
void  grpsm_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_GRPSM_23B1205A_6741418E_6ABA3A04_H */
