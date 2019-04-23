#define FPGADRV_OIAMC_PORT_MAX      4
#define FPGADRV_OIAMC_E1_MAX	    63
#define FPGADRV_OIAMC_TS_MAX	    32
#define FPGADRV_NAME	            "mem"
#define FPGADRV_OIAMC_CHAN_ALL	    1024 

struct mutex_hd {
	pthread_mutex_t mutex;
};

struct oic_device{
	unsigned char	*fpgareg;
	pthread_mutex_t lock;
};

struct verinfo{
	unsigned char ver;
	unsigned int date;
};

struct src_mac{
	unsigned char mac0;
	unsigned char mac1;
	unsigned char mac2;
	unsigned char mac3;
	unsigned char mac4;
	unsigned char mac5;
};

struct src_IP{
	unsigned char ip_3;
	unsigned char ip_2;
	unsigned char ip_1;
	unsigned char ip_0;
};

struct dst_IP{
	unsigned char ip_3;
	unsigned char ip_2;
	unsigned char ip_1;
	unsigned char ip_0;
};

struct src_port{
unsigned char port_h;
unsigned char port_l;
};

struct dst_port{
unsigned char port_h;
unsigned char port_l;
};

struct fpga_silence{
	unsigned char smin0;
	unsigned char smax0;
	unsigned char smin1;
	unsigned char smax1;
};

struct fpga_cfg{
	unsigned int index;
	unsigned char en;
	unsigned char ts;
	unsigned char mode;
	unsigned char flag;
};

struct tran_en {
	unsigned char en; // en '1': enable '0': disable 
	unsigned char revert; //'1': revert enable '0': revert disable
};

struct sdh_phy_stat {
	unsigned char	port;
	unsigned char	status;
	unsigned short	b1;
	unsigned short	b2;
	unsigned short	b3;
	unsigned char	e1_cnt;
	unsigned char	phy_type;
	unsigned short	chcnt_64k;
	unsigned int	fmcnt_64k;
	unsigned short	chcnt_2m;
	unsigned int 	fmcnt_2m;
	unsigned char 	clear_becode;
};

struct e1_status {
	unsigned int port;
	unsigned int vaild[FPGADRV_OIAMC_E1_MAX];
	unsigned int sync_err[FPGADRV_OIAMC_E1_MAX];
}; 

struct hdlc_channel_stat {
	unsigned char 	port;
	unsigned short	id;
	unsigned char	type;
	unsigned char	e1;
	unsigned char	ts;
	unsigned int	ok_pkts;
	unsigned int	err_pkts;
};

struct get_hdlc_stat_to_file {
	unsigned int		 ch_vaild;
	struct hdlc_channel_stat chstat[FPGADRV_OIAMC_CHAN_ALL];
};

struct chan_tran_cfg {
	unsigned char chan; // chan no: oipc: 1~4 oiamc: don't care
	unsigned char port; // port no: oipc: 1~32 oiamc:1-4
	unsigned char e1; // e1 no: 1~63
	unsigned char ts; // ts no: 0~31
	unsigned char en; // en '1': start '0': stop 
};

int get_bd_info(struct verinfo *ver);
int set_bd_start(void);
int fpgamif_set_sel(void *hd, int *sel);
int fpgamif_get_sel(void *hd, int *sel);
int fpgamif_set_smac(void *hd, struct src_mac *Smac);
int fpgamif_get_smac(void *hd, struct src_mac *Smac);
int fpgamif_set_sip(void *hd, struct src_IP *SIP);
int fpgamif_get_sip(void *hd, struct src_IP *SIP);
int fpgamif_set_dip(void *hd, struct dst_IP *DIP);
int fpgamif_get_dip(void *hd, struct dst_IP *DIP);
int fpgamif_set_Sport(void *hd, struct src_port *Sport);
int fpgamif_get_Sport(void *hd, struct src_port *Sport);
int fpgamif_set_Dport1(void *hd, struct dst_port *Dport1);
int fpgamif_get_Dport1(void *hd, struct dst_port *Dport1);
int fpgamif_set_Dport2(void *hd, struct dst_port *Dport2);
int fpgamif_get_Dport2(void *hd, struct dst_port *Dport2);
int fpgamif_set_dev_id(void *hd, unsigned int *Devid);
int fpgamif_get_dev_id(void *hd, unsigned int *Devid);
int fpgamif_set_fpga_clear_index(void *hd, unsigned int *index);
int fpgamif_set_fpga_silence(void *hd, struct fpga_silence *sn);
int fpgamif_get_fpga_silence(void *hd, struct fpga_silence *sn);
int fpgamif_set_fpga_indexlimit(void *hd, unsigned int *val);
int fpgamif_get_fpga_indexlimit(void *hd, unsigned int *indexlimit);
int fpgamif_set_fpga_rel(void *hd, unsigned int index);
int fpgamif_set_fpga_reset_pointer(void *hd);

int fpgamif_set_fpga_anm(void *hd, unsigned int index);
int fpgamif_set_fpga_scan_clear(void *hd, unsigned char *mask);
int fpgamif_set_fpga_tran_en(void *hd, struct tran_en *tran);
int fpgamif_get_sdh_port_stat(void *hd, struct sdh_phy_stat *phy_stat);
int fpgamif_get_e1_vaild(void *hd, struct e1_status *e1_s);
int fpgamif_get_all_hdlc_channel_stat(void *hd, struct get_hdlc_stat_to_file *stat);
int fpgamif_set_fpga_tran(void *hd, struct chan_tran_cfg *tran);
int fpgamif_get_one_port_phy_status( struct sdh_phy_stat *phy_stat);
int fpgamif_set_fpga_chan_tran(struct chan_tran_cfg *tran);
int fpgamif_get_phy_status(void *hd, struct sdh_phy_stat *phy_stat);

int start_check(void);
int init_mem(void);
void release_mem(void);
void *fpgamif_open (void);
int fpgamif_close (void *hd);
