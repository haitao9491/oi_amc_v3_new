/*
 * (C) Copyright 2015
 *
 * fpgaload.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include "fpgaload.h"

#define FPGALOAD_BUFSZ		4096
#define FPGALOAD_DONECHECK_TRY	10


void usage (char *program) 
{
	printf("Usage: %s <id> <image file>\n", basename(program));
	printf("    id : The No. of FPGA.\n");
}

int main(int argc, char **argv)
{
	int id;
	char *imgfile, fname[32];
	int fd = 0;
	FILE *fp = NULL;
	struct stat st;
	unsigned int  image_size;
	unsigned char buf[FPGALOAD_BUFSZ];
	int num_blocks, last_block = 0, i;
	struct fpga_cfg_info cfg;
	int rc = -1, done = 0;

	if (argc != 3) {
		usage(argv[0]);
		return -1;
	}
	
	id = atoi(argv[1]);
	if (id < 0) {
		fprintf(stderr, "Invalid id: %s\n", argv[1]);
		return -1;
	}

	imgfile = argv[2];

	memset(&st, 0, sizeof(st));
	if (stat(imgfile, &st) != 0) {
		fprintf(stderr, "Failed to get size of file %s\n", imgfile);
		return -1;
	}
	if (st.st_size == 0) {
		fprintf(stderr, "Empty image file %s\n", imgfile);
		return -1;
	}
	image_size = st.st_size;

	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", DEV_NAME);

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Failed to open device %s\n", fname);
		return -1;
	}

	fp = fopen(imgfile, "r");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open image file %s\n", imgfile);
		goto exit;
	}

	num_blocks = image_size / FPGALOAD_BUFSZ;
	last_block = (image_size % FPGALOAD_BUFSZ) ? : 0;

	/* Loading */
	memset(&cfg, 0, sizeof(cfg));
	cfg.index = id;
	if (ioctl(fd, DEV_CFG_FPGA_START, &cfg) != 0) {
		fprintf(stderr, "Failed to start configuration of fpga[%d].\n", id);
		goto exit;
	}

	for (i = 0; i < num_blocks; i++) {
		if (fread(buf, FPGALOAD_BUFSZ, 1, fp) != 1) {
			fprintf(stderr, "Failed to read image file %s: block %d\n", imgfile, i);
			goto exit;
		}

		memset(&cfg, 0, sizeof(cfg));
		cfg.index = id;
		cfg.data = buf;
		cfg.len = FPGALOAD_BUFSZ;
		
		if (ioctl(fd, DEV_CFG_FPGA_LOAD, &cfg) != 0) {
			fprintf(stderr, "Failed to load image file %s: block %d\n", imgfile, i);
			goto exit;
		}
	}
	if (last_block > 0) {
		if (fread(buf, last_block, 1, fp) != 1) {
			fprintf(stderr, "Failed to read image file %s: block %d\n", imgfile, i);
			goto exit;
		}

		memset(&cfg, 0, sizeof(cfg));
		cfg.index = id;
		cfg.data = buf;
		cfg.len = last_block;
		
		if (ioctl(fd, DEV_CFG_FPGA_LOAD, &cfg) != 0) {
			fprintf(stderr, "Failed to load image file %s: block %d\n", imgfile, i);
			goto exit;
		}

	}

	for (i = 0; i < FPGALOAD_DONECHECK_TRY; i++) {
		memset(&cfg, 0, sizeof(cfg));
		cfg.index = id;

		if (ioctl(fd, DEV_CFG_FPGA_DONE, &cfg) == 0) {
			done = 1;
			break;
		}
		else {
			memset(buf, 'a', FPGALOAD_BUFSZ);
			cfg.data = buf;
			cfg.len = FPGALOAD_BUFSZ;

			if (ioctl(fd, DEV_CFG_FPGA_LOAD, &cfg) != 0) {
				fprintf(stderr, "Failed to load pading data: %d\n", i);
			}
		}
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.index = id;
	if (ioctl(fd, DEV_CFG_FPGA_STOP, &cfg) != 0) {
		fprintf(stderr, "Failed to stop configuration of fpga[%d]\n", id);
	}

	if (done)
		rc = 0;

exit:
	if (fp)
		fclose(fp);
	if (fd)
		close(fd);

	return rc;
}

