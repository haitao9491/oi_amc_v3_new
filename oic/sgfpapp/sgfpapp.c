#include <stdlib.h>
//#include <sys/time.h>
#include <string.h>

#include "os.h"
#include "aplog.h"
#include "cconfig.h"

#include "sgfplib.h"

#define PORT_MAX            1024
#define CHANNEL_MAX         63
#define CHANNEL_NULL        255
#define HDLC_PORTMAX        4
#define LOCAL               1
#define CONFIG              2
#define SET_FLAG_TRY        1
#define SET_FLAG_GROUP      2

void *__lgwr__handle = NULL;

enum {
	GET = 0,
	TRY,
	SET,
	SHOW_TRY,
	SHOW_SET,
	STATE,
	CURRENT,
	HISTORY,
	GE_STAT,
	HDLC_SCHAN,
	HDLC_SCAN,
	HDLC_DB_STATUS,
	SET_FRAME_LEN,
	SHOW_FRAME_LEN,
	CFG_SCAN_MODE,
	SHOW_SCAN_MODE,
	CFG_SELNUM,
	SHOW_SELNUM,
	CFG_FORWARD_ENABLE,
	CFG_FORWARD_DISABLE,
	CFG_FORWARD_RULE,
	CFG_CHANNEL_FORWARD,
    GET_H
};

static char* cfgfile = NULL;
static int port = 0;
static int port_last = 0;
static int channel = 0;
static int channel_last = 0;
static int try_cout = 1;
static int cmd_type = 0;
static struct slink_info *g_link = NULL;
static struct sgroup_all_info *g_gallinfo = NULL;
static void* ghd = NULL;
static int ch_info_flag = 0;
static int g_svc_info_flag = 0;
static int g_port_info_clear = 0;
static int g_port_info_fpga = 0;
static int g_port_info_group = 0;
static int g_ge_stat_clear = 0; 
static int g_hdlc_port = 0; 
static int g_hdlc_port_last = 0; 
static int g_chstat_mask = 0; 
static int g_frame_min = 0;
static int g_frame_max = 0;
static int g_scan_mode_mask = 0; 
static int g_set_flag = 0;

static unsigned char selnum;
static unsigned char value;
struct spu_forward_rule rul;
struct chan_flag chan_flag;

static char frame_cfg_name[] = "/application/etc/sgfpapp.cfg";

struct sgfpcfg {
	int fr_len_min;
	int fr_len_max;
	int scan_mode;
};

static void sgfp_show_usage(char *progname)
{
    printf("        sgfpapp show port_info <clear>                : show info of port.\n");
    printf("        sgfpapp show port_info fpga <0-3> group <1-8> : show info of port for high.\n");
    printf("        sgfpapp show svc_info                         : show info of port business.\n");
    printf("		sgfpapp show slink <1-4> <1-63>               : show info of port<1-4>, channel<1-63>.\n");
    printf("		sgfpapp show slink <1-4>                      : show info of port<1-4> all channel.\n");
    printf("		sgfpapp show slink                            : show info of all port all channel.\n");
    printf("		sgfpapp show ch_info <1-4> <1-63>             : show ch_info of port<1-4>, channel<1-63>.\n");
    printf("		sgfpapp show ch_info <1-4>                    : show ch_info of port<1-4> all channel.\n");
    printf("		sgfpapp show ch_info                          : show ch_info of all port all channel.\n");
    printf("		sgfpapp cfg try <try_cnt> <cfg>               : cfg try-group-rule times of <try_cnt> by <cfg>.\n");
    printf("		sgfpapp cfg group [fpgaid] <cfg>              : cfg formal-group-rule by <cfg>.\n");
    printf("		sgfpapp show scan_info                        : show the last try group info\n");
    printf("		sgfpapp show set_info                         : show the last set group info\n");
    printf("		sgfpapp current study                         : show current learned group groups.\n");
    printf("		sgfpapp history study                         : show history learned groups.\n");
    printf("		sgfpapp show ge_stat <clear>                  : show HDLC GE information.\n");
    printf("		sgfpapp show hdlc schan [port]                : show HDLC link information.\n");
    printf("		sgfpapp show hdlc scan [local|config]         : show HDLC FPGA scan information of local or config.\n");
    printf("		sgfpapp cfg hdlc frame_len <0-65534> <0-65534> : set HDLC min and max frame length.\n");
    printf("		sgfpapp show hdlc frame_len                    : show HDLC frame length.\n");
    printf("		sgfpapp cfg hdlc scan_mode [local|config]     : set HDLC scan mode.\n");
    printf("		sgfpapp show hdlc scan_mode                   : show HDLC scan mode.\n");
    printf("		sgfpapp show hdlc db_status                   : show HDLC internal debug information.\n");
    printf("		sgfpapp cfg selnum <0-255>                    : set selnum.\n");
    printf("		sgfpapp show selnum               	      : show selnum.\n");
    printf("		sgfpapp cfg h2lfd <0-1> <0-63> <0-3> <0-255>  : set HOP2LOP forward rule of src port <0-1>, src chennel <0-63>, \n");
    printf("								   			    dst port <0-3>, dst selnum <0-255>\n");
    printf("		sgfpapp cfg l2lmfd <0-3> <0-62> <0-3> <0-255> : set LOP2LKMP forward rule of src port <0-3>, src channel <0-62>, \n");	
    printf("											     dst port <0-3>, dst selnum <0-255>\n");
    printf("		sgfpapp cfg forward enable  		      : set forward enable\n");
    printf("		sgfpapp cfg forward disable		      : set forward disable\n");
    printf("		sgfpapp cfg channel <1-64> enable    	      : set channel <1-64> and enable\n");	
    printf("		sgfpapp cfg channel <1-64> disable  	      : set channel <1-64> and disable\n");	
}

static int sgfp_parse_args(int argc, char **argv)
{
    int i = 1;
    int tport = 0;

	if (argc <= 2) {
		printf("please input parameter\n");
		return -1;
	}

	if ((strcmp(argv[i], "show") == 0) &&
		((strcmp(argv[i+1], "port_info") == 0) ||
		(strcmp(argv[i+1], "svc_info") == 0))) {
		if (argc < 3) {
			printf("unknown parameter\n");
			return -1;
		}

		if (argc == 4) {
			if (strcmp(argv[i+2], "clear") == 0)
				g_port_info_clear = 1;
		}
		if (argc == 5) {
			if (strcmp(argv[i+2], "group") == 0)
				g_port_info_group = atoi(argv[i+3]);

            if (g_port_info_group < 1 || g_port_info_group > 8) {
                printf("input group 1-8.\n"); 
                return -1;
            }
        }
        if (argc == 7) {
			if (strcmp(argv[i+2], "fpga") == 0) {
				g_port_info_fpga = (atoi(argv[i+3]) + 1);
                if (g_port_info_fpga > 4) {
                    return -1; 
                }
            }
			if (strcmp(argv[i+4], "group") == 0)
				g_port_info_group = atoi(argv[i+5]);

            if (g_port_info_group < 1 || g_port_info_group > 2) {
                printf("input group 1-2.\n");
                return -1;
            }
        }
		cmd_type = STATE;
    } else if ((strcmp(argv[i], "show") == 0) &&
			((strcmp(argv[i+1], "slink") == 0) ||
              (strcmp(argv[i+1], "ch_info") == 0)) ) {
        if (strcmp(argv[i+1], "ch_info") == 0) {
            ch_info_flag = 1;
        }
		cmd_type = GET;
		if (argc == 3) {
			port = PORT_MAX;
			channel = CHANNEL_MAX;
		}
		else if (argc == 4) {
			port = strtol(argv[3], NULL, 0);
			if (port == PORT_MAX) {
				port_last = 1;
			}
			if (port < 0 || port > PORT_MAX) {
				printf("unknown port\n");
				return -1;
			}
            
			channel = CHANNEL_MAX;
		}
		else if (argc == 5) {
			port = strtol(argv[3], NULL, 0);
			if (port == PORT_MAX) {
				port_last = 1;
			}
			if (port <= 0 || port > PORT_MAX) {
				printf("unknown port\n");
				return -1;
			}
			channel = strtol(argv[4], NULL, 0);
			if (channel == CHANNEL_MAX) {
				channel_last = 1;
			}
			if (channel <= 0 || channel > CHANNEL_MAX) {
				printf("unknown channel\n");
				return -1;
			}
		}
		else if (argc == 6) {
			if (strcmp(argv[3], "port") != 0) {
                return -1;
            }
            //10G port 
            tport = strtol(argv[4], NULL, 0);

            if (tport <= 2) {
                g_port_info_fpga = 1;
            }
            else if ((tport >= 3) && (tport <= 4)) {
                g_port_info_fpga = 2;
            }
            else if ((tport >= 5) && (tport <= 6)) {
                g_port_info_fpga = 3;
            }
            else if ((tport >= 7) && (tport <= 8)) {
                g_port_info_fpga = 4;
            } else {
                printf("unknown port %d\n", tport);
                return -1;
            }
            //stm-1 port
            port = strtol(argv[5], NULL, 0);
            if (port <= 0 || port > 64) {
				printf("unknown port\n");
				return -1;
			}
            if (tport % 2 == 0) {
                port +=64;
            }
			channel = CHANNEL_MAX;
        }
		else if (argc == 7) {
            if (strcmp(argv[3], "port") != 0) {
                return -1;
            }
            //10G port 
            port = strtol(argv[4], NULL, 0);
            if (port <= 2) {
                g_port_info_fpga = 1;
            }
            else if ((port >= 3) && (port <= 4)) {
                g_port_info_fpga = 2;
            }
            else if ((port >= 5) && (port <= 6)) {
                g_port_info_fpga = 3;
            }
            else if ((port >= 7) && (port <= 8)) {
                g_port_info_fpga = 4;
            } else {
                printf("unknown port %d\n", port);
                return -1;
            }
            //stm-1 port
            port = strtol(argv[5], NULL, 0);
            if (port <= 0 || port > 64) {
				printf("unknown port\n");
				return -1;
			}
            channel = strtol(argv[6], NULL, 0);
			if (channel <= 0 || channel > CHANNEL_MAX) {
				printf("unknown channel\n");
				return -1;
			}
        }
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "show") == 0) &&
          (strcmp(argv[i+1], "slink-h") == 0))  {
              cmd_type = GET_H;
              if (argc == 7) {
                  if (strcmp(argv[3], "fpga") != 0) {
                      return -1;
                  }
                  //10G port
                  g_port_info_fpga = (strtol(argv[4], NULL, 0) + 1);
                  if (g_port_info_fpga > 4) {
                        return -1;
                  }
                  if (strcmp(argv[5], "group") != 0) {
                      return -1;
                  }
                  g_port_info_group = strtol(argv[6], NULL, 0);
              }
    }
	else if ((strcmp(argv[i], "cfg") == 0) &&
			(strcmp(argv[i+1], "try") == 0)) {
		if (argc == 5) {
			cmd_type = TRY;
			try_cout = strtol(argv[3], NULL, 0);
			cfgfile = argv[4];
		} else if (argc == 7) {
			cmd_type = TRY;
            if (strcmp(argv[3], "fpga") != 0) {
                return -1;
            }
            g_port_info_fpga = (strtol(argv[4], NULL, 0) + 1);
            if (g_port_info_fpga > 4) {
                return -1;
            }
			try_cout = strtol(argv[5], NULL, 0);
			cfgfile = argv[6];
        } else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
			(strcmp(argv[i+1], "group") == 0)) {
		if (argc == 4) {
			cmd_type = SET;
			cfgfile = argv[3];
        } else if (argc == 6) {
			cmd_type = SET;
            if (strcmp(argv[3], "fpga") == 0) {
                g_port_info_fpga = (strtol(argv[4], NULL, 0) + 1);
            }
			cfgfile = argv[5];
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}	
	else if ((strcmp(argv[i], "show") == 0) &&
			(strcmp(argv[i+1], "scan_info") == 0)) {
		if (argc == 3) {
			cmd_type = SHOW_TRY;
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "show") == 0) &&
			(strcmp(argv[i+1], "set_info") == 0)) {
		if (argc == 3) {
			cmd_type = SHOW_SET;
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "current") == 0) &&
			(strcmp(argv[i+1], "study") == 0)) {
		if (argc == 3) {
			cmd_type = CURRENT;
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "history") == 0) &&
			(strcmp(argv[i+1], "study") == 0)) {
		if (argc == 3) {
			cmd_type = HISTORY;
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "ge_stat") == 0)) {
		if ((argc != 3) && (argc != 4)) {
			printf("unknown parameter\n");
			return -1;
		}

		if (argc == 4) {
			if (strcmp(argv[i+2], "clear") == 0)
				g_ge_stat_clear = 1;
		}
		cmd_type = GE_STAT;
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "schan") == 0)) {
		if ((argc != 4) && (argc != 5)) {
			printf("unknown parameter\n");
			return -1;
		}
		g_chstat_mask = GET_CHSTAT_MASK;
		cmd_type = HDLC_SCHAN;
		g_hdlc_port = HDLC_PORTMAX; 
		if (argc == 5) {
			g_hdlc_port = strtol(argv[4], NULL, 0);
			if ((g_hdlc_port < 1) || (g_hdlc_port > 4)) {
				printf("hdlc port out of range!\n");
				return -1;
			}
			if (g_hdlc_port  == HDLC_PORTMAX) {
				g_hdlc_port_last = 1;
			}
		}
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "scan") == 0)) {
		if ((argc != 5) && (argc != 6)) {
			printf("unknown parameter\n");
			return -1;
		}
		if (strcmp(argv[i+3], "local") == 0) {
			g_chstat_mask = GET_CHSTAT_LOCAL_MASK;
		}
		else if (strcmp(argv[i+3], "config") == 0) {
			g_chstat_mask = GET_CHSTAT_CFG_MASK;
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
		cmd_type = HDLC_SCAN;
		if (argc == 6) {
			g_hdlc_port = strtol(argv[5], NULL, 0);
			if ((g_hdlc_port < 1) || (g_hdlc_port > 4)) {
				printf("hdlc port out of range!\n");
				return -1;
			}
			if (g_hdlc_port  == HDLC_PORTMAX) {
				g_hdlc_port_last = 1;
			}
		}
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "db_status") == 0)) {
		if ((argc != 4) && (argc != 5)) {
			printf("unknown parameter\n");
			return -1;
		}
		g_chstat_mask = GET_CHSTAT_DB_MASK;
		cmd_type = HDLC_DB_STATUS;
		if (argc == 5) {
			g_hdlc_port = strtol(argv[4], NULL, 0);
			if ((g_hdlc_port < 1) || (g_hdlc_port > 4)) {
				printf("hdlc port out of range!\n");
				return -1;
			}
			if (g_hdlc_port  == HDLC_PORTMAX) {
				g_hdlc_port_last = 1;
			}
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "frame_len") == 0)) {
		if (argc != 6) {
			printf("unknown parameter\n");
			return -1;
		}
		cmd_type = SET_FRAME_LEN;
		g_frame_min = strtol(argv[4], NULL, 0);
		g_frame_max = strtol(argv[5], NULL, 0);
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "frame_len") == 0)) {
		if (argc != 4) {
			printf("unknown parameter\n");
			return -1;
		}
		cmd_type = SHOW_FRAME_LEN;
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "scan_mode") == 0)) {
		if (argc != 5) {
			printf("unknown parameter\n");
			return -1;
		}
		if (strcmp(argv[i+3], "local") == 0) {
			g_scan_mode_mask = LOCAL; 	
		}
		else if (strcmp(argv[i+3], "config") == 0) {
			g_scan_mode_mask = CONFIG; 	
		}
		cmd_type = CFG_SCAN_MODE;
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "hdlc") == 0) &&
		(strcmp(argv[i+2], "scan_mode") == 0)) {
		if (argc != 4) {
			printf("unknown parameter\n");
			return -1;
		}
		cmd_type = SHOW_SCAN_MODE;
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "selnum") == 0)){
		if(argc == 4){
			int Iselnum = strtol(argv[3], NULL, 0);
			if ((Iselnum < 0) || (Iselnum > 256)) {
				printf("selnum out of range!\n");
				return -1;
			}
			selnum = (unsigned char)Iselnum;
			cmd_type = CFG_SELNUM;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "show") == 0) &&
		(strcmp(argv[i+1], "selnum") == 0)){
		if(argc == 3){
			cmd_type = SHOW_SELNUM;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "h2lfd") == 0)){
		if(argc == 7){
			int Isrc_port = strtol(argv[3], NULL, 0);
			int Isrc_channel = strtol(argv[4], NULL, 0);
			int Idst_port = strtol(argv[5], NULL, 0);
			int Idst_selnum = strtol(argv[6], NULL, 0);

			if ((Isrc_port < 0) || (Isrc_port > 1)) {
				printf("src_port out of range!\n");
				return -1;
			}
			if ((Isrc_channel < 0) || (Isrc_channel > 63)) {
				printf("src_channel out of range!\n");
				return -1;
			}
			if ((Idst_port < 0) || (Idst_port > 3)) {
				printf("dst_port out of range!\n");
				return -1;
			}
			if ((Idst_selnum < 0) || (Idst_selnum > 255)) {
				printf("dst_selnum out of range!\n");
				return -1;
			}
			rul.src_port = (unsigned char)Isrc_port;
			rul.src_channel = (unsigned char)Isrc_channel;
			rul.dst_port = (unsigned char)Idst_port;
			rul.dst_selnum = (unsigned char)Idst_selnum;
			cmd_type = CFG_FORWARD_RULE;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "l2lmfd") == 0)){
		if(argc == 7){
			int Isrc_port = strtol(argv[3], NULL, 0);
			int Isrc_channel = strtol(argv[4], NULL, 0);
			int Idst_port = strtol(argv[5], NULL, 0);
			int Idst_selnum = strtol(argv[6], NULL, 0);

			if ((Isrc_port < 0) || (Isrc_port > 3)) {
				printf("src_port out of range!\n");
				return -1;
			}
			if ((Isrc_channel < 0) || (Isrc_channel > 62)) {
				printf("src_channel out of range!\n");
				return -1;
			}
			if ((Idst_port < 0) || (Idst_port > 3)) {
				printf("dst_port out of range!\n");
				return -1;
			}
			if ((Idst_selnum < 0) || (Idst_selnum > 255)) {
				printf("dst_selnum out of range!\n");
				return -1;
			}
			rul.src_port = (unsigned char)Isrc_port;
			rul.src_channel = (unsigned char)Isrc_channel;
			rul.dst_port = (unsigned char)Idst_port;
			rul.dst_selnum = (unsigned char)Idst_selnum;
			cmd_type = CFG_FORWARD_RULE;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "forward") == 0) &&
		(strcmp(argv[i+2], "enable") == 0)){
		if(argc == 4){
			cmd_type = CFG_FORWARD_ENABLE;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "forward") == 0) &&
		(strcmp(argv[i+2], "disable") == 0)){
		if(argc == 4){
			cmd_type = CFG_FORWARD_DISABLE;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "channel") == 0) &&
		(strcmp(argv[i+3], "enable") == 0)){
		if(argc == 5){
			int Ichan = strtol(argv[3], NULL, 0);
			chan_flag.flag = 1;
			if ((Ichan < 1) || (Ichan > 64)) {
				printf("channel out of range!\n");
				return -1;
			}
			chan_flag.chan = (unsigned char)Ichan;
			cmd_type = CFG_CHANNEL_FORWARD;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if ((strcmp(argv[i], "cfg") == 0) &&
		(strcmp(argv[i+1], "channel") == 0) &&
		(strcmp(argv[i+3], "disable") == 0)){
		if(argc == 5){
			int Ichan = strtol(argv[3], NULL, 0);
			chan_flag.flag = 0;
			if ((Ichan < 1) || (Ichan > 64)) {
				printf("channel out of range!\n");
				return -1;
			}
			chan_flag.chan = (unsigned char)Ichan;
			cmd_type = CFG_CHANNEL_FORWARD;
		}
		else{
			printf("unknown parameter\n");
			return -1;
		}
	}
	else {
		printf("Unknown option: %s\n", argv[i]);
		return -1;
	}
	return 0;

}
static int init_env()
{
	g_link = (struct slink_info*)malloc(sizeof(struct slink_info));
	if (g_link == NULL) {
		LOGERROR("malloc linkinfo faild!");
		return -1;
	}
	
	g_gallinfo = (struct sgroup_all_info*)malloc(sizeof(struct sgroup_all_info));
	if (g_gallinfo == NULL) {
		LOGERROR("malloc group info faild!");
		return -1;
	}

	return 0;
}

static void exit_env(void)
{
    if (ghd)
       sgfplib_close(ghd);
	if (g_link)
		free(g_link);
	if (g_gallinfo)
		free(g_gallinfo);
}

static void print_linkinfo_h(struct fpga_board_runinfo_port_ex *info)
{
	char buff[1028] = "";
	char p_rate[4][8] = {"155M", "622M", "2.5G", "10G"};
	char ch_rate[3][8] ={"C4", "C12", "C11"};
	char svc_type[5][8] = {"OTH", "GFP", "LAPS","PPP","ATM"};
	char tmp_p_rate[8] = "";
	char tmp_ch_rate[8] = "";
	char tmp_svc_type[8] = "";
	char channel_num[8] = "";
    char syncerr[8] = "";
//  char c4err[8] = "";
    char sqerr[8] = "";
    char mfierr[8] = "";

    char tuptr[8] = "";
    char v5[8] = "";
    char v5cnt[8] = "";
    char rev0[8] = "";
    char no0[8] = "";
    char rev1[8] = "";
    char no1[8] = "";

	char ce1_sync[8] = "";
	char ce1_err[8] = "";
	char cnfr_e1_sync[8] = "";
	char cnfr_e1_err[8] = "";

    sprintf(channel_num, "%u", (info->channel + 1));
    sprintf(syncerr, "%u", 0);
    sprintf(sqerr, "%u", info->sq_cnt);
    sprintf(mfierr, "%u", info->mfi_cnt);
    sprintf(tuptr, "%u", 0);
    sprintf(v5, "%u", 0);
    sprintf(v5cnt, "0x%04x", 0);
    sprintf(rev0, "%u", 0);
    sprintf(no0, "%u", 0);
    sprintf(rev1, "%u", 0);
    sprintf(no1, "%u", 0);
    sprintf(ce1_sync, "%u", 0);
    sprintf(ce1_err, "%u", 0);
    sprintf(cnfr_e1_sync, "%u", 0);
    sprintf(cnfr_e1_err, "%u", 0);

	if (info->phy_type <= 3) {
		strcpy(tmp_p_rate, p_rate[info->phy_type]);
	}
	else {
		strcpy(tmp_p_rate, "0");
	}
	
	if (info->chan_rate >= 1 && info->chan_rate <= 3) {
		strcpy(tmp_ch_rate, ch_rate[info->chan_rate - 1]);
	}
	else {
		sprintf(tmp_ch_rate, "%u", info->chan_rate);
	}
	
	if (info->svc_type >= 0 && info->svc_type <= 4) {
		strcpy(tmp_svc_type, svc_type[info->svc_type]);
	}
	else {
		sprintf(tmp_svc_type, "%u", info->svc_type);
	}
    
    sprintf(buff, "%3u  %4s  %3s   %4s       %3u  %3u %3u  %3u  %5u %3u  0x%04x  0x%04x    %4s    " \
                "%3s        %3s    %3s", info->fiber, tmp_p_rate,
                channel_num, tmp_ch_rate, info->vc_valid, info->is_lcas, info->is_member,
                info->is_last_member, info->mfi,info->sq, (info->pre_gid & 0xffffffff), info->cur_gid, tmp_svc_type,
                syncerr, sqerr, mfierr);
    printf("%s\n", buff);
}

static void print_linkinfo(struct slink_info* info)
{
	char buff[1028] = "";
	char p_rate[4][8] = {"155M", "622M", "2.5G", "10G"};
	char ch_rate[3][8] ={"C4", "C12", "C11"};
	char svc_type[5][8] = {"OTH", "GFP", "LAPS","PPP","ATM"};
	char tmp_p_rate[8] = "";
	char tmp_ch_rate[8] = "";
	char tmp_svc_type[8] = "";
	char channel_num[8] = "";
    char v5err[8] = "";
    char k4err[8] = "";
//  char c4err[8] = "";
    char sqerr[8] = "";
    char mfierr[8] = "";

    char tuptr[8] = "";
    char v5[8] = "";
    char v5cnt[8] = "";
    char rev0[8] = "";
    char no0[8] = "";
    char rev1[8] = "";
    char no1[8] = "";

	char ce1_sync[8] = "";
	char ce1_err[8] = "";
	char cnfr_e1_sync[8] = "";
	char cnfr_e1_err[8] = "";

	if (info->channel_rate == 1) {
		strcpy(channel_num, "*");
		strcpy(v5err, "*");
		strcpy(k4err, "*");
		strcpy(sqerr, "*");
		strcpy(mfierr, "*");
		strcpy(tuptr, "*");
		strcpy(v5, "*");
		strcpy(v5cnt, "*");
		strcpy(rev0, "*");
		strcpy(no0, "*");
		strcpy(rev1, "*");
		strcpy(no1, "*");
		strcpy(ce1_sync, "*");
		strcpy(ce1_err, "*");
		strcpy(cnfr_e1_sync, "*");
		strcpy(cnfr_e1_err, "*");
	}
	else {
		sprintf(channel_num, "%u", (info->channel + 1));
		sprintf(v5err, "%u", info->v5_sync_cnt);
		sprintf(k4err, "%u", info->k4_sync_cnt);
		sprintf(sqerr, "%u", info->sq_cnt);
		sprintf(mfierr, "%u", info->mfi_cnt);
		sprintf(tuptr, "%u", info->tuptr_val);
		sprintf(v5, "%u", info->v5_val);
		sprintf(v5cnt, "0x%04x", info->v5_cnt);
		sprintf(rev0, "%u", info->tuptr_0110_rev);
		sprintf(no0, "%u", info->tuptr_0110_no);
		sprintf(rev1, "%u", info->tuptr_1001_rev);
		sprintf(no1, "%u", info->tuptr_1001_no);
		sprintf(ce1_sync, "%u", info->e1_sync);
		sprintf(ce1_err, "%u", info->e1_sync_err);
		sprintf(cnfr_e1_sync, "%u", info->nfm_e1_sync);
		sprintf(cnfr_e1_err, "%u", info->nfm_e1_sync_err);
	}

	if (info->fiber_rate <= 3) {
		strcpy(tmp_p_rate, p_rate[info->fiber_rate]);
	}
	else {
		strcpy(tmp_p_rate, "0");
	}
	
	if (info->channel_rate >= 1 && info->channel_rate <= 3) {
		strcpy(tmp_ch_rate, ch_rate[info->channel_rate - 1]);
	}
	else {
		sprintf(tmp_ch_rate, "%u", info->channel_rate);
	}
	
	if (info->svc_type >= 0 && info->svc_type <= 4) {
		strcpy(tmp_svc_type, svc_type[info->svc_type]);
	}
	else {
		sprintf(tmp_svc_type, "%u", info->svc_type);
	}
    if (ch_info_flag > 0) {
	    sprintf(buff, "%3u  %4s  %3s   %4s   %3s %3s %6s     %3s    %3s     %3s    %3s"\
				"    %3s    %3s         %3s        %3s", (info->fiber + 1), tmp_p_rate,
                channel_num, tmp_ch_rate, tuptr, v5, v5cnt, rev0, no0, rev1, no1,
				ce1_sync, ce1_err, cnfr_e1_sync, cnfr_e1_err);
    }
    else {
        sprintf(buff, "%3u  %4s  %3s   %4s       %3u  %3u %3u  %3u  %5u %3u  0x%04x  0x%04x    %4s    " \
                "%3s    %3s    %3s    %3s     %3s", (info->fiber + 1), tmp_p_rate,
                channel_num, tmp_ch_rate, info->vc_valid, info->is_lcas, info->is_member,
                info->is_last_member, info->mfi,info->sq, info->pre_gid, info->cur_gid, tmp_svc_type,
                v5err, k4err, "*",sqerr, mfierr);
    }
	printf("%s\n", buff);
}

static int get_slink_h(void* hd)
{
    int i = 0;
    int rc = 0;
    int start_port = 0;
    int end_port = 64;
	struct fpga_board_runinfo_port_ex port_states;

    memset(&port_states, 0, sizeof(&port_states));

	rc = sgfplib_set_linkinfo_start(hd);
	if (rc) {
		printf("set linkinfo start failed\n");
		return -1;
	}

    //start_port = (g_port_info_group - 1)*64; 
    //end_port = start_port + 64;

    printf("  p p_rate  ch ch_rate  vc_sync lcas mem l_mem   mfi  sq pre_gid cur_gid ch_type" \
           "  vc4_sync_err   sq_err mfi_err\n");
    for (i = start_port; i < end_port; i++) {
        memset(&port_states, 0, sizeof(&port_states));
        port_states.fiber = (((g_port_info_group - 1) & 0x03) << 6) | (i & 0x3f);
		port_states.channel = 0;
        //rc = sgfplib_get_linkinfo(hd, g_link);			
        rc = sgfplib_get_hp_linkinfo(hd, &port_states);
        if (rc) {
            printf("get operation failed!\n");
            return -1;
        }
        print_linkinfo_h(&port_states);
    }
    rc = sgfplib_set_linkinfo_end(hd);
	if (rc) {
		printf("set linkinfo end failed\n");
		return -1;
	}
    return 0;
}

static int get_process(void* hd)
{
    int k = 0, i = 0, j = 0;
	int rc = 0;

	rc = sgfplib_set_linkinfo_start(hd);
	if (rc) {
		printf("set linkinfo start failed\n");
		return -1;
	}

    if (ch_info_flag > 0) {
        printf("  p p_rate  ch ch_rate Tuptr  V5 v5_err " \
                  "0110rev 0110no 1001rev 1001no"\
				  " e1_sync e1_err nfr_e1_sync nfr_e1_err\n");
    }
    else {
        printf("  p p_rate  ch ch_rate  vc_sync lcas mem l_mem   mfi  sq pre_gid cur_gid ch_type" \
                  " v5_err k4_err c4_err sq_err mfi_err\n");
    }
	if ((port != PORT_MAX) || (port_last == 1)) {
		if ((channel != CHANNEL_MAX) || (channel_last == 1)) {
			memset(g_link, 0, sizeof(struct slink_info));
			g_link->fiber = port - 1;
			g_link->channel = channel - 1;
			rc = sgfplib_get_linkinfo(hd, g_link);
			if (rc) {
				printf("get operation failed!\n");
				return -1;
			}
			print_linkinfo(g_link);
		}
		else {
			for (k = 0; k < CHANNEL_MAX; k++) {
				memset(g_link, 0, sizeof(struct slink_info));
				g_link->fiber = port - 1;
				g_link->channel = k;
				rc = sgfplib_get_linkinfo(hd, g_link);			
				if (rc) {
					printf("get operation failed!\n");
					return -1;
				}
				print_linkinfo(g_link);
				//c4 only one channel
				if (g_link->channel_rate == 1) {
					break;
				}
			}
		}
	}
	else {
		if (channel == CHANNEL_MAX) {
			for (i = 0; i < PORT_MAX; i++) {
				for (j = 0; j < CHANNEL_MAX; j++) {
					memset(g_link, 0, sizeof(struct slink_info));
					g_link->fiber = i;
					g_link->channel = j;
					rc = sgfplib_get_linkinfo(hd, g_link);			
					if (rc) {
						printf("get operation failed!\n");
						return -1;
					}
					print_linkinfo(g_link);
					//c4 only one channel
					if (g_link->channel_rate == 1) {
						break;
					}
				}
			}
		}
		if (channel != CHANNEL_MAX) {
			LOGERROR("No such situation");
			return -1;
		}
	}
	rc = sgfplib_set_linkinfo_end(hd);
	if (rc) {
		printf("set linkinfo end failed\n");
		return -1;
	}
	return 0;
}

static int get_state(void *hd)
{
    int i = 0, j = 0;
	int rc = 0;
	int j_max = 0;
	char phy_type[4][8] = {"155M", "622M", "2.5G", "10G"};
	char ch_rate[3][8] ={"C4", "C12", "C11"};
	char tmp_ch_rate[8] = "";
	char tmp_phy_type[8] = "";
	char buff[1028] = "";
	unsigned char vcnum[4] = {0};
	unsigned char gfpnum[4] = {0};
	unsigned char atmnum[4] = {0};
	unsigned char pppnum[4] = {0};
	unsigned char lapsnum[4] = {0};

	struct fpga_board_runinfo_ex port_states;

	memset(&port_states, 0, sizeof(struct fpga_board_runinfo_ex));
	memset(vcnum, 0 , sizeof(vcnum));
	memset(gfpnum, 0 , sizeof(gfpnum));
	memset(atmnum, 0 , sizeof(atmnum));
	memset(pppnum, 0 , sizeof(pppnum));
	memset(lapsnum, 0 , sizeof(pppnum));

	rc = sgfplib_set_linkinfo_start(hd);
	if (rc) {
		printf("set linkinfo start failed\n");
		return -1;
	}

	for (i = 0; i < 4; i++) {
		for (j = 0; j < CHANNEL_MAX; j++) {
			memset(g_link, 0, sizeof(struct slink_info));
			g_link->fiber = i;
			g_link->channel = j;
			rc = sgfplib_get_linkinfo(hd, g_link);			
			if (rc) {
				printf("get operation failed!\n");
				return -1;
			}

			if (g_link->vc_valid == 1) {
				vcnum[i]++;
				if (g_link->svc_type == 1)
					gfpnum[i]++;
				else if (g_link->svc_type == 2)
					lapsnum[i]++;
				else if (g_link->svc_type == 3)
					pppnum[i]++;
				else if (g_link->svc_type == 4)
					atmnum[i]++;
			}
			if (g_link->channel_rate == 1)
				break;
		}
	}

	rc = sgfplib_set_linkinfo_end(hd);
	if (rc) {
		printf("set linkinfo end failed\n");
		return -1;
	}

	if (g_port_info_clear) {
		port_states.clear = 1;
	}
    if (g_port_info_group == 0) {
        port_states.start_port = 0;
        j_max = 1;
    } else if(g_port_info_group > 0) {
        port_states.start_port = (g_port_info_group - 1)*64;
        j_max = 16;
    }
    
    if (g_svc_info_flag) {
        printf("prot vc_num gfp laps ppp atm  e1 nfe1    64k_num    64k_frm"\
               "     2m_num     2m_frm\n");	
    }
    else {
        printf("port los stm1  phy    b1    b2    b3  auptr 0110rev 0110no " \
                       "1001rev 1001no   c2 ch_rate \n");
    }

    for (j = 0; j < j_max; j++) {
        rc = sgfplib_get_fpga_bd_runinfo_ex(hd, &port_states);
        if (rc) {
            printf("get state failed\n");
            return -1;
        }
        for (i = 0; i < FPGA_OI_PORTNUM; i++) {
            if ( port_states.ports[i].phy_type <= 3) {
                strcpy(tmp_phy_type, phy_type[port_states.ports[i].phy_type]);
            }
            else {
                strcpy(tmp_phy_type, "0");
            }

            if (port_states.ports[i].stm1_synced == 0)
                port_states.ports[i].stm1_synced = 1;
            else if(port_states.ports[i].stm1_synced == 1)
                port_states.ports[i].stm1_synced = 0;

            if (port_states.ports[i].chan_rate >= 1 && port_states.ports[i].chan_rate <= 3) {
                strcpy(tmp_ch_rate, ch_rate[port_states.ports[i].chan_rate - 1]);
            }
            else {
                sprintf(tmp_ch_rate, "%u", port_states.ports[i].chan_rate);
            }

            if (g_svc_info_flag) {
                sprintf(buff, " %3d    %3u %3u  %3u %3u %3u  %3u %3u "\
                        "%10u %10u %10u %10u",
                        ((j*4)+i+1), vcnum[i], gfpnum[i], lapsnum[i],
                        pppnum[i], atmnum[i], port_states.ports[i].e1_cnt,
                        port_states.ports[i].nfm_e1_cnt, port_states.ports[i].ch_64k_num,
                        port_states.ports[i].ch_64k_frames, port_states.ports[i].ch_2m_num,
                        port_states.ports[i].ch_2m_frames);
            }
            else {
                sprintf(buff, "%3d  %3u  %3u %4s %5u %5u %5u 0x%04x     %3u    %3u" \
                        "     %3u    %3u 0x%02x     %3s",
                        ((j*4)+i+1), port_states.ports[i].los, port_states.ports[i].stm1_synced,
                        tmp_phy_type, port_states.ports[i].b1_cnt,
                        port_states.ports[i].b2_cnt, port_states.ports[i].b3_cnt, 
                        port_states.ports[i].auptr_val, port_states.ports[i].auptr_0110_rev,
                        port_states.ports[i].auptr_0110_no, port_states.ports[i].auptr_1001_rev,
                        port_states.ports[i].auptr_1001_no,
                        port_states.ports[i].c2_val, tmp_ch_rate);
            }
            printf("%s\n", buff);
        }
        port_states.start_port += 4;
    }
	return 0;
}

static int read_cfgid(unsigned long hd, int index, int sec_index)
{
	char *p, *q, value[128], buftmp[64];
	int len = 0;
	int fiber = 0, fiber_rate = 0, channel = 0, channel_rate = 0;

	if (CfgGetValue(hd, (char *)"Sgfp Group Info",
                 (char *)"link", value, index+1, sec_index+1) == -1) {
        LOGERROR("Loading link failed.");
        return -1;
    }

	len = strlen(value);
	/*fiber*/
	p = value;
	q = strchr(p, ',');
	if (q == NULL) {
		LOGERROR("No port");
		return -1;
	}
	memset(buftmp, 0, sizeof(buftmp));
	strncpy(buftmp, p, q-p);
	fiber = atoi(buftmp);
	if ((fiber < 0) || (fiber >= 16)) {
		LOGERROR("fiber out of range!");
		return -1;
	}
    g_gallinfo->port_group = (fiber >> 2) & 0x03;
	g_gallinfo->ginfo[sec_index].linkarrays[index].fiber = (unsigned char)fiber;
	p = q + 1;

	/*fiber_rate*/
	q = strchr(p, ',');
	if (q == NULL) {
		LOGERROR("No port rate");
		return -1;
	}
	memset(buftmp, 0, sizeof(buftmp));
	strncpy(buftmp, p, q-p);
	fiber_rate = atoi(buftmp);
	if ((fiber_rate != 0) && (fiber_rate != 1) && (fiber_rate != 2) && (fiber_rate != 3)) {
		LOGERROR("fiber rate out of range!");
		return -1;
	}
	g_gallinfo->ginfo[sec_index].linkarrays[index].fiber_rate = (unsigned char)atoi(buftmp);
	p = q + 1;

	/*channel*/
	q = strchr(p, ',');
	if (q == NULL) {
		LOGERROR("No channel");
		return -1;
	}
	memset(buftmp, 0, sizeof(buftmp));
	strncpy(buftmp, p, q-p);
	channel = atoi(buftmp);
	if (channel < 0 || channel > 128) {
		LOGERROR("channel out of range 128!");
		return -1;
	}
	g_gallinfo->ginfo[sec_index].linkarrays[index].channel = (unsigned char)atoi(buftmp);
	p = q + 1;

	/*channel_rate*/
	if ((p-value) >= len) {
		LOGERROR("No channel rate");
		return -1;
	}
	memset(buftmp, 0, sizeof(buftmp));
	strcpy(buftmp, p);
	channel_rate = atoi(buftmp);
	if ((channel_rate != 1) && (channel_rate != 2) && (channel_rate != 3)) {
		LOGERROR("channel rate out of range!");
		return -1;
	}
	g_gallinfo->ginfo[sec_index].linkarrays[index].channel_rate = (unsigned char)atoi(buftmp);

    if (index == 0)
    {
        if (g_set_flag == SET_FLAG_TRY)
        {
            g_gallinfo->ginfo[sec_index].group_id = sec_index;
        } else {
            g_gallinfo->ginfo[sec_index].group_id = ((fiber & 0x03) << 6) | (channel & 0x3f);
        }
        printf("group_id 0x%x\n", g_gallinfo->ginfo[sec_index].group_id);
    }
	return 0;
}

static void print_groupinfo(int try_num)
{
	int i;
	char buf[1028] = "";

	for (i = 0; i< g_gallinfo->gsize; i++) {
			sprintf(buf, "%6d%8d%8u%8u", try_num+1, i, g_gallinfo->ginfo[i].resokpkt,
				g_gallinfo->ginfo[i].reserrpkt);
		
			printf("%s\n",buf);
		}
}

static void debug_try_print_groupinfo()
{
	int i, j, k;

	LOG("try_cout == %d",try_cout);
	for (k = 0; k < try_cout; k++) {
		for (i = 0; i< g_gallinfo->gsize; i++) {
			for (j = 0; j < g_gallinfo->ginfo[i].linkarrays_size; j++ ) {
                LOG("try_num:%d,groupid:%d,%u,%u,%u,%u", k+1, i+1,
                    g_gallinfo->ginfo[i].linkarrays[j].fiber,
					g_gallinfo->ginfo[i].linkarrays[j].fiber_rate,
                    g_gallinfo->ginfo[i].linkarrays[j].channel,
					g_gallinfo->ginfo[i].linkarrays[j].channel_rate);
			}
        }
    }
}

static void debug_set_print_groupinfo()
{
	int i, j;

    for (i = 0; i< g_gallinfo->gsize; i++) {
        for (j = 0; j < g_gallinfo->ginfo[i].linkarrays_size; j++ ) {
            LOG("groupid:%d,%u,%u,%u,%u", i+1,
                g_gallinfo->ginfo[i].linkarrays[j].fiber,
                g_gallinfo->ginfo[i].linkarrays[j].fiber_rate,
                g_gallinfo->ginfo[i].linkarrays[j].channel,
                g_gallinfo->ginfo[i].linkarrays[j].channel_rate);
        }
    }
}

static int alloc_ginfo(char *cfgfile)
{
	unsigned long cfghd;
    int secnum = 0;
	int idnum = 0;
	int i, j;
	
    cfghd = CfgInitialize(cfgfile);
    if (cfghd == 0ul) {
        printf("cfghd error");
        return -1;
    }
	
    secnum = CfgGetCount(cfghd, (char *)"Sgfp Group Info", NULL, 0);
	g_gallinfo->gsize = secnum;
	g_gallinfo->ginfo = (struct sgroup_info *)malloc(sizeof(struct sgroup_info)*secnum);
	if (g_gallinfo->ginfo == NULL) {
		printf("malloc group info failed!\n");
		return -1;
	}

	for (i = 0; i < secnum; i++) {
		idnum = CfgGetCount(cfghd, (char *)"Sgfp Group Info", (char *)"link", i+1);
		g_gallinfo->ginfo[i].is_valid = 1;
		g_gallinfo->ginfo[i].linkarrays_size = idnum;
		g_gallinfo->ginfo[i].linkarrays = (struct slink_info *)malloc(sizeof(struct slink_info)*idnum);
		memset(g_gallinfo->ginfo[i].linkarrays, 0 , sizeof(struct slink_info)*idnum);
		if (g_gallinfo->ginfo[i].linkarrays == NULL) {
			LOGERROR("malloc linkinfo of group info faild!");
			return -1;
		}
		for (j = 0; j < idnum; j++) {
			if (read_cfgid(cfghd, j, i) < 0) {
				LOGERROR("read config file faild!");
				return -1;
			};
		}
	}
	CfgInvalidate(cfghd);
	return 0;
}

void free_ginfo()
{
	int i = 0;
	for (i = 0; i < g_gallinfo->gsize; i++) {
			if (g_gallinfo->ginfo[i].linkarrays)
				free(g_gallinfo->ginfo[i].linkarrays);
		}

	if (g_gallinfo->ginfo)
		free(g_gallinfo->ginfo);
}

static int try_process(void* hd, char* cfgfile, int cout)
{
	int i = 0, rc = 0;

    g_set_flag = SET_FLAG_TRY;
	printf("  trynum  groupid  ok_num  error_num\n");
	for(i = 0; i < cout; i++) {
		if(alloc_ginfo(cfgfile) < 0) {
			printf("alloc ginfo failed!\n");
			free_ginfo();
			return -1;
		};
		rc = sgfplib_set_try_group(hd, g_gallinfo);
		if (rc == 0) {
			/*debug*/
			debug_try_print_groupinfo();
			/*debug*/
			print_groupinfo(i);
		}
		else if (rc == 1) {
			printf("try operation outof time\n ");
			free_ginfo();
			return -1;
		}
		else {
			printf("try operation error!");
			free_ginfo();
			return -1;
		}
		free_ginfo();
	}
	return 0;
}

static int show_file(char * file_name)
{
	char buf[256] = "";
	FILE *fp = NULL;

	if ((fp=fopen(file_name,"r"))==NULL) {
        printf("%s Open Failed. Maybe is generating\n",file_name);
        return -1;
    }
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		printf("%s", buf);
	}
	return 0;
}

static int set_process(void* hd, char* cfgfile)
{
	int rc = 0;

    g_set_flag = SET_FLAG_GROUP;
	if (alloc_ginfo(cfgfile) < 0) {
		printf("alloc ginfo failed!\n");
		free_ginfo();
		return -1;
	};
	rc = sgfplib_set_group(hd, g_gallinfo);
	if (rc) {
		printf("set operation faild!\n");
		free_ginfo();
		return -1;
	}
	debug_set_print_groupinfo();
	
	free_ginfo();
	return 0;
}

static void show_sgroupinfo(struct sgroup_all_info* gsinfo)
{
	int i = 0, j = 0;
	char p_rate[4][8] = {"155M", "622M", "2.5G", "10G"};
	char ch_rate[3][8] ={"C4", "C12", "C11"};
	char tmp_p_rate[8] = "";
	char tmp_ch_rate[8] = "";
	char buf[1028] = "";
	struct slink_info* info = NULL;
	struct slink_info* slink = NULL;

	struct sgroup_info *sgroup = gsinfo->ginfo;
	for (i = 0; i < gsinfo->gsize; i++) {
		printf("group_id type NO. port  ch ch_rate\n");
		for (j = 0; j < ((sgroup + i)->linkarrays_size); j++) {
			slink = (sgroup + i)->linkarrays;
			info = (slink +j);
			if (info->fiber_rate <= 3) {
				strcpy(tmp_p_rate, p_rate[info->fiber_rate]);
			}
			else {
				strcpy(tmp_p_rate, "0");
			}

			if (info->channel_rate >= 1 && info->channel_rate <= 3) {
				strcpy(tmp_ch_rate, ch_rate[info->channel_rate - 1]);
			}
			else {
				sprintf(tmp_ch_rate, "%u", info->channel_rate);
			}

			sprintf(buf, "     %3d %4s %3d  %3u %3u     %3s\n",
					i+1, tmp_p_rate, j+1, info->fiber, info->channel, tmp_ch_rate);
			printf("%s", buf);
		}
	}
}

static void clear_gsinfo(struct sgroup_all_info *gsinfo)
{
	int i = 0;
	for (i = 0; i < gsinfo->gsize; i++) {
		struct sgroup_info *sgroup = gsinfo->ginfo;
		if (sgroup == NULL)
			break;

		if (sgroup + i) {
			if ((sgroup + i)->linkarrays) {
				free((sgroup + i)->linkarrays);
				(sgroup + i)->linkarrays = NULL;
			}
		}
	}
	if (gsinfo->ginfo) {
		free(gsinfo->ginfo);
		gsinfo->ginfo = NULL;
	}
}

static int show_try_process(void* hd)
{
	int rc = 0;
	struct sgroup_all_info  gsinfo;

	memset(&gsinfo, 0 , sizeof(struct sgroup_all_info));
	rc = sgfplib_get_try_group(hd, &gsinfo);
	if (rc) {
		printf("get try groupinfo failed!\n");
		return -1;
	}
	show_sgroupinfo(&gsinfo);
	clear_gsinfo(&gsinfo);
	return 0;
}

static int show_set_process(void* hd)
{
	int rc = 0;
	struct sgroup_all_info gsinfo;

	memset(&gsinfo, 0 , sizeof(struct sgroup_all_info));
	rc = sgfplib_get_group(hd, &gsinfo);
	if (rc) {
		printf("get try groupinfo failed!\n");
		return -1;
	}
	show_sgroupinfo(&gsinfo);
	clear_gsinfo(&gsinfo);
	return 0;
}

static int get_ge_state(void *hd)
{
	int rc = 0;
	int i = 0;
	char buff[1028];
	struct bd_ge_stat gestat;

	memset(&gestat, 0, sizeof(struct bd_ge_stat));
	if (g_ge_stat_clear) {
		gestat.clear = 1;
	}
	rc = sgfplib_get_hdlc_ge_stat(hd, &gestat);
	if (rc) {
		printf("get ge info failed!\n");
		return -1;
	}

	printf("        GE       tran   tran_clr       hdlc   hdlc_clr\n");
	for (i = 0; i < GE_NUM_MAX; i++) {
		sprintf(buff, "%10d %10u %10u %10u %10u",
			gestat.gstat[i].ge,	gestat.gstat[i].tran, gestat.gstat[i].tran_clr,
			gestat.gstat[i].hdlc, gestat.gstat[i].hdlc_clr);
		printf("%s\n",buff);
	}

	return 0;
}


static int show_hdlc_chstate(struct hdlc_port_stat *pchstat)
{
	int i = 0;
	int type_index = 0;
	int port_index = 0;
	unsigned int num[4] = {};
	char ctype[3][8] = {"1*64k", "31*64k", "32*64k"};
	char buf_base[512] = "";
	char buf[1024] = "";

	if (pchstat == NULL) {
		printf("hdlc port stat is null\n");
		return -1;
	}
	memset(num, 0, sizeof(num));
	switch(g_chstat_mask) {
		case GET_CHSTAT_MASK:
			printf(" NO.  match_num port   type         e1         ts    ok_pkgs\n");
			break;
		case GET_CHSTAT_LOCAL_MASK:
		case GET_CHSTAT_CFG_MASK:
			printf(" NO.  match_num port   type         e1         ts\n");
			break;
		case GET_CHSTAT_DB_MASK:
			printf(" NO.  match_num port   type         e1         ts"\
					"     crc_ok         7e    crc_err\n");
			break;
		default:
			printf("no such hdlc mask\n");
			return -1;
	}
	for (i = 0; i < pchstat->num_chstats; i++) {
		type_index = pchstat->chstat[i].type;
		if (type_index < 1 || type_index > 3) {
			printf("no such type!");
			return -1;
		}
		port_index = pchstat->chstat[i].port;
		sprintf(buf_base, "%4u %10u  %3u %6s %10u %10u",
				++num[port_index], pchstat->chstat[i].match_num,
				pchstat->chstat[i].port+1, ctype[type_index-1],
				pchstat->chstat[i].e1+1, pchstat->chstat[i].ts);

		switch(g_chstat_mask) {
				case GET_CHSTAT_MASK:
					sprintf(buf,"%s %10u", buf_base, pchstat->chstat[i].ok_pkts);						
					break;
				case GET_CHSTAT_LOCAL_MASK:
				case GET_CHSTAT_CFG_MASK:
					sprintf(buf,"%s", buf_base);
					break;
				case GET_CHSTAT_DB_MASK:
					sprintf(buf,"%s %10u %10u %10u", buf_base, pchstat->chstat[i].ok_pkts,
							pchstat->chstat[i].n_7e, pchstat->chstat[i].err_pkts);						
					break;
				default:
					printf("no such hdlc mask\n");
					return -1;
			}

		if (((g_hdlc_port != HDLC_PORTMAX) && (g_hdlc_port != 0)) ||
				(g_hdlc_port_last == 1)) {
			if (g_hdlc_port == (pchstat->chstat[i].port+1)) {
				printf("%s\n", buf);
			}
		}
		else {
			printf("%s\n", buf);
		}
	}
	return 0;
}

static int get_hdlc_chstate(void *hd)
{
	int rc = 0;
	struct hdlc_port_stat pchstat; 

	memset(&pchstat, 0, sizeof(struct hdlc_port_stat));
	rc = sgfplib_get_hdlc_chstat(hd, g_chstat_mask, &pchstat);
	if (rc < 0) {
		printf("sgfp get hdlc chstat failed!\n");
		return -1;
	}
	rc = show_hdlc_chstate(&pchstat);
	if (rc < 0) {
		printf("show hdlc chstat failed!\n");
		return -1;
	}
	
	return 0;
}

static int set_hdlc_frame_len(void *hd)
{
	int rc = 0;
	FILE *fp = NULL;
	char cend[2] = { 0x0a, 0};
	char buf[1024] = "";
	struct hdlc_cfg phcfg;

	memset(&phcfg , 0, sizeof(struct hdlc_cfg));
	phcfg.mask = HDLC_CFG_LEN;
	phcfg.len_max = g_frame_max;	
	phcfg.len_min = g_frame_min;	
	rc = sgfplib_set_hdlc_cfg(hd, &phcfg);
	if (rc < 0) {
		printf("sgfp set frame len failed !\n");
		return -1;
	}

	fp = fopen(frame_cfg_name, "w");
    if (fp == NULL) {
        LOGERROR("file %s openning failed.", frame_cfg_name);
        return -1;
    }
	sprintf(buf, "[HDLC_CFG]%sfr_len_max = %u%s"\
			"fr_len_min = %u%sscan_mode = %u%s", cend,
			g_frame_max, cend, g_frame_min, cend,
			0, cend);
	fwrite(buf, strlen(buf), 1, fp);
	fclose(fp);

	return 0;
}

static int read_sgfp_cfg(struct sgfpcfg* pcfgvalues)
{
	unsigned long cfghd;
	char value[128] = "";

	cfghd = CfgInitialize((char*)frame_cfg_name);
	if (cfghd == 0ul) {
		printf("cfghd error");
		return -1;
	}
	if (CfgGetValue(cfghd, (char *)"HDLC_CFG",
				 (char *)"fr_len_max", value, 1, 1) == -1) {
		LOGERROR("Loading id failed.");
		return -1;
	}
	pcfgvalues->fr_len_max = atoi(value);
	if (CfgGetValue(cfghd, (char *)"HDLC_CFG",
				 (char *)"fr_len_min", value, 1, 1) == -1) {
		LOGERROR("Loading id failed.");
		return -1;
	}
	pcfgvalues->fr_len_min = atoi(value);	
	if (CfgGetValue(cfghd, (char *)"HDLC_CFG",
				 (char *)"scan_mode", value, 1, 1) == -1) {
		LOGERROR("Loading id failed.");
		return -1;
	}
	pcfgvalues->scan_mode = atoi(value);	
	CfgInvalidate(cfghd);
	return 0;
}

static int show_frame_len_mode(void)
{
	int rc = 0;
	struct sgfpcfg cfgvalues;

	rc = read_sgfp_cfg(&cfgvalues); 
	if (rc < 0) {
		printf("read config failed\n");
		return -1;
	}

	if (cmd_type == SHOW_FRAME_LEN) {
		printf("max_len:%d min_len:%d\n", cfgvalues.fr_len_max, cfgvalues.fr_len_min);
	}
	else if (cmd_type == SHOW_SCAN_MODE) {
		printf("1:local mode 2:config mode\n");
		printf("scan_mode:%d\n", cfgvalues.scan_mode);
	}
	return 0;
}


static int cfg_hdlc_scan_mode(void *hd)
{
	int rc = 0;
	FILE *fp = NULL;
	struct hdlc_cfg phcfg;
	struct sgfpcfg cfgvalues;
	char value[1028] = "";
	char cend[2] = { 0x0a, 0};

	memset(&phcfg , 0, sizeof(struct hdlc_cfg));
	

	phcfg.mask = HDLC_CFG_SCAN_MODE;
	if (g_scan_mode_mask == LOCAL) {
		phcfg.scan_mode = LOCAL;
	}
	else if (g_scan_mode_mask == CONFIG) {
		phcfg.scan_mode = CONFIG;
	}
	rc = sgfplib_set_hdlc_cfg(hd, &phcfg);
	if (rc < 0) {
		printf("scan mode sgfp set frame len failed !\n");
		return -1;
	}

	rc = read_sgfp_cfg(&cfgvalues); 
	if (rc < 0) {
		printf("read config failed\n");
		return -1;
	}
	fp = fopen(frame_cfg_name, "w");
    if (fp == NULL) {
        LOGERROR("file %s openning failed.", frame_cfg_name);
        return -1;
    }
	sprintf(value, "[HDLC_CFG]%sfr_len_max = %u%s"\
			"fr_len_min = %u%sscan_mode = %u%s", cend,
			cfgvalues.fr_len_max, cend, cfgvalues.fr_len_min, cend,
			g_scan_mode_mask, cend);
	fwrite(value, strlen(value), 1, fp);
	fclose(fp);

	return 0;
}

static int set_selnum(void *hd)
{
	int rc = 0;
	rc = sgfplib_set_spu_selnum(hd,selnum);
	if (rc != 0) {
		printf("sgfplib_set_spu_selnum failed\n");
		return -1;
	}
	printf("set spu selnum is %d\n",selnum);
	return 0;
}

static int get_selnum(void *hd)
{
	int rc = 0;
	value = 0;

	rc = sgfplib_get_spu_selnum(hd, &value);
	if (rc != 0) {
		printf("sgfplib_get_spu_selnum failed\n");
		return -1;
	}
	printf("get spu selnum is %d\n",value);
	return 0;
}

static int forward_enable(void *hd)
{
	int rc = 0;
	
	rc = sgfplib_set_spu_forward_enable(hd);
	if (rc != 0) {
		printf("sgfplib_set_spu_forward_enable failed\n");
		return -1;
	}
	printf("set forward enable\n");
	return 0;
}

static int forward_disable(void *hd)
{
	int rc = 0;

	rc = sgfplib_set_spu_forward_disable(hd);
	if (rc != 0) {
		printf("sgfplib_set_spu_forward_disable failed\n");
		return -1;
	}
	printf("set forward  disable\n");
	return 0;
}

static int forward_rule(void *hd)
{
	int rc = 0;

	rc = sgfplib_set_spu_forward_rule(hd, rul);
	if (rc != 0) {
		printf("sgfplib_set_spu_forward_rule failed\n");
		return -1;
	}
	printf("set src port is %d,set src channel is %d,\nset dst port is %d,set dst selnum  is %d.\n",rul.src_port,rul.src_channel,rul.dst_port,rul.dst_selnum);
	return 0;
}

static int forward_channel(void *hd)
{
	int rc = 0;

	rc = sgfplib_set_spu_channel_forward(hd, chan_flag);
	if (rc != 0) {
		printf("sgfplib_set_spu_channel_forward failed\n");
		return -1;
	}
	if (0 == chan_flag.flag){
	printf("set channel is %d, channel disable\n",chan_flag.chan);
	}
	else{
	printf("set channel is %d, channel enable\n",chan_flag.chan);
	}
	return 0;
}

static int sgfp_run()
{
	char current_file[128] = "/tmp/sgfp_0.cfg";
	char history_file[128] = "/tmp/sgfp_history.cfg";
	switch(cmd_type)
	{
		case GET:
			if (get_process(ghd) < 0) {
				printf("get process failed!\n");
				return -1;
			}
			break;
		case TRY:
			if (try_process(ghd, cfgfile, try_cout) < 0) {
				printf("try process failed!\n");
				return -1;
			}
			break;
		case SET:
			if (set_process(ghd, cfgfile) < 0) {
				printf("set process failed!\n");
				return -1;
			}
			break;
		case SHOW_TRY:
			if (show_try_process(ghd) < 0) {
				printf("try process failed!\n");
				return -1;
			}
			break;
		case SHOW_SET:
			if (show_set_process(ghd) < 0) {
				printf("set process failed!\n");
				return -1;
			}
			break;
		case STATE:
			if (get_state(ghd) < 0) {
				printf("get state failed!\n");
				return -1;
			}
			break;
		case CURRENT:
			if (show_file((char *)current_file) < 0) {
				printf("show current study failed!\n");
				return -1;
			}
			break;
		case HISTORY:
			if (show_file((char *)history_file) < 0) {
				printf("show history study failed!\n");
				return -1;
			}
			break;
		case GE_STAT:
			if (get_ge_state(ghd) < 0) {
				printf("get ge state failed!\n");
				return -1;
			}
			break;
		case HDLC_SCHAN:
		case HDLC_SCAN:
		case HDLC_DB_STATUS:
			if (get_hdlc_chstate(ghd) < 0) {
				printf("get hdlc chstate failed!\n");
				return -1;
			}
			break;
		case SET_FRAME_LEN:
			if (set_hdlc_frame_len(ghd) < 0) {
				printf("set hdlc frame len failed !\n");
				return -1;
			}
			break;
		case SHOW_FRAME_LEN:
			if (show_frame_len_mode() < 0) {
				printf("show hdlc frame len failed!\n");
				return -1;
			}
			break;
		case SHOW_SCAN_MODE:
			if (show_frame_len_mode() < 0) {
				printf("show hdlc scan mode failed !\n");
				return -1;
			}
			break;
		case CFG_SCAN_MODE:
			if (cfg_hdlc_scan_mode(ghd) < 0) {
				printf("config hdlc scan mode failed !\n");
				return -1;
			}
			break;
		case CFG_SELNUM:
			if (set_selnum(ghd) < 0) {
				printf("set selnum failed!\n");
				return -1;
			}
			break;
		case SHOW_SELNUM:
			if (get_selnum(ghd) < 0) {
				printf("get selnum failed!\n");
				return -1;
			}
			break;
		case CFG_FORWARD_ENABLE:
			if (forward_enable(ghd) < 0) {
				printf("forward_enable failed!\n");
				return -1;
			}
			break;
		case CFG_FORWARD_DISABLE:
			if (forward_disable(ghd) < 0) {
				printf("forward_disable failed!\n");
				return -1;
			}
			break;
		case CFG_FORWARD_RULE:
			if (forward_rule(ghd) < 0) {
				printf("forward_rule failed!\n");
				return -1;
			}
			break;
		case CFG_CHANNEL_FORWARD:
			if (forward_channel(ghd) < 0) {
				printf("forward_channel failed!\n");
				return -1;
			}
			break;
        case GET_H:
            if (get_slink_h(ghd) < 0) {
                printf("get slink h failed\n");
            }
            break;

		default:
			LOGERROR("no such cmdtype!\n");
			return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
    int rc = 0;

    rc = init_env();
    if (rc < 0) {
		printf("init_env error\n");
        sgfp_show_usage(argv[0]);
        exit_env();
        return rc;
    }

    rc = sgfp_parse_args(argc, argv); 
    if (rc < 0) {
		printf("parse args error\n");
        sgfp_show_usage(argv[0]);
        exit_env();
        return rc;
    }

	ghd = sgfplib_open(g_port_info_fpga);
    if (ghd == NULL) {
        if (g_port_info_fpga == 1) {
            ghd = sgfplib_open(0);
            if (ghd == NULL) {
                rc = -1;
                return rc;
            }
        } else {
            rc = -1;
            return rc;
        }
    }

    rc = sgfp_run(); 
    if (rc < 0) {
		printf("run error\n");
        sgfp_show_usage(argv[0]);
        exit_env();
        return rc;
    }

    exit_env();

    return 0;
}
