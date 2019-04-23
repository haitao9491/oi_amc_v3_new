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

static void usage()
{
	fprintf(stderr, "Usage: clvwdctl [enable|disable].\n");
}

int main(int argc, char *argv[])
{
	int fd;
	char fname[32];
	int val = 0;

	if (argc != 2) {
		usage();
		return -1;
	}

	if (!strcmp(argv[1], "enable")) {
		val = 1;
	} else if (!strcmp(argv[1], "disable")) {
		val = 0;
	} else {
		usage();
		return -1;
	}

	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", CLOVES_NAME);

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		printf("open %s device err!\n", fname);
		return -1;
	}

	if (ioctl(fd, CLOVES_CTL_WATCHDOG, &val) == 0) {
		printf("watchdog status: %s\n", argv[1]);
	}

	close(fd);

	return 0;
}
