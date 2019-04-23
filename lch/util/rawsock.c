/*
 *
 * rawsock.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include "os.h"
#include "aplog.h"
#include "misc.h"
#include "rawsock.h"

struct rawsock_hd {
	int sd;
	char *device;
	union {
		struct {
			unsigned char enet_dst[6];
			unsigned char enet_src[6];
			unsigned short enet_type;
		} d;
		unsigned char buf[14];
	} etherhdr;
};

static int rawsock_do_open(struct rawsock_hd *hd)
{
	struct ifreq ifr;
	struct sockaddr_ll sll;

	hd->sd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (hd->sd == -1) {
		LOGERROR("rawsock_open: open socket failed.");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, hd->device,
			lchmin(sizeof(ifr.ifr_name), strlen(hd->device)));
	if (ioctl(hd->sd, SIOCGIFINDEX, &ifr) == -1) {
		LOGERROR("rawsock_open: ioctl SIOCGIFINDEX failed.");
		return -1;
	}

	memset(&sll, 0, sizeof(sll));
	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(ETH_P_ALL);

	if (bind(hd->sd, (struct sockaddr *)&sll, sizeof(sll)) == -1) {
		LOGERROR("rawsock_open: bind socket failed.");
		return -1;
	}

	if ((hd->etherhdr.d.enet_src[0] == 0) &&
	    (hd->etherhdr.d.enet_src[1] == 0) &&
	    (hd->etherhdr.d.enet_src[2] == 0) &&
	    (hd->etherhdr.d.enet_src[3] == 0) &&
	    (hd->etherhdr.d.enet_src[4] == 0) &&
	    (hd->etherhdr.d.enet_src[5] == 0)) {
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, hd->device,
			lchmin(sizeof(ifr.ifr_name), strlen(hd->device)));
		if (ioctl(hd->sd, SIOCGIFHWADDR, &ifr) == -1) {
			LOGERROR("rawsock_open: ioctl SIOCGIFHWADDR failed.");
			return -1;
		}
		memcpy(hd->etherhdr.d.enet_src, ifr.ifr_hwaddr.sa_data, 6);
	}

	return 0;
}

void *rawsock_open(char *device,
		char *enet_dst, char *enet_src, unsigned short enet_type)
{
	struct rawsock_hd *hd;

	hd = (struct rawsock_hd *)malloc(sizeof(*hd));
	if (hd == NULL) {
		LOGCRITICAL("rawsock_open: Insufficient memory.");
		return NULL;
	}
	memset(hd, 0, sizeof(*hd));
	hd->sd = -1;

	hd->device = strdup(device ? device : "eth0");

	if (enet_dst)
		memcpy(hd->etherhdr.d.enet_dst, enet_dst, 6);
	else {
		hd->etherhdr.d.enet_dst[0] = 0x00;
		hd->etherhdr.d.enet_dst[1] = 0x01;
		hd->etherhdr.d.enet_dst[2] = 0x6C;
		hd->etherhdr.d.enet_dst[3] = 0x01;
		hd->etherhdr.d.enet_dst[4] = 0x02;
		hd->etherhdr.d.enet_dst[5] = 0x03;
	}

	if (enet_src)
		memcpy(hd->etherhdr.d.enet_src, enet_src, 6);

	if (enet_type == 0)
		enet_type = 0x0800;
	hd->etherhdr.d.enet_type = htons(enet_type);

	if (rawsock_do_open(hd) < 0) {
		rawsock_close(hd);
		return NULL;
	}

	return hd;
}

int rawsock_send(void *hd, unsigned char *data, int len)
{
	struct rawsock_hd *rhd = (struct rawsock_hd *)hd;

	data -= 14;
	len += 14;
	memcpy(data, rhd->etherhdr.buf, 14);

	if (write(rhd->sd, data, len) != len)
		return -1;
	return 0;
}

int rawsock_copy_send(void *hd, unsigned char *data, int len)
{
	struct rawsock_hd *rhd = (struct rawsock_hd *)hd;
	unsigned char *buf;

	buf = (unsigned char *)malloc(len + 14);
	if (buf == NULL) {
		LOGCRITICAL("rawsock_copy_send: insufficient memory.");
		return -1;
	}

	memcpy(buf, rhd->etherhdr.buf, 14);
	memcpy(buf + 14, data, len);
	if (write(rhd->sd, buf, len + 14) != (len + 14)) {
		free(buf);
		return -1;
	}

	free(buf);
	return 0;
}

void rawsock_close(void *hd)
{
	struct rawsock_hd *rhd = (struct rawsock_hd *)hd;

	if (rhd) {
		if (rhd->device)
			free(rhd->device);
		if (rhd->sd != -1)
			close(rhd->sd);
		free(rhd);
	}
}

