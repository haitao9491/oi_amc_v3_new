/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * upgdef.h - A description goes here.
 *
 */

#ifndef _HEAD_UPGDEF_45C95FF5_316B2BF1_70A03EC8_H
#define _HEAD_UPGDEF_45C95FF5_316B2BF1_70A03EC8_H

#define MNOTIFY_ADDR		"224.0.0.8"
#define MNOTIFY_PORT		1908
#define MNOTIFY_TX_PORT		1906

#define CLI_CONTROL_PORT	1902
#define SVR_CONTROL_PORT	1904

enum board_type {
	BOARD_TYPE_UNKNOWN = 0,
	BOARD_TYPE_EIPB_V2,
	BOARD_TYPE_MPCB_V3,
	BOARD_TYPE_MACB_V2,
	BOARD_TYPE_OI_AMC_V3,
	BOARD_TYPE_EI_AMC_V1,
	BOARD_TYPE_MACB_V3,
	BOARD_TYPE_OI_AMC_V4,
	BOARD_TYPE_XSCB_V2,
	BOARD_TYPE_XPPP_V2,
};

enum upgrade_protocol_command {
	UG_NOTIFY = 0x2380,
	UG_UPGRADE_COMMAND,
	UG_UPGRADE_ACCEPT,
	UG_UPGRADE_RESULT,
	UG_REBOOT_COMMAND,
	UG_REBOOT_ACCEPT,
	UG_DNGRADE_COMMAND,
	UG_DNGRADE_ACCEPT,
	UG_DNGRADE_RESULT,
};

struct board_info {
	unsigned int   slot;
	unsigned int   subslot;
	unsigned char  type;
	unsigned short basever;
	unsigned short appver;
	unsigned short appver_prev;
	char           ctrlip[32];
	unsigned int   ctrlport;
};

#if defined(__cplusplus)
extern "C" {
#endif

int   get_boardtype(char *boardname);
char *get_boardtype_list();

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_UPGDEF_45C95FF5_316B2BF1_70A03EC8_H */
