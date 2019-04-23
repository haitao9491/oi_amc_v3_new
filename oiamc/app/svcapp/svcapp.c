#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "fpgamif.h"

static void usage(char *argv)
{
    printf("      show bdinfo     : show fpga version info.\n");
    printf("      show smac       : show info of src mac.\n");
    printf("      show sip        : show info of src IP.\n");
    printf("      show dip        : show info of dst IP.\n");
    printf("      show sport      : show info of src port\n");
    printf("      show dport1     : show info of dst port1.\n");
    printf("      show dport2     : show info of dst port2.\n");
    printf("      show devid      : show info of dev ID.\n");
    printf("      show silence    : show info of silence.\n");
    printf("      show indexlimit : show info of indexlimit.\n");
    printf("      show sel        : show info of fpga sel.\n");
    printf("      show phy-stat   : show info of phy stat.\n");
    printf("      show chan-stat  : show info of chan stat.\n");
    printf("      show e1-status <1-4> \
                                  : show info of one port e1-status.\n");
	
    printf("      cfg bdstart                : set board start .\n");
    printf("      cfg smac xx:xx:xx:xx:xx:xx : set src mac, Ex: 10:10:10:20:20:20 .\n");
    printf("      cfg sip <ipaddr>           : set src IP; Ex: 192.168.10.10.\n");
    printf("      cfg dip <ipaddr>           : set dst IP; Ex: 192.168.10.253.\n");
    printf("      cfg sport  <0-65535>       : set src port.\n");
    printf("      cfg dport1 <0-65535>       : set dst port1.\n");
    printf("      cfg dport2 <0-65535>       : set dst port2.\n");
    printf("      cfg devid  <0-255>         : set dev ID.\n");
    printf("      cfg rel             <0-8191>   : set rel event.\n");
    printf("      cfg clear index     <0-8191>   : set clear index.\n");
    printf("      cfg indexlimit      <0-8191>   : set indexlimit.\n");
    printf("      cfg silence	<0-0xff>-<0-0xff> <0-0xff>-<0-0xff> : set silence range.\n");
    printf("      cfg reset pointer          : set reset point to fpga.\n");
    printf("      cfg sel <0-255>            : set sel value of fpga.\n");
    printf("      cfg voice-chan <start|stop> [port] [en] [ts] : \
                                             : set start|stop one channel for transfer voice. \n");
}

int svcapp_set_smac(char *str, int len, char *re)
{
	int rc = 0;
	int i = 0;
	struct src_mac smac;
	if (len != 17) {
		printf("parse src mac error\n");
		return -1;
	}
	for (i = 0; i < len; i += 3) {
		if (i < 15) {
			if (!isxdigit(str[i]) || !isxdigit(str[i + 1]) || (str[i + 2] != ':')){
				printf("parse src mac error\n");
				return -1;
			}
		}
	}
	if (!str) {
		printf("please input src mac\n");
		return -1;
	}
	memset(&smac, 0, sizeof(struct src_mac));
	smac.mac0 = (char)strtol(str, (char **)NULL, 16);
	smac.mac1 = (char)strtol(str + 3, (char **)NULL, 16);
	smac.mac2 = (char)strtol(str + 6, (char **)NULL, 16);
	smac.mac3 = (char)strtol(str + 9, (char **)NULL, 16);
	smac.mac4 = (char)strtol(str + 12, (char **)NULL, 16);
	smac.mac5 = (char)strtol(str + 15, (char **)NULL, 16);
	
	if((rc = fpgamif_set_smac(NULL, &smac)) < 0){
		printf("fpgamif set smac failed\n");
		return -1;
	}
	printf("set src mac: %02x:%02x:%02x:%02x:%02x:%02x\n", smac.mac0, smac.mac1, smac.mac2, smac.mac3, smac.mac4, smac.mac5);
	return 0;
}

int svcapp_set_sip(char *str, int len, char *re)
{
	int rc = 0;
	int ip_a = 0;
	struct src_IP sip;
	if (!str) {
		printf("please input src ip\n");
		return -1;
	}
	ip_a = inet_addr(str);
	memset(&sip, 0, sizeof(struct src_IP));
	sip.ip_0 = (ip_a >> 24) & 0xFF;
	sip.ip_1 = (ip_a >> 16) & 0xFF;
	sip.ip_2 = (ip_a >> 8) & 0xFF;
	sip.ip_3 = (ip_a >> 0) & 0xFF;
	if((rc = fpgamif_set_sip(NULL, &sip)) < 0){
		printf("fpgamif set src IP failed\n");
		return -1;
	}
	printf("set src ip: %d.%d.%d.%d\n", sip.ip_0, sip.ip_1, sip.ip_2, sip.ip_3);
	return 0;
}

int svcapp_set_dip(char *str, int len, char *re)
{
	int rc = 0;
	int ip_a = 0;
	struct dst_IP dip;
	if (!str) {
		printf("please input dst ip\n");
		return -1;
	}
	ip_a = inet_addr(str);
	memset(&dip, 0, sizeof(struct dst_IP));
	dip.ip_0 = (ip_a >> 24) & 0xFF;
	dip.ip_1 = (ip_a >> 16) & 0xFF;
	dip.ip_2 = (ip_a >> 8) & 0xFF;
	dip.ip_3 = (ip_a >> 0) & 0xFF;
	
	if((rc = fpgamif_set_dip(NULL, &dip)) < 0){
		printf("fpgamif set dst IP failed\n");
		return -1;
	}
	printf("set dst ip: %d.%d.%d.%d\n", dip.ip_0, dip.ip_1, dip.ip_2, dip.ip_3);
	return 0;
}

int svcapp_set_sport(char *str, int len, char *re)
{
	int rc = 0;
	int val = 0;
	struct src_port sport;
	if (!str) {
		printf("please input src port\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 || val > 65535){
		printf("port out of range\n");
		return -1;
	}
	memset(&sport, 0, sizeof(struct src_port));
	sport.port_h = (unsigned char)((val >> 8) & 0xff);
	sport.port_l = (unsigned char)((val >> 0) & 0xff);

	if((rc = fpgamif_set_Sport(NULL, &sport)) < 0){
		printf("fpgamif set src port failed\n");
		return -1;
	}
	printf("set src port: %d\n", val);
	return 0;
}

int svcapp_set_dport1(char *str, int len, char *re)
{
	int rc = 0;
	int val = 0;
	struct dst_port dport1;
	if (!str) {
		printf("please input dst port1\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 || val > 65535){
		printf("port out of range\n");
		return -1;
	}
	memset(&dport1, 0, sizeof(struct dst_port));
	dport1.port_h = (unsigned char)((val >> 8) & 0xff);
	dport1.port_l = (unsigned char)((val >> 0) & 0xff);

	if((rc = fpgamif_set_Dport1(NULL, &dport1)) < 0){
		printf("fpgamif set dst port1 failed\n");
		return -1;
	}
	printf("set dst port1: %d\n", val);
	return 0;
}

int svcapp_set_dport2(char *str, int len, char *re)
{
	int rc = 0;
	int val = 0;
	struct dst_port dport2;
	if (!str) {
		printf("please input dst port2\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 || val > 65535){
		printf("port out of range\n");
		return -1;
	}
	memset(&dport2, 0, sizeof(struct dst_port));
	dport2.port_h = (unsigned char)((val >> 8) & 0xff);
	dport2.port_l = (unsigned char)((val >> 0) & 0xff);
	if((rc = fpgamif_set_Dport2(NULL, &dport2)) < 0){
		printf("fpgamif set dst port2 failed\n");
		return -1;
	}
	printf("set dst port2: %d\n", val);
	return 0;
}

int svcapp_set_devid(char *str, int len, char *re)
{
	int rc = 0;
	int val = 0;
	unsigned int devid = 0;
	if (!str) {
		printf("please input dev-id\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 || val > 255){
		printf("devid out of range\n");
		return -1;
	}
	devid = (unsigned int)val;
	if((rc = fpgamif_set_dev_id(NULL, &devid)) < 0){
		printf("fpgamif set devid failed\n");
		return -1;
	}
	printf("set devid: %d\n", devid);
	return 0;
}

int svcapp_set_clear_index(char *str, int len, char *re)
{
	int rc = 0;
	int val = 0;
	unsigned int index = 0;
	if (!str) {
		printf("please input index\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 || val > 8191){
		printf("index out of range\n");
		return -1;
	}
	index = (unsigned int)val;
	if((rc = fpgamif_set_fpga_clear_index(NULL, &index)) < 0){
		printf("fpgamif set fpga clear index failed\n");
		return -1;
	}
	printf("clear index: %d succeed\n", index);
	return 0;
}

int svcapp_set_silence(int argc, char *argv[])
{
    int rc = 0;
    int smin0 = 0x00;
    int smax0 = 0x00;
    int smin1 = 0x00;
    int smax1 = 0x00;
	if (argc != 2){
		printf("unknown parameter\n");
		return -1;
	}
	struct fpga_silence slient;
	memset(&slient, 0, sizeof(struct fpga_silence));
	sscanf(argv[0], "%x-%x", &smin0, &smax0);
	sscanf(argv[1], "%x-%x", &smin1, &smax1);
	if (!smin0 || !smax0 || !smin1 || !smax1) {
		printf("smin: %x smax: %x smin1: %x smax1: %x", smin0, smax0, smin1, smax1);
		return -1;
	}
	if (smin0 < 0 || smin0 > 0xff || smax0 < 0 || smax0 > 0xff || smin1 < 0 || smin1 > 0xff || smax1 < 0 || smax1 > 0xff) {
		printf("out rang smin: %x smax: %x smin1: %x smax1: %x", smin0, smax0, smin1, smax1);
		return -1;
	}
	slient.smin0 = smin0;
	slient.smax0 = smax0;
	slient.smin1 = smin1;
	slient.smax1 = smax1;
	rc = fpgamif_set_fpga_silence(NULL,&slient);
	if(rc < 0){
		printf("fpgamif set slient failed\n");
		return -1;
	}
	printf("set slient: %x - %x ,%x - %x\n", slient.smin0, slient.smax0, slient.smin1, slient.smax1);
	return 0;
}

int svcapp_set_indexlimit(int argc, char *str)
{
	int rc = 0;
	int val = 0;
	unsigned int indexlimit = 0;
	if (argc != 1){
		printf("unknown parameter\n");
		return -1;
	}
	if (!str) {
		printf("please input indexlimit\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 && val > 8191) {
		printf("indexlimit out of range\n");
		return -1;
	}
	indexlimit = (unsigned int)val;
	rc = fpgamif_set_fpga_indexlimit(NULL, &indexlimit);
	if (rc < 0) {
		printf("fpgamif set indexlimit failed\n");
		return -1;
		}
	printf("set indexlimit: %d succeed\n", val);
	return 0;
}

int svcapp_set_rel(char *str, int len, char *re)
{
	int rc = 0;
	int val = 0;
	unsigned int index = 0;
	if (!str) {
		printf("please input index\n");
		return -1;
	}
	val = atoi(str);
	if(val < 0 && val > 8191) {
		printf("index out of range\n");
		return -1;
	}
	index = (unsigned int)val;
	rc = fpgamif_set_fpga_rel(NULL, index);
	if (rc < 0) {
		printf("fpgamif set rel failed\n");
		return -1;
		}
	printf("set rel: %d\n", index);
	return 0;
}

int svcapp_get_bd_info(void)
{
	int rc = 0;
	struct verinfo ver;
	
	memset(&ver, 0, sizeof(struct verinfo));
	if((rc = get_bd_info(&ver)) < 0){
		printf("fpgamif get board info failed\n");
		return -1;
	}
	printf("version: 0x%02x, date: %02x%02x%02x%02x\n", ver.ver,
				(ver.date >> 24) & 0xff, (ver.date >> 16) & 0xff, (ver.date >> 8) & 0xff, (ver.date & 0xff));
	return 0;
}

int svcapp_get_smac(void)
{
	int rc = 0;
	struct src_mac smac;
	
	memset(&smac, 0, sizeof(struct src_mac));
	if((rc = fpgamif_get_smac(NULL, &smac)) < 0){
		printf("fpgamif set smac failed\n");
		return -1;
	}
	printf("get src mac: %02x:%02x:%02x:%02x:%02x:%02x\n", smac.mac0, smac.mac1, smac.mac2, smac.mac3, smac.mac4, smac.mac5);
	return 0;
}

int svcapp_get_sip(void)
{
	int rc = 0;
	struct src_IP sip;

	memset(&sip, 0, sizeof(struct src_IP));
	if((rc = fpgamif_get_sip(NULL, &sip)) < 0){
		printf("fpgamif get src IP failed\n");
		return -1;
	}
	printf("get src ip: %d.%d.%d.%d\n", sip.ip_0, sip.ip_1, sip.ip_2, sip.ip_3);
	return 0;
}

int svcapp_get_dip(void)
{
	int rc = 0;
	struct dst_IP dip;

	memset(&dip, 0, sizeof(struct dst_IP));
	if((rc = fpgamif_get_dip(NULL, &dip)) < 0){
		printf("fpgamif get dst IP failed\n");
		return -1;
	}
	printf("get dst ip: %d.%d.%d.%d\n", dip.ip_0, dip.ip_1, dip.ip_2, dip.ip_3);
	return 0;
}

int svcapp_get_sport(void)
{
	int rc = 0;
	unsigned int val = 0;
	struct src_port sport;

	memset(&sport, 0, sizeof(struct src_port));
	if((rc = fpgamif_get_Sport(NULL, &sport)) < 0){
		printf("fpgamif get src port failed\n");
		return -1;
	}
	val = ((sport.port_h << 8) | sport.port_l);
	printf("get src port: %d\n", val);
	return 0;
}

int svcapp_get_dport1(void)
{
	int rc = 0;
	unsigned int val = 0;
	struct dst_port dport;

	memset(&dport, 0, sizeof(struct src_port));
	if((rc = fpgamif_get_Dport1(NULL, &dport)) < 0){
		printf("fpgamif get dst port1 failed\n");
		return -1;
	}
	val = ((dport.port_h << 8) | dport.port_l);
	printf("get dst port1: %d\n", val);
	return 0;
}

int svcapp_get_dport2(void)
{
	int rc = 0;
	unsigned int val = 0;
	struct dst_port dport2;

	memset(&dport2, 0, sizeof(struct dst_port));
	if((rc = fpgamif_get_Dport2(NULL, &dport2)) < 0){
		printf("fpgamif get dst port2 failed\n");
		return -1;
	}
	val = ((dport2.port_h << 8) | dport2.port_l);
	printf("get dst port2: %d\n", val);
	return 0;
}

int svcapp_get_devid(void)
{
	int rc = 0;
	unsigned int devid;

	if((rc = fpgamif_get_dev_id(NULL, &devid)) < 0){
		printf("fpgamif get devid failed\n");
		return -1;
	}
	printf("get dev-id: %d\n", devid);
	return 0;
}

int svcapp_get_silence(void)
{
	int rc = 0;
	struct fpga_silence silence;
	
	memset(&silence, 0, sizeof(struct fpga_silence));
	if((rc = fpgamif_get_fpga_silence(NULL, &silence)) < 0){
		printf("fpgamif get silence failed\n");
		return -1;
	}
	printf("get slient: %02x - %02x , %02x - %02x\n", silence.smin0, silence.smax0, silence.smin1, silence.smax1);
	return 0;
}

int svcapp_get_indexlimit(void)
{
	int rc = 0;
	unsigned int indexlimit = 0;

	if ((rc = fpgamif_get_fpga_indexlimit(NULL, &indexlimit)) < 0){
		printf("fpgamif get in indexlimit\n");
		return -1;
	}
	printf("get indexlimit: %d\n", indexlimit);
	return 0;
}

int get_phy_stat (void)
{
    int i = 0;
    struct sdh_phy_stat stat;

    printf("Port los stm1ok  b1    b2    b3    e1-cnt 64kcnt  64kfmcnt  2mcnt  2mfmcnt \n");

    for (i = 0; i < 4; i++) {
        memset(&stat, 0, sizeof(struct sdh_phy_stat));
        stat.port = i;
        if (fpgamif_get_one_port_phy_status(&stat) != 0) {
            printf("port[%d]: fpgamif_get_one_port_phy_status is failed \n", i);
            continue;
        }

        printf("%2d   %d      %d %5d %5d %5d %5d  %4d    %5d     %4d  %5d\n",
                i, (stat.status & 0x01), (stat.status & 0x02) ? 1:0, \
                stat.b1, stat.b2, stat.b3, stat.e1_cnt, stat.chcnt_64k, stat.fmcnt_64k, stat.chcnt_2m, stat.fmcnt_2m);
    }

    return 0;
}

int get_chann_stat(void)
{
    struct get_hdlc_stat_to_file stat;
    int i = 0;
    //char *str_type[] = { "64k", "2m"};
    char *str_type[] = { "2m", "64k"};

    memset(&stat, 0, sizeof(struct get_hdlc_stat_to_file));

    if (fpgamif_get_all_hdlc_channel_stat(NULL, &stat) != 0) {
        printf("fpgamif_get_all_hdlc_channel_stat\n");
        return -1;
    }

    printf("No.   type  port   e1    ts   ok_pkts  err_pkts\n");
    for (i = 0; i < stat.ch_vaild; i++) {
        printf("%4d  %3s   %2d      %2d   %2d   %5d    %5d \n", \
            stat.chstat[i].id,  str_type[stat.chstat[i].type], stat.chstat[i].port,\
            stat.chstat[i].e1, stat.chstat[i].ts, stat.chstat[i].ok_pkts, stat.chstat[i].err_pkts);
    }

    return 0;
}

int get_e1_status(int port)
{
    struct e1_status e1_s;
    int i = 0;

    memset(&e1_s, 0, sizeof(struct e1_status));
    e1_s.port = port - 1;
    if (fpgamif_get_e1_vaild(NULL, &e1_s) != 0) {
        printf("fpgamif_get_e1_vaild is failed.\n");
        return -1;
    }

    printf("Port  e1  valid  sync_err\n");
    for ( i = 0; i < FPGADRV_OIAMC_E1_MAX; i++) {
        printf("%2d    %2d   %2d     %6d\n", port, (i+1), (e1_s.vaild[i]), e1_s.sync_err[i]);
    }

    return 0;
}

int svcapp_do_show_cmd(int argc, char *argv[])
{
	int rc = 0;
	if (argc == 1) {
		if (!strcmp(argv[0], "bdinfo")){
			rc = svcapp_get_bd_info();
			if (rc != 0) {
				printf("svcapp get board info failed\n");
				return -1;
			}
		} 
		else if (!strcmp(argv[0], "smac")) { 
			rc = svcapp_get_smac();
			if (rc != 0) {
				printf("svcapp get smac failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "sip")) {
			rc = svcapp_get_sip();
			if (rc != 0) {
				printf("svcapp get src ip failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "dip")) {
			rc = svcapp_get_dip();
			if (rc != 0) {
				printf("svcapp get dst ip failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "sport")) {
			rc = svcapp_get_sport();
			if (rc != 0) {
				printf("svcapp get src port failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "dport1")) {
			rc = svcapp_get_dport1();
			if (rc != 0) {
				printf("svcapp get dst port1 failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "dport2")) {
			rc = svcapp_get_dport2();
			if (rc != 0) {
				printf("svcapp get dst port2 failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "devid")) {
			rc = svcapp_get_devid();
			if (rc != 0) {
				printf("svcapp get devid failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "silence")) {
			rc = svcapp_get_silence();
			if (rc != 0) {
				printf("svcapp get slience failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "indexlimit")) {
			rc = svcapp_get_indexlimit();
			if (rc != 0) {
				printf("svcapp get indexlimit failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "sel")) {
			int sel = 0;
			rc = fpgamif_get_sel(NULL, &sel);
			if (rc != 0) {
				printf("svcapp get sel failed\n");
				return -1;
			}
			printf("sel: %d\n", sel);
		}
		else if (!strcmp(argv[0], "phy-stat")) {
            rc = get_phy_stat();
        }
		else if (!strcmp(argv[0], "chan-stat")) {
            rc = get_chann_stat();
        }
		
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}
	else if (argc == 2) {
        printf("arg0 %s argv1 %s\n", argv[0], argv[1]);
        if (!strcmp(argv[0], "e1-status")) {
            if (!argv[1])
                return -1;

            int port = atoi(argv[1]);

            if ((port > 4) || (!port))
                return -1;

            rc = get_e1_status(port);
        }
    }
	else {
		printf("unknown parameter\n");
		return -1;
	}
	return rc;
}

int svcapp_set_voice_chan(char *argv[])
{
    struct chan_tran_cfg cfg;
    memset(&cfg, 0, sizeof(struct chan_tran_cfg));

    if (!strcmp(argv[0], "start")) {
        cfg.en = 1;
    } else if (!strcmp(argv[0], "stop")) {
        cfg.en = 0;
    } else {
        printf("unknown argv %s \n", argv[0]);
        return -1;
    }

    cfg.port = atoi(argv[1]);
    if ((cfg.port > 4) || (cfg.port < 1)) {
        printf("port %d is out range.\n", cfg.port); 
        return -1;
    }

    cfg.e1= atoi(argv[2]);
    if ((cfg.e1 > 63) || (cfg.e1 < 1)) {
        printf("e1 %d is out range.\n", cfg.e1); 
        return -1;
    }

    cfg.ts = atoi(argv[3]);
    if ((cfg.ts > 63) || (cfg.ts < 1)) {
        printf("ts %d is out range.\n", cfg.ts); 
        return -1;
    }

    if (fpgamif_set_fpga_chan_tran(&cfg) != 0) {
        return -1; 
    }

    return 0;
}

int svcapp_do_cfg_cmd (int argc, char *argv[])
{
	int rc = 0;
	if (argc == 1) {
		if (!strcmp(argv[0], "bdstart")) {
			rc = set_bd_start();
			if (rc != 0) {
				printf("svcapp set board start failed\n");
				return -1;
			}
		}
	} else if (argc == 2) {
		if (!strcmp(argv[0], "smac")) { 
			rc = svcapp_set_smac(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set smac failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "sip")) {
			rc = svcapp_set_sip(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set src ip failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "dip")) {
			rc = svcapp_set_dip(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set dst ip failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "sport")) {
			rc = svcapp_set_sport(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set src port failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "dport1")) {
			rc = svcapp_set_dport1(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set dst port1 failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "dport2")) {
			rc = svcapp_set_dport2(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set dst port2 failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "devid")) {
			rc = svcapp_set_devid(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set devid failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "indexlimit")) {
			rc = svcapp_set_indexlimit((argc-1), argv[1]);
			if (rc != 0) {
				printf("svcapp set indexlimit failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "rel")) {
			rc = svcapp_set_rel(argv[1], strlen(argv[1]), NULL);
			if (rc != 0) {
				printf("svcapp set rel failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "reset") &&
				(!strcmp(argv[1], "pointer"))) {
			rc = fpgamif_set_fpga_reset_pointer(NULL);
			if (rc != 0) {
				printf("svcapp set reset pointer failed\n");
				return -1;
			}
		}
		else if (!strcmp(argv[0], "sel")) {
			int sel = 0;
			sel = atoi(argv[1]);
			if (sel < 0 || sel > 255) {
				printf("svcapp: sel value is error.\n");
				return -1;
			}
			rc = fpgamif_set_sel(NULL, &sel);
			if (rc != 0) {
				printf("svcapp set sel failed\n");
				return -1;
			}
		}
		else {
			printf("unknown parameter\n");
			return -1;
		}
	}else if (argc == 3) {
		if ((!strcmp(argv[0], "clear")) &&
				(!strcmp(argv[1], "index"))) {
			rc = svcapp_set_clear_index(argv[2], strlen(argv[2]), NULL);
			if (rc != 0) {
				printf("svcapp set clear index failed\n");
				return -1;
			}
		}else if (!strcmp(argv[0], "silence")) {
			rc = svcapp_set_silence((argc-1), &argv[1]);
			if (rc != 0) {
				printf("svcapp set slience failed\n");
				return -1;
			}
		}else if (argv[2] == NULL) {
			printf("unknown parameter\n");
			return -1;
		}
	}else if (argc == 5) {
        if (!strcmp(argv[0], "voice-chan")) {
            rc = svcapp_set_voice_chan(&argv[1]);
        } else {
            printf("unknown parameter\n");
            return -1;
        }
	}else {
		printf("unknown parameter\n");
		return -1;
	}
	return rc;
}

int main(int argc, char **argv)
{
	void *fd;
	int rc = 0;
	if(argc <= 1){
		printf("please input args\n");
		usage(argv[0]);
		return rc;
	}
	fd = fpgamif_open();
	if (!fd) {
		printf("fpgamif_open faild\n");
		return -1;
	}

	/* Call function related to show. */
	if (!strcmp(argv[1], "show")) {
		rc = svcapp_do_show_cmd((argc - 2), &argv[2]);
	} else if (!strcmp(argv[1], "cfg")) {
		rc = svcapp_do_cfg_cmd((argc - 2), &argv[2]);
	} else {
		printf("parse args error\n");
        	usage(argv[0]);
        	return rc;
	}
	if(rc < 0){
		printf("parse args error\n");
        	usage(argv[0]);
        	return rc;
	}
	fpgamif_close(fd);
	return 0;
}
