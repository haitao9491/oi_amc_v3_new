/*
 * (C) Copyright 2011
 * <www.sycomm.cn>
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "cloves.h"

int main(int argc, char *argv[])
{
	int fd;
	char fname[32];
	unsigned int val = 0;

	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", CLOVES_NAME);

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		return -1;
	}

	if (ioctl(fd, CLOVES_GET_DIALCODE, &val) == 0) {
		printf("%u\n", val);
	}

	close(fd);

	return 0;
}
