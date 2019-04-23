/*
 * (C) Copyright 2016.
 *
 * fst.h
 *
 */

#ifndef __FST_H__
#define __FST_H__

#define NUM_TBL 2
#define NUM_CIC 65536
#define NUM_STAT 4

struct tline{
	unsigned int ts;
	unsigned char op_times;
	unsigned char valid_times;
};

struct table {
	unsigned char slot;
	unsigned char subslot;

	unsigned short index[NUM_TBL][NUM_CIC];
	struct tline table[NUM_TBL][NUM_CIC][NUM_STAT];
#if 0
	struct tline table_1[NUM_CIC][NUM_STAT];
	struct tline table_2[NUM_CIC][NUM_STAT];
	struct tline table_3[NUM_CIC][NUM_STAT];
#endif
};

struct fst_ctl {
	struct table tbl;

	void *data;
};

#define FST_PC_CMP(pc, _opc, _dpc)  (((pc).opc == _opc && (pc).dpc == _dpc) ? 1 : 0)
#define FST_PC_SET(pc, _opc, _dpc)  do{(pc).opc = _opc; (pc).dpc = _dpc;}while(0)

int init_fst(void *arg);
void fst_init_pc(struct fst_ctl*, int, int);
void fst_table_unset(struct fst_ctl *ctl);
int fst_read_stat(void *arg, int tblnum);
void exit_fst(void);
#endif
