/*
 *
 * nm_component.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "os.h"
#include "aplog.h"
#include "list.h"
#include "mutex.h"
#include "nm_typdef.h"
#include "pkt.h"
#include "nmpkt.h"
#include "phyadapt.h"
#include "nm_glb.h"
#include "nm_e1phy.h"
#include "nm_fpga.h"
#include "nm_switch.h"
#include "nm_client.h"
#include "nm_component.h"

struct nm_component_ctl {
	void *e1phy;
	void *fpga;
	void *sw;
};

void *nm_component_open(void)
{
	struct nm_component_ctl *ctl = NULL;
	
	ctl = (struct nm_component_ctl *)malloc(sizeof(*ctl));
	if (!ctl) {
		LOGERROR("%s: malloc failed!", __func__);
		return NULL;
	}

	memset(ctl, 0, sizeof(*ctl));
	ctl->e1phy = NULL;
	ctl->fpga  = NULL;
	ctl->sw    = NULL;

	return ctl;
}

int nm_component_open_comp(void *hd, unsigned int flag)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	
	if (!ctl)
		return -1;

	if (flag & NM_COMPONENT_E1PHY) {
		if (ctl->e1phy == NULL) {
			ctl->e1phy = nm_e1phy_open();
			if (!ctl->e1phy) {
				LOGERROR("%s: open e1phy handle failed!", __func__);
				return -1;
			}
		}
	}

	if (flag & NM_COMPONENT_FPGA) {
		if (ctl->fpga == NULL) {
			ctl->fpga = nm_fpga_open();
			if (!ctl->fpga) {
				LOGERROR("%s: open fpga handle failed!", __func__);
				return -1;
			}
		}
	}

	if (flag & NM_COMPONENT_SWITCH) {
		if (ctl->sw == NULL) {
			ctl->sw = nm_switch_open();
			if (!ctl->sw) {
				LOGERROR("%s: open switch handle failed!", __func__);
				return -1;
			}
		}
	}

	return 0;
}

void *nm_component_get_handle(void *hd, int flag)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	
	if (!ctl)
		return NULL;

	if (flag & NM_COMPONENT_E1PHY) 
		return ctl->e1phy;

	if (flag & NM_COMPONENT_FPGA) 
		return ctl->fpga;

	if (flag & NM_COMPONENT_SWITCH) 
		return ctl->sw;

	return NULL;
}

int nm_component_close_comp(void *hd, int flag)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	
	if (!ctl)
		return -1;

	if (flag & NM_COMPONENT_E1PHY) {
		nm_e1phy_close(ctl->e1phy);
		ctl->e1phy = NULL;
	}

	if (flag & NM_COMPONENT_FPGA) {
		nm_fpga_close(ctl->fpga);
		ctl->fpga = NULL;
	}

	if (flag & NM_COMPONENT_SWITCH) {
		nm_switch_close(ctl->sw);
		ctl->sw = NULL;
	}

	return 0;
}

unsigned int nm_component_get_comp(void *glb, char *ip, int port)
{
	struct nmboard_info bdi;
	int rc = 0;
	
	memset(&bdi, 0, sizeof(bdi));

	rc = nms_glb_get_bdinfo(glb, ip, port, &bdi);
	if (rc != 0) {
		LOGERROR("%s: get bdinfo failed\n", __func__);
		return -1;
	}

	return bdi.comp;
}

int nm_component_close(void *hd)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;

	if (!ctl)
		return -1;

	if (ctl->e1phy) {
		nm_e1phy_close(ctl->e1phy);
		ctl->e1phy = NULL;
	}
	if (ctl->fpga) {
		nm_fpga_close(ctl->fpga);
		ctl->fpga = NULL;
	}
	if (ctl->sw) {
		nm_switch_close(ctl->sw);
		ctl->sw = NULL;
	}

	free(ctl);
	ctl = NULL;

	return 0;
}

void *nm_component_lproc_glb_bdinfo(void *hd, void *glb, char *ip, int port, void *data)
{
	struct nmclient *client = (struct nmclient *)hd;
	struct nm_component_ctl *ctl = NULL;
	struct nmboard_info bdi;

	if (!client || !glb || !ip || !data)
		return NULL;

	ctl = (struct nm_component_ctl *)client->component;
	memset(&bdi, 0, sizeof(bdi));
	nm_glb_decode_bdinfo(data, &bdi);
	nms_glb_insert_bdinfo(glb, ip, port, &bdi);
	nmclient_update_time(client);
	/* open component */
	nm_component_open_comp(ctl, bdi.comp);

	return NULL;
}

void *nm_component_lproc_glb_ntp(void *hd, void *glb, char *ip, int port, void *data)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;

	if (!ctl)
		return NULL;

	return NULL;
}

void *nm_component_lproc_glb(void *hd, void *glb, char *ip, int port, void *data)
{
	struct nmclient *client = (struct nmclient *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *oph = NULL, *p = NULL;
	int cmd = NM_CMD_UNKNOWN;

	if (!client || !glb || !data)
		return NULL;

	p = (unsigned char *)nmpkthdr_get_data(nmph);
	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_BOARD_INFO:
			oph = nm_component_lproc_glb_bdinfo(client, glb, ip, port, p);
			break;
		case NM_CMD_GET_NTP_TIME:
			oph = nm_component_lproc_glb_ntp(client->component, glb, ip, port, p);
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

int nm_component_lproc_e1phy_status(void *hd, void *data)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	int phycnt, pcnt = 0;
	unsigned char *p = NULL;
	int rc = 0;

	if (!ctl || !data)
		return -1;

	p = (unsigned char *)nmpkthdr_get_data(nmph);
	phycnt = *p++;
	pcnt   = *p++;
	rc = nm_e1phy_add_data(ctl->e1phy, phycnt, pcnt, p);
	if (rc != 0)
		return -1;

	return 0;
}

void *nm_component_lproc_e1phy(void *hd, void *data)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *oph = NULL;
	int cmd = NM_CMD_UNKNOWN;

	if (!ctl || !data)
		return NULL;

	if (ctl->e1phy == NULL) {
		LOGERROR("e1phy handle is NULL, Maybe this client hasn't e1phy component. ");
		return NULL;
	}

	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_E1PHY_STATUS:
			nm_component_lproc_e1phy_status(hd, data);
			break;
		case NM_CMD_TRAP_E1PHY_LSCHG:
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

int nm_component_lproc_decode_id_type(void *hd, void *data, int *id, int *type)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *p = NULL;

	if (!ctl || !data)
		return -1;

	if (id != NULL && type != NULL) {
		p = (unsigned char *)nmpkthdr_get_data(nmph);
		*id   = *p++;
		*type = *p++;
	}

	return 0;
}

int nm_component_lproc_fpga_info(void *hd, void *data)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *p = NULL;
	int id, type = 0;
	int cnt, i = 0;

	if (!ctl || !data)
		return -1;

	p = (unsigned char *)nmpkthdr_get_data(nmph);
	cnt = *p++;
	for (i = 0; i < cnt; i++) {
		id = *p++;
		type = *p++;
		nm_fpga_add_id_type(ctl->fpga, id, type);
	}

	return 0;
}

void *nm_component_lproc_fpga(void *hd, void *data)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *p = NULL;
	unsigned char *oph = NULL;
	int id, type = 0;
	int cmd = NM_CMD_UNKNOWN;

	if (!ctl || !data)
		return NULL;

	if (ctl->fpga == NULL) {
		LOGERROR("fpga handle is NULL, Maybe this client hasn't fpga component. ");
		return NULL;
	}

	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_FPGA_INFO:
			nm_component_lproc_fpga_info(hd, data);
			break;
		case NM_CMD_GET_FPGA_STATUS:
			p = (unsigned char *)(data + sizeof(*nmph));
			nm_component_lproc_decode_id_type(hd, data, &id, &type);
			nm_fpga_add_data(ctl->fpga, id, type, p, FPGA_DATA_STATUS);
			break;
		case NM_CMD_GET_FPGA_STAT:
			p = (unsigned char *)(data + sizeof(*nmph));
			nm_component_lproc_decode_id_type(hd, data, &id, &type);
			nm_fpga_add_data(ctl->fpga, id, type, p, FPGA_DATA_STAT);
			break;
		case NM_CMD_GET_FPGA_FLOW_STAT:
			p = (unsigned char *)(data + sizeof(*nmph));
			nm_component_lproc_decode_id_type(hd, data, &id, &type);
			nm_fpga_add_data(ctl->fpga, id, type, p, FPGA_DATA_FSTAT);
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

void *nm_component_lproc_switch(void *hd, void *data)
{
	struct nm_component_ctl *ctl = (struct nm_component_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *oph = NULL;
	unsigned char *p = NULL;
	int cmd = NM_CMD_UNKNOWN;

	if (!ctl || !data)
		return NULL;

	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_SW_PORT_STATUS:
			p = (unsigned char *)(data + sizeof(*nmph));
			nm_switch_add_data(ctl->sw, p);
			break;
		case NM_CMD_GET_SW_PORT_STAT:
			break;
		case NM_CMD_TRAP_SW_PORT_LSCHG:
			break;
		case NM_CMD_SET_SW_PORT_AUTONEG:
			break;
		case NM_CMD_SET_SW_PORT_PWRDN:
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

void *nm_component_lprocess(void *hd, void *glb, char *ip, int port, void *data)
{
	struct nmclient *client = (struct nmclient *)hd;
	pkt_hdr *ph = (pkt_hdr *)data, *oph = NULL;
	nmpkt_hdr *nmph = NULL;
	unsigned char module = 0;
	int len = 0;

	if (!hd || !data)
		return NULL;

	nmph   = (nmpkt_hdr *)pkthdr_get_data(ph);
	module = nmph->module;
	switch(module) {
		case NM_MODULE_GLB_INFO:
			oph = nm_component_lproc_glb(client, glb, ip, port, nmph);
			break;
		case NM_MODULE_E1PHY:
			oph = nm_component_lproc_e1phy(client->component, nmph);
			break;
		case NM_MODULE_SDHPHY:
			break;
		case NM_MODULE_POSPHY:
			break;
		case NM_MODULE_FPGA:
			oph = nm_component_lproc_fpga(client->component, nmph);
			break;
		case NM_MODULE_SWITCH:
			oph = nm_component_lproc_switch(client->component, nmph);
			break;
		case NM_MODULE_TILE:
			break;
		default:
			LOGERROR("unknown this module %d\n", module);
			break;
	}

	if (oph) {
		nmph = (nmpkt_hdr *)pkthdr_get_data(oph);
		len = nmpkthdr_get_dlen(nmph) + sizeof(*nmph);
		pkthdr_set_sync(oph);
		pkthdr_set_plen(oph, len + PKT_LEN);
	}

	return oph;
}

