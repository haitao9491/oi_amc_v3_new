#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include "oic.h"
#include "oicdev.h"
#include "fpga.h"
#include "fst.h"
#include "oicdbg.h"

#define OK 0xff
struct fst_line {
	unsigned int tflag; //table flag
	unsigned short cic;
	struct tline lval;

	unsigned short flag;
};

struct fst_ctl nctl, *nctrl = NULL;

void fst_line_reset(struct fst_line *pf)
{
	memset(pf, 0, sizeof(struct fst_line));
}

int fst_line_valid(struct fst_line *pf)
{
	return !(pf->flag == OK);
}

void fst_line_show(struct fst_line *pf)
{
	if (pf->flag == OK) {
		printk("pf show %d:%d:%d:%d:%d.\n", pf->tflag, pf->cic, pf->lval.ts, pf->lval.op_times, pf->lval.valid_times);
	}
}

int fst_line_get(struct fst_ctl *ctl, struct fst_line *pf)
{
	volatile fpga_reg *fpga = (volatile fpga_reg *)oicdev->fpga_virt;
	volatile plcheck1_t *reg = (volatile plcheck1_t *)&fpga->plc1;
	
	unsigned char val = 0;
	unsigned int cic = 0xffff;
	unsigned int ts = 0xffff;

	if (1 == reg->rrvr) {

		reg->rrsr = 0;
		if (0 == reg->rrsr) {
			val = reg->rrdr;
		}
		pf->tflag = 0;

		reg->rrsr = 1;
		if (1 == reg->rrsr) {
			val = reg->rrdr;
			ts = (val & 0xff) << 8;
		}

		reg->rrsr = 2;
		if (2 == reg->rrsr) {
			val = reg->rrdr;
			ts |=  val & 0xff;
		}
		pf->lval.ts = ts;

		reg->rrsr = 3;
		if (3 == reg->rrsr) {
			val = reg->rrdr;
			cic = (val & 0x3f) << 8;
		}

		reg->rrsr = 4;
		if (4 == reg->rrsr) {
			val = reg->rrdr;
			cic |= val & 0xff;
		}
		pf->cic = cic;

		reg->rrsr = 5;
		if (5 == reg->rrsr) {
			pf->lval.op_times = reg->rrdr;
		}

		reg->rrsr = 6;
		if (6 == reg->rrsr) {
			pf->lval.valid_times = reg->rrdr;
		}

		reg->rror = 1;

		pf->flag = OK;

		return 0;
	}

	return -1;
}
void fst_table_unset(struct fst_ctl *ctl)
{
	if (NULL != ctl) {
		memset(((char *)&ctl->tbl) + 2, 0, sizeof(ctl->tbl) - 2);//ready test
	}
}

int fst_line_set(struct fst_ctl *ctl, struct fst_line *pf)
{
	int index = 0, tnum = 0, cic = 0;
	struct tline *l = NULL;

	if (!fst_line_valid(pf)) {
		tnum = pf->tflag;
		cic = pf->cic;
		index = ctl->tbl.index[tnum][cic];

		if (index < NUM_STAT) {
			l = &ctl->tbl.table[tnum][cic][index];
			memcpy(l, &pf->lval, sizeof(struct tline));

			index++;
			ctl->tbl.index[tnum][cic] = index;
		}

		return 0;
	}

	return -1;
}

int fst_read_stat(void *arg, int tblnum)
{
	int ret = -1;
	unsigned int cnt = 0, vcnt = 0;
	struct fst_line fl, *pfl = NULL;
	struct fst_ctl *ctl = (struct fst_ctl *)arg;
	struct timeval start, end;

	pfl = &fl;
	do_gettimeofday(&start);

	while (1) {
		fst_line_reset(pfl);

		ret = fst_line_get(ctl, pfl);
		pfl->tflag = tblnum;
#if 0
		fst_line_show(pfl);
#endif
		if (ret == 0) {
			fst_line_set(ctl, pfl);
			vcnt++;
		}

		cnt++;
		if ((cnt % 30) == 0) {
			do_gettimeofday(&end);
			if (end.tv_sec - start.tv_sec > 60) {
				break;
			}
		}
		if ((cnt % 10000) == 0) {
			msleep(3);
		}
	}

	printk("fst_read_stat:cnt %d vcnt %d\n", cnt, vcnt);
	return 0;
}

int init_fst(void *arg)
{
	nctrl = &nctl;
	memset(nctrl, 0, sizeof(*nctrl));

	printk("init_fst()...\n");

	if (NULL != arg) {
		nctrl->data = arg;
	}

	oicdev->link_map = (void *)nctrl;

	return 0;
}

void exit_fst(void)
{
	if (NULL != nctrl) {
		nctrl = NULL;
	}
}

