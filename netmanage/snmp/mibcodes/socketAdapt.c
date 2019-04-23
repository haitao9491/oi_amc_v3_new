
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "os.h"
#include "aplog.h"
#include "cconfig.h"
#include "cthread.h"
#include "nm_typdef.h"
#include "pkt.h"
#include "nmpkt.h"
#include "nm_glb.h"
#include "adapter_cs.h"
#include "adapter.h"
#include "seqlst.h"
#include "utimer.h"
#include "boardsInfoTable.h"
#include "e1PhyMibPortFlowStatTable.h"
#include "e1PhyMibPortInfoStatTable.h"
#include "e1PhyMibPortInfoAlarmTable.h"
#include "sdhPhyMibPortFlowStatTable.h"
#include "sdhPhyMibPortInfoAlarmTable.h"
#include "switchMibPortInfoStatTable.h"
#include "socketAdapt.h"

#define NMSERVER_IP	    "192.168.10.10"
#define NMSERVER_PORT	28002
#define SNMP_TIMEOUT    5
#define SNMP_RX_BUFLEN  20480
#define SNMP_TX_BUFLEN  8192
#define SOCK_BPKT_LEN   8192

struct socketAdapt_handle {
	char	       ip[16];
	unsigned short port;
	unsigned int   interval;

	/* internal */
	void *adap;

	void *rxthrd;

	/* timer */
	void *uthd;
	void *timer;

	int count;
};

struct socketAdapt_handle ctl;
static int running = 1;

static int ap_is_running()
{
	return running;
}

static void ap_is_stop()
{
	running = 0;
}

int socketAdapt_send_cmd_req(uint8_t module, uint8_t cmd, uint8_t *data, int dlen)
{
	unsigned char buf[SNMP_TX_BUFLEN];
	pkt_hdr *ph;
	nmpkt_hdr *nmph = NULL;
	uint8_t *p = NULL;
	int len = 0;
	unsigned short seq = 0;

	/* boardinfo:getstatus*/
	memset(buf, 0, sizeof(buf));
	ph = (pkt_hdr *)buf;
	nmph = (nmpkt_hdr *)(buf + sizeof(pkt_hdr));
	nmpkthdr_slow_set_magic(nmph);
	nmph->module = module;
	nmph->cmd = cmd;
	seq_lst_get_seqno(&seq);
	nmpkthdr_slow_set_seq(nmph, seq);
	if (data) {
		if (dlen >= (sizeof(buf) - sizeof(nmpkt_hdr) - sizeof(pkt_hdr))) {
			LOGERROR("socketAdapt_send_cmd_req: dlen %d over range.", dlen);
			return -1;
		}
		p = buf + sizeof(pkt_hdr) + sizeof(nmpkt_hdr);
		memcpy(p, data, dlen);
	}
	nmpkthdr_slow_set_dlen(nmph, dlen);
	len = dlen + sizeof(pkt_hdr) + sizeof(nmpkt_hdr);

	pkthdr_set_sync(ph);
	pkthdr_set_plen(ph, len);

	LGWRDEBUG(ph, len, "CMD_SEND: len:%d", len);

	if (adapter_write(ctl.adap, ph, len) != len) {
		LOGERROR("adapter_write: failed to send M:%uCMD%u", module, cmd);
		return -1;
	}

	return 0;
}

static void socketAdapt_timercb(unsigned int s, unsigned int ns, void *arg)
{
	struct socketAdapt_handle *ctl = (struct socketAdapt_handle *)arg;

	if (!ctl) 
		goto timer_add_again;

	ctl->count++;

	socketAdapt_send_cmd_req(NM_MODULE_GLB_INFO, NM_CMD_GET_BOARD_INFO, NULL, 0);
	boardsInfoTable_proc_send_req();

#if 0
	if ((ctl->count % 3) == 0) {
		/* clear table info */
		e1PhyMibPortInfoAlarmTable_check_and_clear();
		e1PhyMibPortFlowStatTable_check_and_clear();
		e1PhyMibPortInfoStatTable_check_and_clear();
		sdhPhyMibPortInfoAlarmTable_check_and_clear();
		sdhPhyMibPortFlowStatTable_check_and_clear();
		switchMibPortInfoStatTable_check_and_clear();
	}
#endif

timer_add_again:
	ctl->timer = utimer_add_by_offset(ctl->uthd, SNMP_TIMEOUT, 0, ctl);
}

#if 0
static void socketAdapt_proc_timeout(void)
{
	unsigned int ip;
	int port;
	unsigned short seq;
	int rc = 0;

	rc = seq_lst_check_timeout(&ip, &port, &seq);
	if (rc == 1) {
		boardsInfoTable_clear();
		e1PhyMibPortInfoAlarmTable_clear();
		sdhPhyMibPortInfoAlarmTable_clear();
		switchMibPortInfoStatTable_clear(); 
	}
}
#endif

int socketAdapt_pkt_verify(void *buf, int size)
{
	nmpkt_hdr *nmph = NULL;
	int len;

	if (!buf)
		return -1;

	if (size < sizeof(nmpkt_hdr))
		return 0;

	nmph = (nmpkt_hdr *)buf;

	if (!nmpkthdr_valid_magic(nmph)) {
		LOGERROR("%s: nmpkthdr magic is unvalid", __func__);
		return -1;
	}

	len = sizeof(nmpkt_hdr) + nmpkthdr_get_dlen(nmph);

	if (size < len) {
		LOGERROR("nmpkt size %d is lower nmpkt real len %d", size, len);
		return 0;
	}

	return len;
}

int socketAdapt_proc_glbinfo_module(unsigned char *data, int len)
{
	struct nmboard_info *bdinfo;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *p = data + sizeof(*nmph);
	int cnt, i;

	if (nmph->cmd == NM_CMD_GET_BOARD_INFO) {
		boardsInfoTable_reset_valid();
		cnt = p[0];
		bdinfo = (struct nmboard_info *)(p + 1);
		for (i = 0; i < cnt; i++) {
			boardsInfoTable_update(bdinfo);
			bdinfo++;
		}
		boardsInfoTable_delete_invalid();
	}
	else {
		return -1;
	}

	return 0;
}

int socketAdapt_proc_e1phy_module(unsigned char *data, int len)
{
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	struct nm_lkaddr_info po;
	unsigned char *p = data + sizeof(*nmph);

	if (nmph->cmd == NM_CMD_GET_E1PHY_STATUS) {
		po.rack    = p[0];
		po.shelf   = p[1];
		po.slot    = p[2];
		po.subslot = p[3];
		e1PhyMibPortInfoAlarmTable_update(po, &p[4]);
	}
#if 0
	else if (nmph->cmd == NM_CMD_GET_FPGA_STAT) {
		po.rack    = p[0];
		po.shelf   = p[1];
		po.slot    = p[2];
		po.subslot = p[3];
		e1PhyMibPortInfoStatTable_update(po, bdtype, &p[5]);
	}
	else if (nmph->cmd == NM_CMD_GET_FPGA_FLOW_STAT) {
		po.rack    = p[0];
		po.shelf   = p[1];
		po.slot    = p[2];
		po.subslot = p[3];
		e1PhyMibPortFlowStatTable_update(po, bdtype, &p[5]);
	}
#endif
	else {
		return -1;
	}

	return 0;
}

int socketAdapt_proc_fpga_module(unsigned char *data, int len)
{
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	struct nm_lkaddr_info po;
	int bdtype;
	unsigned char *p = data + sizeof(*nmph);

	if (nmph->cmd == NM_CMD_GET_FPGA_STATUS) {
		po.rack    = p[0];
		po.shelf   = p[1];
		po.slot    = p[2];
		po.subslot = p[3];
		bdtype     = p[4];
		sdhPhyMibPortInfoAlarmTable_update(po, bdtype, &p[5]);
	}
	else if (nmph->cmd == NM_CMD_GET_FPGA_STAT) {
		po.rack    = p[0];
		po.shelf   = p[1];
		po.slot    = p[2];
		po.subslot = p[3];
		bdtype     = p[4];
		sdhPhyMibPortFlowStatTable_update(po, bdtype, &p[5]);
	}
	else if (nmph->cmd == NM_CMD_GET_FPGA_FLOW_STAT) {
	}
	else {
		return -1;
	}

	return 0;
}

int socketAdapt_proc_switch_module(unsigned char *data, int len)
{
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	struct nm_lkaddr_info po;
	int cnt;
	unsigned char *p = data + sizeof(*nmph);

	if (nmph->cmd == NM_CMD_GET_SW_PORT_STATUS) {
		po.rack    = p[0];
		po.shelf   = p[1];
		po.slot    = p[2];
		po.subslot = p[3];
		cnt        = p[4];

		switchMibPortInfoStatTable_update(po, cnt, &p[5]);
	}
	else if (nmph->cmd == NM_CMD_SET_SW_PORT_AUTONEG) {
	}
	else if (nmph->cmd == NM_CMD_SET_SW_PORT_PWRDN) {
	}
	else {
		return -1;
	}

	return 0;
}

int socketAdapt_rx_process(unsigned char *data, int len)
{
	nmpkt_hdr *nmph;
	int dlen = 0;

	pkt_hdr *ph = (pkt_hdr *)data;
	unsigned char *p = data + sizeof(pkt_hdr);
	nmph = (nmpkt_hdr *)p;

	if (nmph->module == NM_MODULE_GLB_INFO) {
		dlen = socketAdapt_proc_glbinfo_module(p, len - sizeof(pkt_hdr));
		if (dlen <= 0) {
			return dlen;
		}
	}
	else if (nmph->module == NM_MODULE_E1PHY) {
		dlen = socketAdapt_proc_e1phy_module(p, len - sizeof(pkt_hdr));
		if (dlen <= 0) {
			return dlen;
		}
	}
#if 0
	else if (nmph->module == NM_MODULE_FPGA) {
		dlen = socketAdapt_proc_fpga_module(p, len - sizeof(pkt_hdr));
		if (dlen <= 0) {
			return dlen;
		}
	}
#endif
	else if (nmph->module == NM_MODULE_SWITCH) {
		dlen = socketAdapt_proc_switch_module(p, len - sizeof(pkt_hdr));
		if (dlen <= 0) {
			return dlen;
		}
	}
	else {
		LOGERROR("socketAdapt_rx_process: input module 0x%x", nmph->module);
		return -1;
	}

	nmpkthdr_set_dlen(nmph, dlen);
	len = dlen + sizeof(*nmph);
	pkthdr_set_sync(ph);
	pkthdr_set_dlen(ph, len);

	LGWRDEBUG(ph, pkthdr_get_plen(ph), "socketAdapt rx RESPONSE");

	if (adapter_write(ctl.adap, ph, pkthdr_get_plen(ph)) != pkthdr_get_plen(ph)) {
		LOGERROR("socketAdapt(rx_process): failed to send: %02x", nmph->module);
		return -1;
	}

	return 0;
}

void *socketAdapt_do_rx_proc(void *arg)
{
	pkt_hdr *ph;

	LOG("socketAdapt_do_rx_proc: enter..");
	while (ap_is_running()) {

		utimer_run(ctl.uthd, 0, 0);

		ph = (pkt_hdr *)adapter_read(ctl.adap);
		if (ph == NULL) {
			usleep(1000);
			continue;
		}

		LGWRDEBUG(ph, pkthdr_get_plen(ph), "rx_proc: recved: dlen %d", pkthdr_get_dlen(ph));

		socketAdapt_rx_process((unsigned char *)ph, pkthdr_get_plen(ph));

		free(ph);
	}
	
	return NULL;
}

int init_socketAdapt(void)
{
	int    rc = 0;
	char cfgstr[128];

	/* init and set default value */
	memset(&ctl, 0, sizeof(ctl));
	strcpy(ctl.ip, NMSERVER_IP);
	ctl.port = NMSERVER_PORT;
	ctl.interval = SNMP_TIMEOUT;

	/* seq lst init */
	seq_lst_init();

	/* adap */
	memset(cfgstr, 0, sizeof(cfgstr));
	sprintf(cfgstr, "[SNMPAgent.Adapter.ClientSocket]\nserver = %s,%d,10240,1024,noheartbeat,payload\n", ctl.ip, ctl.port);
	ctl.adap = adapter_register_cs_cfgstr_c(cfgstr, 
			"SNMPAgent.Adapter.ClientSocket", ap_is_running, socketAdapt_pkt_verify);
	if (ctl.adap == NULL) {
		LOGERROR("%s: failed to register snmp adapter.", __func__);
		return -1;
	}
	LOG("registered snmp agent client to %s finished.", ctl.ip);

	/* Other initialization or openning */
	rc = adapter_open(ctl.adap);
	if (rc == -1) {
		LOGERROR("%s: adapter_open failed", __func__);
		goto socketAdapt_init_failed;
	}

	/* create thread */
	ctl.rxthrd = thread_open(socketAdapt_do_rx_proc, &ctl);
	if (ctl.rxthrd == NULL) {
		LOGERROR("%s: failed to create rx thread", __func__);
		goto socketAdapt_init_failed;
	}
	LOG("rx thread create success..");

	ctl.uthd = utimer_open(UTIMER_INTERNAL_TIMESOURCE, socketAdapt_timercb);
	if (ctl.uthd == NULL) {
		LOGERROR("socketAdapt(utimer_open): failed to open timer.");
		goto socketAdapt_init_failed;
	}
	utimer_run(ctl.uthd, 0, 0);
	LOG("timecb opened..");

	/* init */
	ctl.timer = utimer_add_by_offset(ctl.uthd, SNMP_TIMEOUT, 0, &ctl);

	return 0;

socketAdapt_init_failed:
	if (ctl.uthd)
		utimer_close(ctl.uthd);
	if (ctl.rxthrd)
		thread_close(ctl.rxthrd);
	if (ctl.adap)
		adapter_close(ctl.adap);

	seq_lst_exit();

	return -1;
}

void exit_socketAdapt(void)
{
	ap_is_stop();

	if (ctl.uthd)
		utimer_close(ctl.uthd);
	if (ctl.rxthrd)
		thread_close(ctl.rxthrd);
	if (ctl.adap)
		adapter_close(ctl.adap);

	seq_lst_exit();
}


