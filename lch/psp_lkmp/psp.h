/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * psp.h - A description goes here.
 *
 */

#ifndef _HEAD_PSP_45A37693_40853749_37F84B52_H
#define _HEAD_PSP_45A37693_40853749_37F84B52_H

#include "pkt.h"

#define PSP_MSG_REGISTER		0x2360
#define PSP_MSG_REGISTER_ACK	0x2b60	
#define PSP_MSG_REGISTER_UNACK	0x2760
#define PSP_MSG_SCAN_CLEAR      0x2361
#define PSP_MSG_SCAN_CLEAR_ACK	0x2b61
#define PSP_MSG_NOTIFY_ANM      0x2362
#define PSP_MSG_NOTIFY_REL      0x2363
#define PSP_MSG_SCAN_RESULT		0x2364
#define PSP_MSG_VCHANNEL_START	0x2367
#define PSP_MSG_VCHANNEL_STOP	0x2368
#define PSP_MSG_TRAN_ENABLE     0x2369
#define PSP_MSG_STM_MAP_ADD				0x2372
#define PSP_MSG_STM_MAP_DEL				0x2373
#define PSP_MSG_STM_MAP_GET				0x2374
#define PSP_MSG_STM_MAP_RESULT			0x2375
#define PSP_MSG_BLOCK_DATA				0x2376

#define PSP_MSG_PHY_STAT                0x2377
#define PSP_MSG_PHY_STAT_ACK            0x2b77
#define PSP_MSG_CHANNEL_STAT            0x2378
#define PSP_MSG_CHANNEL_STAT_ACK        0x2b78
#define PSP_MSG_GET_STAT                0x2379
#define PSP_MSG_CLEAR_INDEX             0x2385
#define PSP_MSG_CLEAR_INDEX_ACK         0x2b85
#define PSP_MSG_CLEAR_INDEX_NACK        0x2785
#define PSP_MSG_TRAP_LINK_CHANGE        0x2386
#define PSP_MSG_CLEAR_ALL_INDEX			0x2387
#define PSP_MSG_RESET_POINTER			0x2388

#define PSP_MSG_SILENCE_RANGE           0x2381
#define PSP_MSG_SILENCE_RESULT          0x2382

enum PSP_PROBETYPE {
	PSP_PROBETYPE_UNKNOWN = 0x00,
	PSP_PROBETYPE_E1,
	PSP_PROBETYPE_155M_CPOS,
	PSP_PROBETYPE_155M_ATM
};

enum PSP_CARDTYPE {
	PSP_CARDTYPE_AMC = 0x01,
	PSP_CARDTYPE_ATCA,
	PSP_CARDTYPE_OIPC
};

enum PSP_ITFTYPE {
	PSP_ITFTYPE_UNKNOWN = 0x00,
	PSP_ITFTYPE_A,
	PSP_ITFTYPE_ABIS,
	PSP_ITFTYPE_CD,
	PSP_ITFTYPE_IUCS
};

struct psp_msg {
	unsigned short id;

	/* REGISTER */
	struct {
		unsigned char probetype;
		unsigned char cardtype;
		unsigned char slot;
		unsigned char subslot;
	} reg;

	/* SCAN */
	struct {
		unsigned int opc;
		unsigned int dpc;
		unsigned int loglink;
		unsigned char channel; /*ts*/
		unsigned char scanendflag; /*1: scan end*/
		unsigned char num_links;
		unsigned char phylink1;
		unsigned char phylink2;
		unsigned short vpi;
		unsigned short vci;
		unsigned char state;

		unsigned int index;
		unsigned char scanmode;/*1:scan all channel 2:scan specified channel*/
		unsigned char initmask;/*bit0:reset linkmap bit1:mode bit2:get bit3:age channel*/
		unsigned char clear_mask; /* bit0: clear sig. bit1: clear voice. */
	} scan;

	/* PROBE */
	struct {
		unsigned char phylink;
		unsigned char channel;
		unsigned char phylink1;
		unsigned char channel1;
		unsigned short vpi;
		unsigned short vci;
		unsigned char itftype;
	} probe;

	/* MAP */
	struct {
		unsigned char inport;
		unsigned char outport;
	} map;

	/* BLOCK DATA*/
	struct {
		unsigned short flag;
		unsigned short type;
		unsigned int size;
		unsigned char *buf;
	} block;

	struct {
		unsigned char port;
		unsigned char type;
		unsigned char e1;
		unsigned char ts;
		unsigned char endflag;
	} hdlc;

	struct {
		unsigned char port;
		unsigned char loslof;
		unsigned char type;
		unsigned char e1synccnt;
		unsigned short chcnt64k;
		unsigned short chcnt2m;
		unsigned int frmcnt64k;
		unsigned int frmcnt2m;
		unsigned int e1validl;
		unsigned int e1validh;
		unsigned char endflag;
	} phy;

	struct {
		unsigned int flag; /*bit0: get phy stat; bit1: get hdlc stat*/
	} get;

	struct {
		unsigned short index;
	} clear;

    struct {
        unsigned char silencetype;
        unsigned short indexlimit;
		unsigned char revert;
		unsigned char tranen;  /* tran enable */
    } rg; /* register_ack-payload */

	struct {
		unsigned char chan;
		unsigned char port;
		unsigned char e1;
		unsigned char ts;
	} tran;
    
    struct {
        unsigned char revert; 
        unsigned char enable; 
    } tran_en;

	pkt_hdr *ph;
};


#if defined(__cplusplus)
extern "C" {
#endif

int psp_decode_msg(pkt_hdr *ph, struct psp_msg *msg);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PSP_45A37693_40853749_37F84B52_H */
