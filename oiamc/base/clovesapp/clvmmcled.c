/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * clvmmcled.c - A description goes here.
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
#include <libgen.h>
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

static unsigned char ledname = 0x00;
static unsigned char ledstatus = 0x55;

static void usage(char *program)
{
	fprintf(stderr, "Usage: %s [red | green] [on | quick | slow | off].\n", basename(program));
}

static unsigned char ipmi_msg_checksum(unsigned char *data, int len)
{
	unsigned char sum = 0;

	for (; len > 0; len--) {
		sum += *data++;
	}

	return sum;
}

static int msg_encode_setled(unsigned char *data, int len)
{
	unsigned char *p = data;

	memset(data, 0, len);

	*p++ = I2C_ADDR_IPMC;
	*p++ = (IPMI_NETFN_GROUPEX_REQ << 2);
	*p++ = ipmi_msg_checksum(data, 2);
	*p++ = I2C_ADDR_P1014;
	*p++ = 0x00;
	*p++ = IPMI_CMD_SETLED;
	*p++ = ledname;
	*p++ = ledstatus;
	*p++ = 0x00;
	*p++ = ipmi_msg_checksum(data + 3, 6);

	return (p - data);
}

int main(int argc, char *argv[])
{
	int fd;
	unsigned char buf[64];
	int len, rc = 0;
	struct i2c_rdwr_ioctl_data ioctl_data;
	struct i2c_msg msgs[1];
	struct ipmi_msg_res *msgres;

	if (argc != 3) {
		usage(argv[0]);
		return -1;
	}

	if (!strcmp(argv[1], "red")) {
		ledname = 0x01;
	} else if (!strcmp(argv[1], "green")) {
		ledname = 0x02;
	} else {
		usage(argv[0]);
		return -1;
	}

	if (!strcmp(argv[2], "on")) {
		ledstatus = 0xff;
	} else if (!strcmp(argv[2], "quick")) {
		ledstatus = 0x01;
	} else if (!strcmp(argv[2], "slow")) {
		ledstatus = 0x02;
	} else if (!strcmp(argv[2], "off")) {
		ledstatus = 0x00;
	} else {
		usage(argv[0]);
		return -1;
	}

	fd = open(I2C_DEVFILE, O_RDWR);
	if (fd < 0) {
		return -1;
	}

	/* Send request */
	memset(&ioctl_data, 0, sizeof(ioctl_data));
	memset(msgs, 0, sizeof(msgs));

	len = msg_encode_setled(buf, sizeof(buf));
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
		rc = 0;
	}
	else if (msgres->result == 0xc1) {
		rc = -2;
	}
	else if (msgres->result == 0xc3) {
		rc = -3;
	}
	else {
		rc = -4;
	}

	close(fd);
	return rc;
}

