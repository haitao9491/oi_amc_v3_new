/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * clvpllctl.c - A description goes here.
 * 				 set si5324(Phase looked loop)
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

#define I2C_DEVFILE		"/dev/i2c-1"
#define I2C_ADDR_P1014		0x5C
#define I2C_ADDR_SI5324		0x68
#define REG_NUM				43
#define DEFAULT				0
#define DEBUG				1
#define W 					2
#define R 					3

/*********************************
 * P1014: master SI5324: slave
 *********************************/

char addr_val[REG_NUM][2] = {{0x00, 0x64}, {0x01, 0xE4}, {0x02, 0x42}, {0x03, 0x15},
	{0x04, 0x92}, {0x05, 0xED}, {0x06, 0x2D}, {0x07, 0x2A}, {0x08, 0x00}, {0x09, 0xC0},
	{0x0a, 0x00}, {0x0b, 0x40}, {0x13, 0x29}, {0x14, 0x3E}, {0x15, 0xFF}, {0x16, 0xDF},
	{0x17, 0x1F}, {0x18, 0x3F}, {0x19, 0x80}, {0x1F, 0x00}, {0x20, 0x00}, {0x21, 0x03},
	{0x22, 0x00}, {0x23, 0x00}, {0x24, 0x03}, {0x28, 0x80}, {0x29, 0x6B}, {0x2A, 0x4F},
	{0x2B, 0x00}, {0x2C, 0x06}, {0x2D, 0xB4}, {0x2E, 0x00}, {0x2F, 0x13}, {0x30, 0xB6},
	{0x37, 0x00}, {0x83, 0x1F}, {0x84, 0x02}, {0x89, 0x01}, {0x8A, 0x0F}, {0x8B, 0xFF},
	{0x8E, 0x00}, {0x8F, 0x00}, {0x88, 0x40}};
char use_addr = 0, use_val = 0, use_num = 0;

int i2c_write(int fd, unsigned char *buf, int len)
{
	struct i2c_rdwr_ioctl_data ioctl_data;
	struct i2c_msg msgs[1];

	memset(&ioctl_data, 0, sizeof(ioctl_data));
	memset(msgs, 0, sizeof(msgs));

	msgs[0].addr 	= I2C_ADDR_SI5324;
	msgs[0].len 	= len;
	msgs[0].flags	&= ~I2C_M_RD;
	msgs[0].buf 	= buf;

	ioctl_data.msgs  = &msgs[0];
	ioctl_data.nmsgs = 1;
	//printf("i2c_write->[%d] : 0x%x \n", msgs[0].buf[0], msgs[0].buf[1]);
	if (ioctl(fd, I2C_RDWR, &ioctl_data) != 1) {
		close(fd);
		return -1;
	}

	return 0;
}

int i2c_read(int fd, unsigned char *buf, int len)
{
	struct i2c_rdwr_ioctl_data ioctl_data;
	struct i2c_msg msgs[1];

	memset(&ioctl_data, 0, sizeof(ioctl_data));
	memset(msgs, 0, sizeof(msgs));

	msgs[0].addr 	= I2C_ADDR_SI5324;
	msgs[0].len 	= len;
	msgs[0].flags	|= I2C_M_RD;
	msgs[0].buf 	= buf;

	ioctl_data.msgs  = &msgs[0];
	ioctl_data.nmsgs = 1;
	if (ioctl(fd, I2C_RDWR, &ioctl_data) != 1) {
		close(fd);
		return -1;
	}

	return 0;
}

static int parse_args(int argc, char **argv)
{
	if (argc < 2) {
		return DEFAULT;
	}
	else {
		if(strcmp(argv[1], "--debug") == 0) {
			return DEBUG;
		}
		else if (strcmp(argv[1], "w") == 0) {
			if (argc != 4){
				return -1;
			}
			else {
				use_addr = strtol(argv[2], NULL, 0);
				printf("use_addr = %d %x\n", use_addr, use_addr);
				use_val = strtol(argv[3], NULL, 0);
				printf("use_val = %d %x\n", use_val, use_val);
				return W;
			}	
		}
		else if (strcmp(argv[1], "r") == 0) {
			if (argc != 4){
				return -1;
			}
			else {
				use_addr = strtol(argv[2], NULL, 0);
				use_num = strtol(argv[3], NULL, 0);
				return R;
			}	
		}
	}
	return -1;
}

int main(int argc, char *argv[])
{
	int fd, i, rc;
	unsigned char buf[8];

	fd = open(I2C_DEVFILE, O_RDWR);
	if (fd < 0) {
		printf("open i2c failed !!!\n");
		return -1;
	}

	rc = parse_args(argc, argv); 
	
	if (rc == DEFAULT) {
		for(i = 0; i < REG_NUM; i++) {
			memset(buf, 0, sizeof(buf));
			buf[0] = addr_val[i][0]; 	//w_addr
			buf[1] = addr_val[i][1];	//w_val
			if (i2c_write(fd, buf, 2)) {
				printf("i2c_write_1: reg[%d] falid\n", addr_val[i][0]);
				return -1;
			}
		}
	}
	else if (rc == DEBUG) {
		/******************************************
		 * read si5324 reg(0~49)
		 ******************************************/
		for(i = 0; i < REG_NUM; i++) {
			memset(buf, 0, sizeof(buf));
			buf[0] = addr_val[i][0];
			if(i2c_write(fd, buf, 1)) {
				printf("i2c_write_2: reg[%d] faild\n", addr_val[i][0]);	
				return -1;
			}
		
			memset(buf, 0, sizeof(buf));
			if(i2c_read(fd, buf, 2)) {
				printf("i2c_read: reg[%d] faild\n", addr_val[i][0]);	
				return -1;
			}
			printf("[%x]: 0x%x \n", addr_val[i][0], buf[0]);
		}
	}
	else if (rc == W) {
			memset(buf, 0, sizeof(buf));
			buf[0] = use_addr; 	//w_addr
			buf[1] = use_val;	//w_val
			if (i2c_write(fd, buf, 2)) {
				printf("i2c_write_1: reg[%d] falid\n", use_addr);
				return -1;
			}
	}
	else if (rc == R) {
			for(i = 0; i < use_num; i++) {
				memset(buf, 0, sizeof(buf));
				buf[0] = use_addr + i;
				if(i2c_write(fd, buf, 1)) {
					printf("i2c_write_2: reg[%d] faild\n", use_addr + i);	
					return -1;
				}
			
				memset(buf, 0, sizeof(buf));
				if(i2c_read(fd, buf, 2)) {
					printf("i2c_read: reg[%d] faild\n", use_addr + i);	
					return -1;
				}
				printf("[%d]: 0x%x \n", use_addr + i, buf[0]);
			}
	}

	close(fd);

	return 0;
}
