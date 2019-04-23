/*
 *
 * nm_client.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include "os.h"
#include "aplog.h"
#include "misc.h"
#include "list.h"
#include "mutex.h"
#include "pkt.h"
#include "nmpkt.h"
#include "nm_typdef.h"
#include "nm_glb.h"
#include "nm_e1phy.h"
#include "nm_fpga.h"
#include "nm_switch.h"
#include "nm_msg.h"
#include "nm_component.h"
#include "nmserver.h"
#include "nm_client.h"

#define TIMEOUT   15

struct nmclient_ctrl {
	int port;
	void  *lock;
	struct list_head head;   /* client list */

	void *cmd_lock;
	struct list_head cmd_head; /* send cmd list */

	void *data;
};

void *nmclient_cmd_get(void *ctl)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;
	struct nmclient_cmd *cmd = NULL;
	struct list_head  *p, *n = NULL;

	if (!ctrl)
		return NULL;

	mutex_lock(ctrl->cmd_lock);
	list_for_each_safe(p, n, &ctrl->cmd_head) {
		cmd = list_entry(p, struct nmclient_cmd, node);
		if (NULL != cmd) {
			list_del(p);
			mutex_unlock(ctrl->cmd_lock);
			return cmd;
		}
	}
	mutex_unlock(ctrl->cmd_lock);

	return NULL;
}

void nmclient_cmd_release(void *cmd)
{
	struct nmclient_cmd *c = (struct nmclient_cmd *)cmd;

	if (NULL != c) {
		if (NULL != c->ph) {
			free(c->ph);
		}
		free(c);
	}
}

int nmclient_cmd_del(void *ctl, char *ip, int port)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;
	struct nmclient_cmd *cmd = NULL;
	struct list_head  *p, *n = NULL;

	if (!ctrl)
		return -1;

	mutex_lock(ctrl->cmd_lock);
	list_for_each_safe(p, n, &ctrl->cmd_head) {
		cmd = list_entry(p, struct nmclient_cmd, node);
		if (!strcmp(ip, cmd->ip) && port == cmd->port) {
			list_del(p);
			nmclient_cmd_release(cmd);
		}
	}
	mutex_unlock(ctrl->cmd_lock);

	return 0;
}

void nmclient_cmd_destroy(void *ctl)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;
	struct nmclient_cmd *cmd = NULL;
	struct list_head  *p, *n = NULL;

	if (!ctrl)
		return;

	mutex_lock(ctrl->cmd_lock);
	list_for_each_safe(p, n, &ctrl->cmd_head) {
		cmd = list_entry(p, struct nmclient_cmd, node);
		list_del(p);
		nmclient_cmd_release(cmd);
	}
	mutex_unlock(ctrl->cmd_lock);
}

int nmclient_cmd_add(void *ctl, void *cmd)
{
	struct nmclient_cmd *c = NULL;
	struct nmclient_ctrl *ctrl = NULL;

	if (!ctl || !cmd)
		return -1;

	c = (struct nmclient_cmd *)cmd;
	ctrl = (struct nmclient_ctrl *)ctl;

	list_add_tail(&c->node, &ctrl->cmd_head);

	return 0;
}

void *nmclient_cmd_create(char *ip, int port, void *ph, int id)
{
	struct nmclient_cmd *cmd = NULL;

	if (!ip || !ph)
		return NULL;

	cmd = (struct nmclient_cmd *)malloc(sizeof(*cmd));
	if (NULL != cmd) {
		strcpy(cmd->ip, ip);
		cmd->port = port;
		cmd->ph = ph;
		cmd->id = id;
		return cmd;
	}

	return NULL;
}

void nmclient_cmd_timer_query(void *ctl)
{
	struct list_head  *p, *n = NULL;
	struct nmclient_cmd *cmd = NULL;
	struct nmclient *client = NULL;
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;

	if (!ctrl)
		return;

	mutex_lock(ctrl->lock);
	list_for_each_safe(p, n, &ctrl->head) {
		client = list_entry(p, struct nmclient, node);
		if (NULL != client) {
			cmd = nmclient_cmd_create(client->ip, client->port, nmmsg_cmd_glb_info(), 0);
			if (NULL != cmd) {
				nmclient_cmd_add(ctrl, cmd);
			}
		}
	}
	mutex_unlock(ctrl->lock);
}

void *nmclient_get_node(void *ctl, char *ip, int port, int delflag)
{
	struct list_head  *p, *n = NULL;
	struct nmclient *client = NULL;
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;

	if (!ctrl || !ip)
		return NULL;

	list_for_each_safe(p, n, &ctrl->head) {
		client = list_entry(p, struct nmclient, node);
		if (NULL != client && !strcmp(client->ip, ip) && client->port == port) {
			if (delflag) {
				list_del(p);
			}
			return client;
		}
	}

	return NULL;
}

//TAG:Node operations~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int nmclient_add_node(void *ctl, char *ip, int port)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;
	struct nmclient *client = NULL;
	struct timeval t;

	if (!ctrl || !ip)
		return -1;

	mutex_lock(ctrl->lock);
	client = nmclient_get_node(ctrl, ip, port, 0);
	if (NULL == client) {
		client = (struct nmclient *)malloc(sizeof(*client));
		if (NULL != client) {
			memset(client, 0, sizeof(*client));

			client->port = port;
			strcpy(client->ip, ip);
			gettimeofday(&t, NULL);
			client->time_s = (unsigned int)t.tv_sec;

			list_add_tail(&client->node, &ctrl->head);
		}
		LOGDEBUG("Client(%s:%d) connect to server port:%d", ip, port, ctrl->port);
	}
	else {
		LOGERROR("before release, the same nmclient connection; ip %s port %d", ip, port);
	}

	mutex_unlock(ctrl->lock);

	return 0;
}

static void nmclient_release_node(void *node)
{
	struct nmclient *client = (struct nmclient *)node;

	if (client != NULL) {
		nm_component_close(client->component);
		free(client);
		client = NULL;
	}
}

int nmclient_del_node(void *ctl, char *ip, int port)
{
	struct nmclient *client= NULL;
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;

	if (!ctrl)
		return -1;

	mutex_lock(ctrl->lock);
	client = nmclient_get_node(ctrl, ip, port, 1);
	if (NULL != client) {
		nmclient_release_node(client);
		mutex_unlock(ctrl->lock);
		return 0;
	}
	mutex_unlock(ctrl->lock);

	return -1;
}

int nmclient_destroy_nodes(void *ctl)
{
	struct list_head  *p, *n;
	struct nmclient *client = NULL;
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;

	if (!ctrl)
		return -1;

	mutex_lock(ctrl->lock);
	list_for_each_safe(p, n, &ctrl->head) {
		client = list_entry(p, struct nmclient, node);
		if (client!= NULL) {
			list_del(p);
			nmclient_release_node(client);
		}
	}
	mutex_unlock(ctrl->lock);

	return 0;
}

void nmclient_update_time(void *node)
{
	struct nmclient *client = (struct nmclient *)node;
	struct timeval t;

	gettimeofday(&t, NULL);

	if (!client)
		return;

	client->time_s = (unsigned int)t.tv_sec;
}

int nmclient_check_timeout(void *node)
{
	struct nmclient *client = (struct nmclient *)node;
	struct timeval t;

	if (!client)
		return -1;
	
	gettimeofday(&t, NULL);
	if ((t.tv_sec - client->time_s) > TIMEOUT) {
		LOG("TIMEOUT: %s:%d; time:%u:%u", client->ip, client->port, (unsigned int)t.tv_sec, client->time_s);
		return 1;
	}

	return 0;
}

int nmclient_timeout_release(void *ctl, void *glb)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;
	struct nmclient *client = NULL;
	struct list_head *p, *n = NULL;

	if (!ctl)
		return -1;

	mutex_lock(ctrl->lock);
	list_for_each_safe(p, n, &ctrl->head) {
		client = list_entry(p, struct nmclient, node);
		if (nmclient_check_timeout(client) == 1) {
			list_del(p);
			/* release node */
			nmclient_cmd_del(ctrl, client->ip, client->port);
			if (ctrl->port == NMSERVER_LPORT) {
				nms_glb_del_bdinfo(glb, client->ip, client->port);
			}
			nmclient_release_node(client);
		}
	}
	mutex_unlock(ctrl->lock);

	return 0;
}

void *nmclient_rproc_glb(void *glb, void *data, int *dlen)
{
	/* this maybe need struct nmclient_ctrl */
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *oph = NULL;
	int cmd = NM_CMD_UNKNOWN;

	if (!glb || !data)
		return NULL;

	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_BOARD_INFO:
			oph = nms_glb_get_all_bdinfo_pkt(glb, dlen);
			break;
		case NM_CMD_GET_NTP_TIME:
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

static void *nmclient_rproc_get_comp_hdl(void *hd, void *glb, void *data, 
		struct nm_lkaddr_info *lki, int *id, int *type, int flag)
{
	struct nmclient_ctrl *rctl = (struct nmclient_ctrl *)hd;
	struct nmclient_ctrl *lctl = NULL;
	struct nmclient *lcli = NULL;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	void *comp = NULL;
	unsigned char *p = NULL;
	char ip[IP_BUF_LEN];
	int port;
	int rc = 0;

	if (!rctl || !glb || !data || !lki)
		return NULL;

	p = (unsigned char *)nmpkthdr_get_data(nmph);
	rc = nm_glb_decode_lkaddr_info(p, lki);
	if (rc) {
		LOGERROR("%s: nm_glb_decode_lkaddr_info failed", __func__);
		return NULL;
	}

	rc = nms_glb_get_ip_port(glb, ip, &port, lki);
	if (rc) {
		LOGERROR("%s: nms_glb_get_ip_port failed!", __func__);
		return NULL;
	}

	if (id != NULL && type != NULL) {
		*id   = *(p + sizeof(lki));
		*type = *(p + sizeof(lki) + 1);
	}

	lctl = nmclient_ctrl_get_data(rctl);
	lcli = nmclient_get_node(lctl, ip, port, 0);
	if (lcli == NULL) {
		LOGERROR("%s: lcli is NULL", __func__);
		return NULL;
	}
	if (lcli->component == NULL) {
		LOGERROR("%s: lcli component is NULL", __func__);
		return NULL;
	}
	comp = nm_component_get_handle(lcli->component, flag);
	if (comp == NULL) {
		LOGERROR("%s: get %d comp handle is NULL", __func__, flag);
		return NULL;
	}

	return comp;
}

void *nmclient_rproc_e1phy(void *hd, void *glb, void *data, int *dlen)
{
	struct nmclient_ctrl *rctl = (struct nmclient_ctrl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	void *oph = NULL;
	void *e1phy = NULL;
	struct nm_lkaddr_info lki;
	int cmd = NM_CMD_UNKNOWN;

	if (!hd || !glb || !data)
		return NULL;


	memset(&lki, 0, sizeof(lki));
	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_E1PHY_STATUS:
			e1phy = nmclient_rproc_get_comp_hdl(rctl, glb, data, &lki, NULL, NULL, NM_COMPONENT_E1PHY);
			oph = nm_e1phy_get_data_pkt(e1phy, &lki, dlen);
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

void *nmclient_rproc_fpga(void *hd, void *glb, void *data, int *dlen)
{
	struct nmclient_ctl *ctl = (struct nmclient_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *oph = NULL;
	void *fpga = NULL;
	struct nm_lkaddr_info lki;
	int id, type = 0;
	int cmd = NM_CMD_UNKNOWN;

	if (!ctl || !data)
		return NULL;

	memset(&lki, 0, sizeof(lki));
	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_FPGA_INFO:
			fpga = nmclient_rproc_get_comp_hdl(hd, glb, data, &lki, NULL, NULL, NM_COMPONENT_FPGA);
			oph = nm_fpga_get_id_type_pkt(fpga, &lki, dlen);
			break;
		case NM_CMD_GET_FPGA_STATUS:
			fpga = nmclient_rproc_get_comp_hdl(hd, glb, data, &lki, &id, &type, NM_COMPONENT_FPGA);
			oph = nm_fpga_get_data_pkt(fpga, id, type, &lki, dlen, FPGA_DATA_STATUS);
			break;
		case NM_CMD_GET_FPGA_STAT:
			fpga = nmclient_rproc_get_comp_hdl(hd, glb, data, &lki, &id, &type, NM_COMPONENT_FPGA);
			oph = nm_fpga_get_data_pkt(fpga, id, type, &lki, dlen, FPGA_DATA_STAT);
			break;
		case NM_CMD_GET_FPGA_FLOW_STAT:
			fpga = nmclient_rproc_get_comp_hdl(hd, glb, data, &lki, &id, &type, NM_COMPONENT_FPGA);
			oph = nm_fpga_get_data_pkt(fpga, id, type, &lki, dlen, FPGA_DATA_FSTAT);
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

int nmclient_rproc_set_cmd(void *hd, void *glb, void *data, void *oph)
{
	struct nmclient_ctl *rctl = (struct nmclient_ctl *)hd;
	struct nmclient_ctl *lctl = NULL;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *p = NULL;
	struct nmclient_cmd *cmd = NULL;
	struct nm_lkaddr_info lki;
	char ip[IP_BUF_LEN];
	int port = 0;
	int rc = 0;

	if (!rctl || !data)
		return -1;

	memset(&lki, 0, sizeof(lki));
	p = (unsigned char *)nmpkthdr_get_data(nmph);
	rc = nm_glb_decode_lkaddr_info(p, &lki);
	if (rc) {
		LOGERROR("%s: nm_glb_decode_lkaddr_info failed", __func__);
		return -1;
	}

	rc = nms_glb_get_ip_port(glb, ip, &port, &lki);
	if (rc) {
		LOGERROR("%s: nms_glb_get_ip_port failed!", __func__);
		return -1;
	}

	lctl = nmclient_ctrl_get_data(rctl);
	cmd = nmclient_cmd_create(ip, port, oph, 0);
	if (NULL != cmd) {
		nmclient_cmd_add(lctl, cmd);
	}

	return 0;
}

void *nmclient_rproc_switch(void *hd, void *glb, void *data, int *dlen)
{
	struct nmclient_ctl *ctl = (struct nmclient_ctl *)hd;
	nmpkt_hdr *nmph = (nmpkt_hdr *)data;
	unsigned char *oph = NULL;
	void *sw = NULL;
	struct nm_lkaddr_info lki;
	int cmd = NM_CMD_UNKNOWN;

	if (!ctl || !data)
		return NULL;

	memset(&lki, 0, sizeof(lki));
	cmd = nmph->cmd;
	switch (cmd) {
		case NM_CMD_GET_SW_PORT_STATUS:
			sw  = nmclient_rproc_get_comp_hdl(hd, glb, data, &lki, NULL, NULL, NM_COMPONENT_SWITCH);
			oph = nm_sw_get_status_pkt(sw, &lki, dlen);
			break;
		case NM_CMD_GET_SW_PORT_STAT:
			break;
		case NM_CMD_TRAP_SW_PORT_LSCHG:
			break;
		case NM_CMD_SET_SW_PORT_AUTONEG:
			nmclient_rproc_set_cmd(hd, glb, data, nmmsg_cmd_set_sw_an(data));
			break;
		case NM_CMD_SET_SW_PORT_PWRDN:
			nmclient_rproc_set_cmd(hd, glb, data, nmmsg_cmd_set_sw_pwrdn(data));
			break;
		default:
			LOGERROR("%s: unknown this cmd %d", __func__, cmd);
			break;
	}

	return oph;
}

//TAG:client process

void *nmclient_rprocess(void *ctl, void *glb, char *ip, int port, void *data)
{
	struct nmclient_ctrl *rctl = (struct nmclient_ctrl *)ctl;
	pkt_hdr *ph = (pkt_hdr *)data, *oph = NULL;
	nmpkt_hdr *nmph, *nmp = NULL;
	int dlen = 0;

	if (!ctl || !ip || !data)
		return NULL;

	nmph = (nmpkt_hdr *)pkthdr_get_data(ph);

	switch (nmph->module) {
		case NM_MODULE_GLB_INFO:
			oph = nmclient_rproc_glb(glb, nmph, &dlen);
			break;
		case NM_MODULE_E1PHY:
			oph = nmclient_rproc_e1phy(rctl, glb, nmph, &dlen);
			break;
		case NM_MODULE_SDHPHY:
			break;
		case NM_MODULE_POSPHY:
			break;
		case NM_MODULE_FPGA:
			oph = nmclient_rproc_fpga(rctl, glb, nmph, &dlen);
			break;
		case NM_MODULE_SWITCH:
			oph = nmclient_rproc_switch(rctl, glb, nmph, &dlen);
			break;
		case NM_MODULE_TILE:
			break;
		default:
			LOGERROR("unknown this module %d\n", nmph->module);
			break;
	}

	if (oph != NULL) {
		/* set nmpkt_hdr hdr */
		nmp = (nmpkt_hdr *)pkthdr_get_data(oph);
		nmpkthdr_construct_hdr(nmp, nmph->module, nmph->cmd, nmph->seq, dlen);
		pkthdr_set_sync(oph);
		pkthdr_set_plen(oph, dlen + NMPKT_LEN + PKT_LEN);
	}

	return oph;
}

void *nmclient_lprocess(void *ctl, void *glb, char *ip, int port, void *data)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;
	struct nmclient *client = NULL;
	pkt_hdr *oph = NULL;

	if (NULL != ctl && NULL != ip && NULL != data) {
		mutex_lock(ctrl->lock);
		client = nmclient_get_node(ctl, ip, port, 0);
		if (NULL == client) {
			LOGERROR("lprocess: %s:%d get node failed.", ip, port);
			mutex_unlock(ctrl->lock);
			return NULL;
		}
		if (!client->component) {
			client->component = nm_component_open();
			if (!client->component) {
				LOGERROR("%s: client->component open failed", __func__);
				mutex_unlock(ctrl->lock);
				return NULL;
			}
		}
		oph = nm_component_lprocess(client, glb, ip, port, data);
		mutex_unlock(ctrl->lock);
	}
	else {
		LOGERROR("nmclient_lprocess: input parameter has NULL(%p:%p:%p).", ctl, ip, data);
	}

	return oph;
}

//TAG:ctrl~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void nmclient_ctrl_set_data(void *ctl, void *data)
{
	struct nmclient_ctrl *ctrl = NULL;

	if (NULL != ctl && NULL != data) {
		ctrl = (struct nmclient_ctrl *)ctl;
		ctrl->data = data;
	}
}

void *nmclient_ctrl_get_data(void *ctl)
{
	struct nmclient_ctrl *ctrl = NULL;

	if (NULL != ctl) {
		ctrl = (struct nmclient_ctrl *)ctl;
		return ctrl->data;
	}

	return NULL;
}

void *nmclient_ctrl_init(int svr_port)
{
	struct nmclient_ctrl *ctrl = NULL;

	ctrl = malloc(sizeof(*ctrl));
	if (NULL != ctrl) {

		memset(ctrl, 0, sizeof(*ctrl));
		ctrl->port = svr_port;
		INIT_LIST_HEAD(&ctrl->head);
		INIT_LIST_HEAD(&ctrl->cmd_head);

		ctrl->lock = mutex_open(NULL);
		ctrl->cmd_lock = mutex_open(NULL);
		if (NULL != ctrl->lock && NULL != ctrl->cmd_lock) {
			return ctrl;
		}
	}

	nmclient_ctrl_exit(ctrl);

	return NULL;
}

void nmclient_ctrl_exit(void *ctl)
{
	struct nmclient_ctrl *ctrl = (struct nmclient_ctrl *)ctl;

	if (NULL != ctrl)  {
		nmclient_cmd_destroy(ctrl);
		nmclient_destroy_nodes(ctrl);

		if (NULL != ctrl->lock) {
			mutex_close(ctrl->lock);
		}

		if (NULL != ctrl->cmd_lock) {
			mutex_close(ctrl->cmd_lock);
		}
	}

	free(ctrl);
}

