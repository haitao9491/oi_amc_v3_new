#ifndef __H_NM_CLIENT_H_
#define __H_NM_CLIENT_H_

#include "list.h"

struct nmclient {
	struct list_head node;

	char ip[IP_BUF_LEN];
	unsigned short port;
	unsigned short type; 
	unsigned int time_s;

	void *component;
};

struct nmclient_cmd {
	struct list_head node;

	int id;
	char ip[IP_BUF_LEN];
	int port;

	void *ph;
};

#if defined(__cplusplus)
extern "C" {
#endif

void *nmclient_cmd_get(void *ctl);

void nmclient_cmd_release(void *cmd);
void nmclient_cmd_timer_query(void *ctl);

int nmclient_add_node(void *ctl, char *ip, int port);
int nmclient_del_node(void *ctl, char *ip, int port);
void nmclient_update_time(void *node);
int nmclient_timeout_release(void *ctl, void *glb);

void *nmclient_lprocess(void *ctl, void *glb, char *ip, int port, void *data);
void *nmclient_rprocess(void *ctl, void *glb, char *ip, int port, void *data);

void nmclient_ctrl_set_data(void *ctl, void *data);
void *nmclient_ctrl_get_data(void *ctl);
void nmclient_ctrl_exit(void *ctl);
void *nmclient_ctrl_init(int port);

#if defined(__cplusplus)
}
#endif

#endif
