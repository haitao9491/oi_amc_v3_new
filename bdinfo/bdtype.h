/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * bdtype.h - A description goes here.
 *
 */

#ifndef _HEAD_BDTYPE_38657760_55A34158_7D59760C_H
#define _HEAD_BDTYPE_38657760_55A34158_7D59760C_H

enum board_type {
	BOARD_TYPE_UNKNOWN = 0,
	BOARD_TYPE_EIPB_V2,
	BOARD_TYPE_MPCB_V3,
	BOARD_TYPE_MACB_V2,
	BOARD_TYPE_OI_AMC_V3,
	BOARD_TYPE_EI_AMC_V1,
	BOARD_TYPE_MACB_V3,
	BOARD_TYPE_OI_AMC_V4,
};

#if defined(__cplusplus)
extern "C" {
#endif

int   get_boardtype(char *boardname);
char *get_boardtype_list();

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_BDTYPE_38657760_55A34158_7D59760C_H */
