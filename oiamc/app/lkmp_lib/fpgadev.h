typedef struct {
	unsigned char	ver;		/* version register		*/
	unsigned char	dar[4];		/* date register		*/
} version_t;

typedef struct {
	unsigned char	dsr;		/*0x06*/
	unsigned char	bcr;		/*0x07*/
	unsigned char	brr;		/*0x08*/
} bdstartup_t;

typedef struct {
	unsigned char	cfg_sel_rw;			/*0x30*/
	unsigned char	cfg_info_sel;		
	unsigned char	cfg_info_data_3;
	unsigned char	cfg_info_data_2;
	unsigned char	cfg_info_data_1;
	unsigned char	cfg_info_data_0;
	unsigned char	cfg_info_data_r_3;
	unsigned char	cfg_info_data_r_2;
	unsigned char	cfg_info_data_r_1;
	unsigned char	cfg_info_data_r_0;
	unsigned char	cfg_info_rw_en;		/*0x3a*/
} bdcfginfo_t;

typedef struct {
	unsigned char clear_index_h;             /*0x60*/ 
	unsigned char clear_index_l; 
	unsigned char clear_index_en;			/*0x62*/
} clindex_t;

typedef struct {
	unsigned char	plr;				/*0x70*/
	unsigned char	reserved[0x74 - 0x70];
	unsigned char	cicr0;				/*0x75*/
	unsigned char	cicr1;
	unsigned char	csr;
	unsigned char	cer;				/*0x78*/
} linkmap_t;

typedef struct {
	unsigned char	val;					/*0x7a*/
	unsigned char	sel;					/*0x7b*/
	unsigned char	en;						/*0x7c*/
	unsigned char	rw;						/*0x7d*/
	unsigned char	rvalue;					/*0x7e*/
}slient_t;

typedef struct {
	unsigned char	port_type_sel;			/*0x50*/
	unsigned char	sdh_phy_info;
	unsigned char	sig_type_sel;
	unsigned char	cnt_ch_scan1;
	unsigned char	cnt_ch_scan0;
	unsigned char	cnt_fr3;
	unsigned char	cnt_fr2;
	unsigned char	cnt_fr1;
	unsigned char	cnt_fr0;
	unsigned char	cnt_eth_bit3;
	unsigned char	cnt_eth_bit2;
	unsigned char	cnt_eth_bit1;
	unsigned char	cnt_eth_bit0;			/*0x5c*/
} board_runinfo_t;

typedef struct {
	unsigned char	bselect;				/*0x8d*/
	unsigned char	bcode_h;
	unsigned char	bcode_l;
} bcode_t;

typedef struct {
    unsigned char   chan_en;
    unsigned char   chan_sel_h;
    unsigned char   chan_sel_l;
    unsigned char   chan_type;
    unsigned char   chan_port;
    unsigned char   chan_e1;
    unsigned char   chan_ts;
} chan_stat_t;

typedef struct {
    unsigned char   mif_e1_num;         /* 0x130 */
    unsigned char   reserved0[ 0x143 - 0x130 ];
    unsigned char   e1_valid;           /* 0x144 */
    unsigned char   reserved1[ 0x14f - 0x144 ];
    unsigned char   port_sel;           /* 0x150 */
} e1_status_t;

typedef struct {
    unsigned char ch_transfer_1;
    unsigned char ch_transfer_0;
    unsigned char ch_transfer_din;
    unsigned char ch_transfer_en;
} ch_transfer_t;

typedef struct {
	version_t		version;
	unsigned char	reserved0;
	bdstartup_t		startup;
	unsigned char	reserved1[0x1e - 0x08];
	unsigned char   sel;    /* 0x1f */
    ch_transfer_t   ch_tran;
	unsigned char	reserved2[0x2f - 0x23];
	bdcfginfo_t		cfginfo;
	unsigned char	reserved3[0x4f - 0x3a];
	board_runinfo_t runinfo;
	unsigned char	reserved4[0x5f - 0x5c];
	clindex_t		clindex;
	unsigned char	reserved5[0x6f - 0x62];
	linkmap_t		lkm;
	unsigned char	reset_pointer;
	slient_t		slient;
    chan_stat_t     chan_stat;
	unsigned char	reserved6[0x12f - 0x85]; 
    e1_status_t     e1_status;
} fpga_reg;





