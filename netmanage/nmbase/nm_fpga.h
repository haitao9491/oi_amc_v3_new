#ifndef __H_NM_FPGA_H_
#define __H_NM_FPGA_H_

#include "list.h"
#include "nm_glb.h"

#define FPGA_DATA_STATUS   1
#define FPGA_DATA_STAT     2
#define FPGA_DATA_FSTAT    3

/* fpga port status */
struct fpga_port_status {
	unsigned char los;
	unsigned char lof;
	unsigned char stm1;
	unsigned char e1cnt;
	unsigned int  sig64kchcnt;
	unsigned int  sig64kfrcnt;
	unsigned int  sig2mchcnt;
	unsigned int  sig2mfrcnt;
};

struct fpga_status {
	unsigned char pcnt;
	struct fpga_port_status *st;
};

/* fpga port statistics */
struct fpga_port_stat {
	unsigned int  sig64kchcnt;
	unsigned int  sig64kfrcnt;
	unsigned int  sig2mchcnt;
	unsigned int  sig2mfrcnt;
};

struct fpga_stat {
	unsigned char pcnt;
	struct fpga_port_stat *sta;
};

/* fpga port flow statistics */
struct fpga_port_fstat {
	unsigned int flow;
};

struct fpga_fstat {
	unsigned char pcnt;
	struct fpga_port_fstat *fsta;
};

#if defined(__cplusplus)
extern "C" {
#endif


void *nm_fpga_open(void);
int nm_fpga_add_id_type(void *hd, int id, int type);
int nm_fpga_add_data(void *hd, 
		int id, int type, void *data, int flag);
int nm_fpga_get_id_type(void *hd, 
		int (*fpga_func)(int id, int type, void *arg), void *arg);
int nm_fpga_get_data(void *hd, 
		int id, int type, void *da, int flag);
void *nm_fpga_get_id_type_pkt(void *hd, struct nm_lkaddr_info *lki, int *dlen);
void *nm_fpga_get_data_pkt(void *hd, 
		int id, int type, struct nm_lkaddr_info *lki, int *dlen, int flag);
int nm_fpga_del_data(void *hd, int id, int type);
int nm_fpga_release_data(void *hd);
int nm_fpga_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif
