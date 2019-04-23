/*
 *
 * adapter.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "adapter.h"

struct st_adapter {
	char *name;

	void *data;

	int (*adap_open)(void *adap);
	void *(*adap_read)(void *adap);
	void *(*adap_read_from)(void *adap, char *ip, int *port);
	int (*adap_write)(void *adap, void *data, int len);
	int (*adap_write_to)(void *adap, void *data, int len, char *ip, int port);
	int (*adap_ioctl)(void *adap, int code, void *arg);
	void (*adap_freebuf)(void *adap, void *buf);
	void (*adap_close)(void *adap);
	void (*adap_set_pkt_cb)(void *adap, void *priv, adap_pkt_cb pcb);
};

void *adapter_allocate(void)
{
	struct st_adapter *adap;

	adap = (struct st_adapter *)malloc(sizeof(struct st_adapter));
	if (adap == NULL)
		return NULL;
	memset(adap, 0, sizeof(struct st_adapter));

	adap->name = strdup("Adapter.Default");

	return adap;
}

void adapter_free(void *adap)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p) {
		if (p->name)
			free(p->name);
		free(p);
	}
}

void adapter_set_open(void *adap,
		int (*adap_open)(void *adap))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_open = adap_open;
}

void adapter_set_read(void *adap,
		void *(*adap_read)(void *adap))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_read = adap_read;
}

void adapter_set_read_from(void *adap,
		void *(*adap_read_from)(void *adap, char *ip, int *port))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_read_from = adap_read_from;
}

void adapter_set_write(void *adap,
		int (*adap_write)(void *adap, void *data, int len))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_write = adap_write;
}

void adapter_set_write_to(void *adap,
		int (*adap_write_to)(void *adap, void *data, int len, char *ip, int port))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_write_to = adap_write_to;
}

void adapter_set_ioctl(void *adap,
		int (*adap_ioctl)(void *adap, int code, void *arg))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_ioctl = adap_ioctl;
}

void adapter_set_freebuf(void *adap,
		void (*adap_freebuf)(void *adap, void *buf))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_freebuf = adap_freebuf;
}

void adapter_set_close(void *adap,
		void (*adap_close)(void *adap))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_close = adap_close;
}

void adapter_set_set_pkt_cb(void *adap,
		void (*adap_set_pkt_cb)(void *adap, void *priv, adap_pkt_cb pcb))
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->adap_set_pkt_cb = adap_set_pkt_cb;
}

void adapter_set_data(void *adap, void *data)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p)
		p->data = data;
}

void *adapter_get_data(void *adap)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	return p->data;
}

void adapter_set_name(void *adap, char *adap_name)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && adap_name) {
		char *str = strdup(adap_name);

		if (str) {
			if (p->name)
				free(p->name);
			p->name = str;
		}
	}
}

char *adapter_get_name(void *adap)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	return p->name;
}

int adapter_open(void *adap)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && p->adap_open)
		return p->adap_open(adap);

	return -1;
}

void *adapter_read(void *adap)
{
	struct st_adapter *p = (struct st_adapter *)adap;
	void *data = NULL;

	if (p && p->adap_read)
		data = p->adap_read(adap);

	return data;
}

void *adapter_read_from(void *adap, char *ip, int *port)
{
	struct st_adapter *p = (struct st_adapter *)adap;
	void *data = NULL;

	if (p && p->adap_read_from)
		data = p->adap_read_from(adap, ip, port);

	return data;
}

int adapter_write(void *adap, void *data, int len)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && p->adap_write)
		return p->adap_write(adap, data, len);

	return -1;
}

int adapter_write_to(void *adap, void *data, int len, char *ip, int port)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && p->adap_write_to)
		return p->adap_write_to(adap, data, len, ip, port);

	return -1;
}

void adapter_set_pkt_cb(void *adap, void *priv, adap_pkt_cb pcb)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && p->adap_set_pkt_cb) {
		p->adap_set_pkt_cb(adap, priv, pcb);
	}
}

int adapter_ioctl(void *adap, int code, void *arg)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && p->adap_ioctl)
		return p->adap_ioctl(adap, code, arg);

	return -1;
}

void adapter_freebuf(void *adap, void *buf)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p && p->adap_freebuf)
		p->adap_freebuf(adap, buf);
	else
		free(buf);
}

void adapter_close(void *adap)
{
	struct st_adapter *p = (struct st_adapter *)adap;

	if (p) {
		if (p->adap_close)
			p->adap_close(adap);
		adapter_free(adap);
	}
}

