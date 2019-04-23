/*
 * (C) Copyright 2017
 * wangxiumei <wangxiumei_ryx@163.com>
 *
 * fpga.h - A description goes here.
 *
 */

#ifndef _HEAD_FPGA_002D746C_42632349_664D791E_H
#define _HEAD_FPGA_002D746C_42632349_664D791E_H

typedef struct {
	unsigned char	vrr;		/* version register		*/
	unsigned char	dar[4];		/* date register		*/
} verinfo_t;

typedef struct {
	unsigned char	dsr;		/* ddr status register		*/
	unsigned char	bcr;		/* board control register	*/
} bdstartup_t;

typedef struct {
	unsigned char	ser[4];		/* second register              */
	unsigned char	usr[4];		/* micro-second register        */
	unsigned char	ter;		/* time enable register         */
} timesync_t;

typedef struct {
	unsigned char	itr;		/* information type register	*/
	unsigned char	wvr;		/* write value register		*/
	unsigned char	wer;		/* write enable register	*/
	unsigned char	rvr;		/* read value register		*/
	unsigned char	rer;		/* read enable register		*/
} bdcfginfo_t;

typedef struct {
	unsigned char	ptr;		/* port type register         */
	unsigned char	oir;		/* optic information register */
	unsigned char	str;		/* signalling type register   */
	unsigned char	scnr0;		/* signalling channel number register 0 */
	unsigned char	scnr1;		/* signalling channel number register 1 */
	unsigned char	sfnr0;		/* signalling frame number register 0 */
	unsigned char	sfnr1;		/* signalling frame number register 1 */
	unsigned char	sfnr2;		/* signalling frame number register 2 */
	unsigned char	sfnr3;		/* signalling frame number register 3 */
	unsigned char	etr0;		/* ethernet traffic register 0 */
	unsigned char	etr1;		/* ethernet traffic register 1 */
	unsigned char	etr2;		/* ethernet traffic register 2 */
	unsigned char	etr3;		/* ethernet traffic register 3 */
} bdruninfo_t;

typedef struct {
	unsigned char	btcr0;		/* base timeslot channel register 0 */
	unsigned char	btcr1;		/* base timeslot channel register 1 */
	unsigned char	btvr;		/* base timeslot valid register   */
	unsigned char	bter;		/* base timeslot enable register  */
} trsch_t;

typedef struct {
	unsigned char   srl0;       /* silence range0 low register */
	unsigned char   srh0;       /* silence range0 high register */
	unsigned char   srl1;       /* silence range1 low register */
	unsigned char   srh1;       /* silence range1 high register */
	unsigned char   schr;       /* silence channel high register */
	unsigned char   sclr;       /* silence channel low register */
	unsigned char   srsr;       /* silence return stat register */
	unsigned char   str;        /* silence timeout register */
} silence_t;

typedef struct {
	unsigned char	sel;		/* read write select register */
	unsigned char	bcr;		/* base 2M channel register 1 */
	unsigned char	wrr;		/* write valid register   */
	unsigned char	rdr;		/* read  valid register   */
	unsigned char	ber;		/* base 2M enable register  */
} vld2mch_t;

typedef struct {
	unsigned char	bcr;		/* base 2M channel register 1 */
	unsigned char	bvr;		/* base 2M valid register   */
	unsigned char	ber;		/* base 2M enable register  */
} trs2mch_t;

typedef struct {
	unsigned char	plr;		/* bit0: pl reset bit1: pl enable */
	unsigned char	reserved0[0x75 - 0x71];
	unsigned char	cicr0;		/* cic register 0  */
	unsigned char	cicr1;		/* cic register 1 */
	unsigned char	csr;		/* channel status register    */
	unsigned char	cer;		/* config enable register  */
} plcheck0_t;

typedef struct {
	unsigned char	rrvr;		/* read result valid register    */
	unsigned char	rrsr;		/* read result select register   */
	unsigned char	rrdr;		/* read result data register   */
	unsigned char	rror;		/* read result over register   */
} plcheck1_t;

typedef struct {
	unsigned char	bsr;		/* B error code select register */
	unsigned char	bvr0;		/* B error code value register 0  */
	unsigned char	bvr1;		/* B error code value register 1 */
} becode_t;

typedef struct {
	unsigned char	enr;		/* e1 number register */
	unsigned char	v5ecr[2];	/* v5 error code register */
	unsigned char	v5ptr;		/* v5 ptr register */
	unsigned char	v5vr;		/* v5 value register */

	unsigned char	auptr[2];	/* au ptr register */
	unsigned char	auptrndf0[2]; /* au ptr 5I or 5D revert 0 */
	unsigned char	auptrndf1[2]; /* au ptr 5I or 5D revert 1 */

	unsigned char	tuptrndf0[2]; /* tu ptr 5I or 5D revert 0 */
	unsigned char	tuptrndf1[2]; /* tu ptr 5I or 5D revert 1 */

	unsigned char	ser;		/* sync enable register */
	unsigned char	seecr[2];	/* sync enable error count register */

	unsigned char	eccr;		/* e1 change count register */

	unsigned char	reserved0;
	unsigned char	esdr;		/* e1 speed different register */
} chipstat_t;

typedef struct {
	unsigned char	age_chan2;//new 
	unsigned char	age_chan1;//new
} age_channel; 

typedef struct {
	unsigned char	adr0; /*adr0:7~0 bits is ram addr[11:0] low 8 bits*/ 
	unsigned char	adr1; /*adr0:3~0 bits is ram addr[11:0] high 4 bits; adr0:4 bit is enable; adr0:7 bit is clear*/
	unsigned char	data0; /*sig location[12:0] low 8 bits*/
	unsigned char	data1; /*data1:4~0 bits is sig location[12:0] high 5 bits; data1:5 bit is vaild flag*/
}sigchannel_t;

typedef struct {
	verinfo_t		verinfo;
	unsigned char	reserved0;
	bdstartup_t		startup;
	timesync_t		timesync;
	unsigned char	reserved1[0x1f - 0x10];
	trsch_t			trsch;   /* 64K channel transfer */
	silence_t       slp;
	unsigned char	reserved2[0x2f - 0x2b];
	bdcfginfo_t		cfginfo;
	unsigned char	reserved3[0x4f - 0x34];
	bdruninfo_t		runinfo;
	unsigned char	reserved4[0x63 - 0x5c];
	vld2mch_t		vld2mch; /* 2M channel valid */
	trs2mch_t		trs2mch; /* 2M channel transfer */
	unsigned char	reserved5[0x6f - 0x6b];
	plcheck0_t		plc0;
	unsigned char	reserved6[0x8c - 0x78];
	becode_t		becode;
	unsigned char	reserved7[0xff - 0x8f];
	plcheck1_t		plc1;
	unsigned char	reserved8[0x10f - 0x103];
	age_channel		age_ch;
	unsigned char	reserved9[0x12f - 0x111];
	chipstat_t		cstat;
	unsigned char	reserved10[0x15f - 0x144];
	sigchannel_t	issig;
} fpga_reg;

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FPGA_002D746C_42632349_664D791E_H */
