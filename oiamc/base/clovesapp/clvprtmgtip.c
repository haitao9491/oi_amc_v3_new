/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * clvprtmgtip.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


#define I2C_DEVFILE		"/dev/i2c-0"
#define I2C_ADDR_P1014		0x5C
#define I2C_ADDR_IPMC		0x5A

#define IPMI_NETFN_APP_REQ	0x06
#define IPMI_NETFN_APP_RES	0x07
#define IPMI_NETFN_GROUPEX_REQ	0x2C
#define IPMI_NETFN_GROUPEX_RES	0x2D

#define IPMI_CMD_SETLED		0x07
#define IPMI_CMD_GETHA		0x34


struct ipmi_msg_req {
	unsigned char dstaddr;
	unsigned char netfn_lun;
	unsigned char hchksum;
	unsigned char srcaddr;
	unsigned char seq_lun;
	unsigned char cmd;

	/* payload (N bytes) corresponding to cmd */
	/* data checksum (1 byte) */
};

struct ipmi_msg_res {
	unsigned char dstaddr;
	unsigned char netfn_lun;
	unsigned char hchksum;
	unsigned char srcaddr;
	unsigned char seq_lun;
	unsigned char cmd;
	unsigned char result;

	/* payload (N bytes) corresponding to cmd */
	/* data checksum (1 byte) */
};


static unsigned char ipmi_msg_checksum(unsigned char *data, int len)
{
	unsigned char sum = 0;

	for (; len > 0; len--) {
		sum += *data++;
	}

	return sum;
}

static int msg_encode_getha(unsigned char *data, int len)
{
	unsigned char *p = data;

	memset(data, 0, len);

	*p++ = I2C_ADDR_IPMC;
	*p++ = (IPMI_NETFN_APP_REQ << 2);
	*p++ = ipmi_msg_checksum(data, 2);
	*p++ = I2C_ADDR_P1014;
	*p++ = 0x00;
	*p++ = IPMI_CMD_GETHA;
	*p++ = 0xF1;
	*p++ = ipmi_msg_checksum(data + 3, 4);

	return (p - data);
}

int main(int argc, char *argv[])
{
	int fd;
	unsigned char buf[64];
	int len;
	struct i2c_rdwr_ioctl_data ioctl_data;
	struct i2c_msg msgs[1];
	struct ipmi_msg_res *msgres;
	unsigned char rack = 0, shelf = 0, ha = 0x77, ga = 1;
	unsigned char haip = 0, hostip = 0;

	fd = open(I2C_DEVFILE, O_RDWR);
	if (fd < 0) {
		return -1;
	}

	/* Send request */
	memset(&ioctl_data, 0, sizeof(ioctl_data));
	memset(msgs, 0, sizeof(msgs));

	len = msg_encode_getha(buf, sizeof(buf));
	msgs[0].addr = I2C_ADDR_IPMC >> 1;
	msgs[0].len  = len;
	msgs[0].buf  = buf;
	msgs[0].flags &= ~I2C_M_RD;
	ioctl_data.msgs  = &msgs[0];
	ioctl_data.nmsgs = 1;

	if (ioctl(fd, I2C_RDWR, &ioctl_data) != 1) {
		close(fd);
		return -1;
	}

	/* Receive response */
	memset(&ioctl_data, 0, sizeof(ioctl_data));
	memset(msgs, 0, sizeof(msgs));
	memset(buf, 0, sizeof(buf));

	msgs[0].addr = I2C_ADDR_P1014;
	msgs[0].len  = sizeof(buf);
	msgs[0].buf  = buf;
#define I2C_S_RD 0x0200
	msgs[0].flags |= I2C_M_RD;
	msgs[0].flags |= I2C_S_RD;
	ioctl_data.msgs  = &msgs[0];
	ioctl_data.nmsgs = 1;

	if (ioctl(fd, I2C_RDWR, &ioctl_data) != 1) {
		close(fd);
		return -1;
	}

	/* Process result */
	msgres = (struct ipmi_msg_res *)buf;
	if (msgres->result == 0x00) {
		rack  = (buf[8] >> 4) & 0x0f;
		shelf = (buf[8] >> 0) & 0x0f;
		ha    = buf[7];
		ga    = buf[9] + 1;
	}

	if ((ha == 0x77) || (ha == 0x4f)) {
		haip = 5 + ga;
	}
	else {
		haip = ((ha & 0x0f) + 1) * 5 + ga;
	}

	if ((rack & 0x08) == 0x08) {
		if(shelf <= 2) {
			if (haip == (5 + ga)) {
				hostip = 5 + ga;
			}
			else {
				hostip = shelf * 70 + haip;
			}	
		}
		else {
			hostip = 5 + ga;
		}
	}
	else {
		if (haip == (5 + ga)) {
			hostip = 5 + ga;
		}
		else {
			hostip = ((shelf & 0x07) * 30) + haip;
		}
	}

	rack  = rack & 0x07;
	rack += 10;
	printf("192.168.%d.%d\n", rack, hostip);

	close(fd);
	return 0;
}

