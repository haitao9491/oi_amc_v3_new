#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <pthread.h>
#include "fpgadev.h"
#include "fpgamif.h"
static int memfd = -1;
#define READ_MODE	0x01
#define WRITE_MODE	0x00
#define FPGA_CFG_SEL_SRC_MAC0	0x00
#define FPGA_CFG_SEL_SRC_MAC1	0x01
#define FPGA_CFG_SEL_SRC_IP		0x02
#define FPGA_CFG_SEL_DST_IP		0x03
#define FPGA_CFG_SEL_SRC_PORT	0x04
#define FPGA_CFG_SEL_DST_PORT_1	0x05
#define FPGA_CFG_SEL_DST_PORT_2	0x06
#define FPGA_CFG_SEL_DEV_ID     0x07
#define voc_silentp_min_0		0x0a
#define voc_silentp_max_0		0x0b
#define voc_silentp_min_1		0x0c
#define voc_silentp_max_1		0x0d
#define cic_index_max_l			0x14
#define cic_index_max_h			0x15

#define BDREGCHK(reg, val) { \
 	(reg) = (val); \
 	if ((reg) != (val)) { \
 		printf("[%d] reg 0x%x != val 0x%x \n", __LINE__, reg, val); \
 		return -1; \
 	} \
}


struct oic_device *oicdev = NULL;
struct dev_fd {
	int fd;
};

unsigned char *devreg = NULL;

void *fpgamif_open (void)
{
	struct dev_fd *dev;
	char fname[32];
	dev = (struct dev_fd *)malloc(sizeof(struct dev_fd));
	if (!dev) {
		printf("devfd malloc failed\n");
		return NULL;
	}
	oicdev = (struct oic_device *)malloc(sizeof(struct oic_device));
	if (!oicdev) {
		printf("oicdev malloc failed\n");
		return NULL;
	}
	memset(fname, 0, sizeof(fname));
	memset(oicdev, 0, sizeof(struct oic_device));

	if(pthread_mutex_init(&oicdev->lock, NULL) != 0){
		printf("mutex init faild\n");
		return NULL;
	}
	init_mem();
	return dev;
}

int fpgamif_close(void *hd)
{
	struct dev_fd *dev = (struct dev_fd *)hd;
	close(dev->fd);
	if (!dev)
		free(dev);
	release_mem();
	return 0;
}

int start_check(void)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdstartup_t *start = (volatile bdstartup_t *)&fpga_mif->startup;
	if (start->bcr != 0x01) {
		return -1;
	}
	return 0;
}

int get_bd_info(struct verinfo *ver)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile version_t *verinfo = (volatile version_t *)&fpga_mif->version;
	pthread_mutex_lock(&oicdev->lock);
	ver->ver = verinfo->ver;
	ver->date = (verinfo->dar[0] << 24) | (verinfo->dar[1] << 16) |
		(verinfo->dar[2] << 8) | (verinfo->dar[3] << 0);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int set_bd_start(void)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdstartup_t *start = (volatile bdstartup_t *)&fpga_mif->startup;
	int i = 0;
	pthread_mutex_lock(&oicdev->lock);
	sleep(0.5);
	BDREGCHK(start->brr, 0x00);
	sleep(0.5);
	BDREGCHK(start->brr, 0x01);
	while (start->dsr != 0x01) {
		sleep(1);
		i++;
		if(i >= 10) {
			printf("board start is faild\n");
			pthread_mutex_unlock(&oicdev->lock);
			return -1;			
			}
	}
	BDREGCHK(start->bcr, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	printf("cfg_bd_start\n");
	return 0;
}

int fpgamif_set_sel(void *hd, int *sel)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;

	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(fpga_mif->sel, (*sel & 0xff));
	pthread_mutex_unlock(&oicdev->lock);

	return 0;
}

int fpgamif_get_sel(void *hd, int *sel)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;

	pthread_mutex_lock(&oicdev->lock);
	*sel = fpga_mif->sel;
	pthread_mutex_unlock(&oicdev->lock);

	return 0;
}

int fpgamif_set_smac(void *hd, struct src_mac *Smac)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_MAC0);
	BDREGCHK(bdinfo->cfg_info_data_3, Smac->mac0);
	BDREGCHK(bdinfo->cfg_info_data_2, Smac->mac1);
	BDREGCHK(bdinfo->cfg_info_data_1, Smac->mac2);
	BDREGCHK(bdinfo->cfg_info_data_0, Smac->mac3);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_MAC1);
	BDREGCHK(bdinfo->cfg_info_data_3, Smac->mac4);
	BDREGCHK(bdinfo->cfg_info_data_2, Smac->mac5);
	BDREGCHK(bdinfo->cfg_info_data_1, 0x00);
	BDREGCHK(bdinfo->cfg_info_data_0, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_smac(void *hd, struct src_mac *Smac)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_MAC0);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	Smac->mac0 = bdinfo->cfg_info_data_r_3;
	Smac->mac1 = bdinfo->cfg_info_data_r_2;
	Smac->mac2 = bdinfo->cfg_info_data_r_1;
	Smac->mac3 = bdinfo->cfg_info_data_r_0;
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_MAC1);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	Smac->mac4 = bdinfo->cfg_info_data_r_3;
	Smac->mac5 = bdinfo->cfg_info_data_r_2;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_sip(void *hd, struct src_IP *SIP)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_IP);
	BDREGCHK(bdinfo->cfg_info_data_3, SIP->ip_0);
	BDREGCHK(bdinfo->cfg_info_data_2, SIP->ip_1);
	BDREGCHK(bdinfo->cfg_info_data_1, SIP->ip_2);
	BDREGCHK(bdinfo->cfg_info_data_0, SIP->ip_3);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_sip(void *hd, struct src_IP *SIP)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_IP);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	SIP->ip_0 = bdinfo->cfg_info_data_r_3;
	SIP->ip_1 = bdinfo->cfg_info_data_r_2;
	SIP->ip_2 = bdinfo->cfg_info_data_r_1;
	SIP->ip_3 = bdinfo->cfg_info_data_r_0;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_dip(void *hd, struct dst_IP *DIP)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DST_IP);
	BDREGCHK(bdinfo->cfg_info_data_3, DIP->ip_0);
	BDREGCHK(bdinfo->cfg_info_data_2, DIP->ip_1);
	BDREGCHK(bdinfo->cfg_info_data_1, DIP->ip_2);
	BDREGCHK(bdinfo->cfg_info_data_0, DIP->ip_3);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_dip(void *hd, struct dst_IP *DIP)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DST_IP);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
    	DIP->ip_0 = bdinfo->cfg_info_data_r_3;
   	DIP->ip_1 = bdinfo->cfg_info_data_r_2;
   	DIP->ip_2 = bdinfo->cfg_info_data_r_1;
   	DIP->ip_3 = bdinfo->cfg_info_data_r_0;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_Sport(void *hd, struct src_port *Sport)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_PORT);
	BDREGCHK(bdinfo->cfg_info_data_3, Sport->port_h);
	BDREGCHK(bdinfo->cfg_info_data_2, Sport->port_l);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_Sport(void *hd, struct src_port *Sport)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_SRC_PORT);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
    Sport->port_h = bdinfo->cfg_info_data_r_3;
    Sport->port_l = bdinfo->cfg_info_data_r_2;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_Dport1(void *hd, struct dst_port *Dport1)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DST_PORT_1);
	BDREGCHK(bdinfo->cfg_info_data_3, Dport1->port_h);
	BDREGCHK(bdinfo->cfg_info_data_2, Dport1->port_l);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_Dport1(void *hd, struct dst_port *Dport1)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DST_PORT_1);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	Dport1->port_h = bdinfo->cfg_info_data_r_3;
	Dport1->port_l = bdinfo->cfg_info_data_r_2;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_Dport2(void *hd, struct dst_port *Dport2)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DST_PORT_2);
	BDREGCHK(bdinfo->cfg_info_data_3, Dport2->port_h);
	BDREGCHK(bdinfo->cfg_info_data_2, Dport2->port_l);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_Dport2(void *hd, struct dst_port *Dport2)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DST_PORT_2);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	Dport2->port_h = bdinfo->cfg_info_data_r_3;
	Dport2->port_l = bdinfo->cfg_info_data_r_2;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}


int fpgamif_set_dev_id(void *hd, unsigned int *Devid)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	unsigned char value = 0;
	pthread_mutex_lock(&oicdev->lock);
	value = *Devid & 0xFF;
	BDREGCHK(bdinfo->cfg_sel_rw, WRITE_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DEV_ID);
	BDREGCHK(bdinfo->cfg_info_data_3, value);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_dev_id(void *hd, unsigned int *Devid)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile bdcfginfo_t *bdinfo = (volatile bdcfginfo_t *)&fpga_mif->cfginfo;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(bdinfo->cfg_sel_rw, READ_MODE);
	BDREGCHK(bdinfo->cfg_info_sel, FPGA_CFG_SEL_DEV_ID);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x00);
	BDREGCHK(bdinfo->cfg_info_rw_en, 0x01);
	*Devid = (unsigned int)bdinfo->cfg_info_data_r_3;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_fpga_clear_index(void *hd, unsigned int *index)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile clindex_t *clindex = (volatile clindex_t *)&fpga_mif->clindex;
	unsigned char value = 0;
	pthread_mutex_lock(&oicdev->lock);
	value = clindex->clear_index_en;
	if(0x80 != (value & 0x80)){
		printf("clear index is faild\n");
		return -1;
	}
	value = 0x3f & (*(unsigned int *)index >> 8);
	BDREGCHK(clindex->clear_index_h, value);
	value = 0xff & (*(unsigned int *)index >> 0);
	BDREGCHK(clindex->clear_index_l, value);
	BDREGCHK(clindex->clear_index_en, 0x80);
	BDREGCHK(clindex->clear_index_en, 0x81);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_fpga_silence(void *hd, struct fpga_silence *sn)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile slient_t *slient = (volatile slient_t *)&fpga_mif->slient;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(slient->rw, 0);
	BDREGCHK(slient->val, sn->smin0);
	BDREGCHK(slient->sel, voc_silentp_min_0);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	BDREGCHK(slient->rw, 0);
	BDREGCHK(slient->val, sn->smax0);
	BDREGCHK(slient->sel, voc_silentp_max_0);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	BDREGCHK(slient->rw, 0);
	BDREGCHK(slient->val, sn->smin1);
	BDREGCHK(slient->sel, voc_silentp_min_1);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	BDREGCHK(slient->rw, 0);
	BDREGCHK(slient->val, sn->smax1);
	BDREGCHK(slient->sel, voc_silentp_max_1);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_fpga_silence(void *hd, struct fpga_silence *sn)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile slient_t *slient = (volatile slient_t *)&fpga_mif->slient;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(slient->rw, 1);
	BDREGCHK(slient->sel, voc_silentp_min_0);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	sn->smin0 = slient->rvalue;
	BDREGCHK(slient->rw, 1);
	BDREGCHK(slient->sel, voc_silentp_max_0);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	sn->smax0 = slient->rvalue;
	BDREGCHK(slient->rw, 1);
	BDREGCHK(slient->sel, voc_silentp_min_1);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	sn->smin1 = slient->rvalue;
	BDREGCHK(slient->rw, 1);
	BDREGCHK(slient->sel, voc_silentp_max_1);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	sn->smax1 = slient->rvalue;
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_fpga_indexlimit(void *hd, unsigned int *val)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile slient_t *slient = (volatile slient_t *)&fpga_mif->slient;
	unsigned char va = 0;
	pthread_mutex_lock(&oicdev->lock);
	va = (*(unsigned int *)val) & 0xFF;
	BDREGCHK(slient->rw, 0);
	BDREGCHK(slient->val, va);
	BDREGCHK(slient->sel, cic_index_max_l);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	va = (*(unsigned int *)val >> 8) & 0x1F;
	BDREGCHK(slient->rw, 0);
	BDREGCHK(slient->val, va);
	BDREGCHK(slient->sel, cic_index_max_h);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_get_fpga_indexlimit(void *hd, unsigned int *indexlimit)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile slient_t *slient = (volatile slient_t *)&fpga_mif->slient;
	unsigned char va_l = 0;
	unsigned char va_h = 0;
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(slient->rw, 1);
	BDREGCHK(slient->sel, cic_index_max_l);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	va_l = slient->rvalue;
	BDREGCHK(slient->rw, 1);
	BDREGCHK(slient->sel, cic_index_max_h);
	BDREGCHK(slient->en, 0x00);
	BDREGCHK(slient->en, 0x01);
	va_h = slient->rvalue;
	*indexlimit = (va_h << 8) | (va_l << 0);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_fpga_rel(void *hd, unsigned int index)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile linkmap_t *lkm = (volatile linkmap_t *)&fpga_mif->lkm;
	unsigned char val = 0;
	pthread_mutex_lock(&oicdev->lock);
	val = (index >> 8) & 0xff;
	BDREGCHK(lkm->cicr0, val);
	val = index & 0xff;
	BDREGCHK(lkm->cicr1, val);
	BDREGCHK(lkm->csr, 0x00);
	BDREGCHK(lkm->cer, 0x00);
	BDREGCHK(lkm->cer, 0x01);
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_fpga_reset_pointer(void *hd)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile unsigned char *reset_pointer = (volatile unsigned char *)(&fpga_mif->reset_pointer);
	pthread_mutex_lock(&oicdev->lock);
	BDREGCHK(*reset_pointer, 0);
	BDREGCHK(*reset_pointer, 1);	
	pthread_mutex_unlock(&oicdev->lock);
	return 0;
}

int fpgamif_set_fpga_scan_clear(void *hd, unsigned char *mask)
{
	return 0;
}

int fpgamif_set_fpga_tran_en(void *hd, struct tran_en *tran)
{
	return 0;
}


int fpgamif_get_e1_vaild(void *hd, struct e1_status *e1_s)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
    volatile e1_status_t *e1 = (volatile e1_status_t *)(&fpga_mif->e1_status);
    int i = 0;

	BDREGCHK(e1->port_sel, e1_s->port);
    for (i = 0; i < FPGADRV_OIAMC_E1_MAX; i++) {
        BDREGCHK(e1->mif_e1_num, i);
        e1_s->vaild[i] = e1->e1_valid;
        e1_s->sync_err[i] = 0;
    }

	return 0;
}

#define GET_HDLC_BY_PORT            1
#define GET_HDLC_ALL                2

static int get_hdlc_info(int mode, unsigned int port, unsigned int *chan_num, struct hdlc_channel_stat *stat)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile chan_stat_t *chan_reg = (volatile chan_stat_t *)(&fpga_mif->chan_stat);
    struct hdlc_channel_stat *chans_stat = stat;
    struct hdlc_channel_stat *chan_stat = NULL;
    unsigned char tmpval = 0;
    unsigned int num = 0;
    int i = 0;

	pthread_mutex_lock(&oicdev->lock);
    for (i = 0; i < FPGADRV_OIAMC_CHAN_ALL; i++) {
        /* select chan-num */
        tmpval = (i & 0xff00) >> 8;
        BDREGCHK(chan_reg->chan_sel_h, tmpval);
        tmpval = (i & 0x00ff) >> 0;
        BDREGCHK(chan_reg->chan_sel_l, tmpval);

        /* select chan-num enable */
        BDREGCHK(chan_reg->chan_en, 0);
        BDREGCHK(chan_reg->chan_en, 1);

        tmpval = chan_reg->chan_ts;
        if ((tmpval < 1) || (tmpval > 31)) {
            /* chan unvalid */
            continue; 
        }

        /* chan valid */
        chan_stat = &chans_stat[num];
        num++;

        if (mode == GET_HDLC_BY_PORT) {
            if (port != chan_reg->chan_port) {
                continue;
            }
        }

        chan_stat->ts = chan_reg->chan_ts;
        chan_stat->e1 = chan_reg->chan_e1;
        chan_stat->port = chan_reg->chan_port;
        chan_stat->type = chan_reg->chan_type;
        chan_stat->id = num;
    }

    *chan_num = num;
	pthread_mutex_unlock(&oicdev->lock);

    return 0;
}

int fpgamif_get_all_hdlc_channel_stat(void *hd, struct get_hdlc_stat_to_file *stat)
{
    memset(stat->chstat, 0, sizeof(struct hdlc_channel_stat) * FPGADRV_OIAMC_CHAN_ALL);
    if (get_hdlc_info(GET_HDLC_ALL, 0, &stat->ch_vaild, stat->chstat) != 0) {
        printf("Failed to get_hdlc_info.\n");
        return -1;
    }

	return 0;
}

/* transfer voice chan */
int fpgamif_set_fpga_chan_tran(struct chan_tran_cfg *tran)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile ch_transfer_t *tran_reg = (volatile ch_transfer_t *)(&fpga_mif->ch_tran);
    int port = 0;
    int e1 = 0;
    int ts = 0;
    int channel = 0;

    port = tran->port - 1;
    e1 = tran->e1 - 1;
	ts = tran->ts;
    channel = (port * 64 * 32) + (e1 * 32) + ts;

    BDREGCHK(tran_reg->ch_transfer_1, ((channel >> 8) & 0xff));
    BDREGCHK(tran_reg->ch_transfer_0, ((channel >> 0) & 0xff));
    BDREGCHK(tran_reg->ch_transfer_din, tran->en);
    BDREGCHK(tran_reg->ch_transfer_en, 0);
    BDREGCHK(tran_reg->ch_transfer_en, 1);

	return 0;
}

int fpgamif_get_one_port_phy_status(struct sdh_phy_stat *phy_stat)
{
	volatile fpga_reg *fpga_mif = (volatile fpga_reg *)oicdev->fpgareg;
	volatile board_runinfo_t *runinfo = (volatile board_runinfo_t *)(&fpga_mif->runinfo);
    unsigned char loslof = 0;
    int port = 0;


	pthread_mutex_lock(&oicdev->lock);
    port = phy_stat->port;
	BDREGCHK(runinfo->port_type_sel, (port & 0x0f) << 4);
    loslof = ((runinfo->sdh_phy_info & 0x80) >> 7) | ((runinfo->sdh_phy_info & 0x40) >> 5);
	phy_stat->status = loslof;
	phy_stat->e1_cnt = runinfo->sdh_phy_info & 0x3f;

    /* bcode 1 */
	phy_stat->b1 = 0;
    /* bcode 2 */
	phy_stat->b2 = 0;
    /* bcode 3 */
	phy_stat->b3 = 0;

    /* fpga un define phy_type, default 1. */
	phy_stat->phy_type = 1;

    /* 64k link cnt, frame cnt*/
    BDREGCHK(runinfo->sig_type_sel, 0);
    phy_stat->chcnt_64k = (runinfo->cnt_ch_scan1 << 8) | (runinfo->cnt_ch_scan0 << 0);
    phy_stat->fmcnt_64k = (runinfo->cnt_fr3 << 24) | (runinfo->cnt_fr2 << 16) | (runinfo->cnt_fr1 << 8) | (runinfo->cnt_fr0 << 0);

    /* 2m link cnt, frame cnt*/
    BDREGCHK(runinfo->sig_type_sel, 1);
    phy_stat->chcnt_2m = (runinfo->cnt_ch_scan1 << 8) | (runinfo->cnt_ch_scan0 << 0);
    phy_stat->fmcnt_2m = (runinfo->cnt_fr3 << 24) | (runinfo->cnt_fr2 << 16) | (runinfo->cnt_fr1 << 8) | (runinfo->cnt_fr0 << 0);

	pthread_mutex_unlock(&oicdev->lock);

	return 0;
}

int fpgamif_get_phy_status(void *hd, struct sdh_phy_stat *phy_status)
{
    struct sdh_phy_stat *pstat = NULL;
    int i = 0;

    for (i = 0; i < FPGADRV_OIAMC_PORT_MAX; i++) {
        pstat = &phy_status[i]; 
        pstat->port = i;
        if (fpgamif_get_one_port_phy_status(pstat) != 0) {
            printf("fpgamif_get_one_port_phy_status is failed \n");
            continue;
        } 
    }

	return 0;
}

int fpgamif_set_fpga_anm(void *hd, unsigned int index)
{
	return 0;
}

int init_mem(void)
{
	if ((memfd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		printf("Failed to open /dev/mem\n");
		return -1;
	}
	oicdev->fpgareg = (unsigned char *)mmap(0, 0x500, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0xf1000000);
	if (oicdev->fpgareg == (unsigned char *)MAP_FAILED) {
		printf("Failed to map oicdev->fpgareg registers.\n");
		return -1;
	}
	return 0;
}

void release_mem(void)
{
	if (oicdev->fpgareg != (unsigned char *)MAP_FAILED) {
		munmap(oicdev->fpgareg, 0x500);
		oicdev->fpgareg = (unsigned char *)MAP_FAILED;
	}
	pthread_mutex_destroy(&oicdev->lock);
}
