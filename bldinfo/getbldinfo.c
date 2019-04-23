/*
 * (C) Copyright 2015
 * Beijing HLYT Technology Co., Ltd.
 *
 * getbldinfo.c - A brief introduction goes here.
 *
 */

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include "apfrm.h"

char *shlib = NULL;

static void getbldinfo_show_usage(char *progname)
{
	printf("        --library|-l <name>: name of the shared library.\n");
}

static int getbldinfo_parse_args(int argc, char **argv)
{
	int i = 0;

	while (i < argc) {
		if ((strcmp(argv[i], "--library") == 0) ||
				(strcmp(argv[i], "-l") == 0)) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			shlib = argv[i];
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	if (shlib == NULL) {
		fprintf(stderr, "No shared library specified.\n");
		return -1;
	}

	ap_set_foreground();

	return 0;
}

static int getbldinfo_run(long instance, unsigned long data)
{
	int rc = -1;
	void *handle = NULL;
	char *error = NULL;
	const char *(*getBldInfo)(void);

	handle = dlopen(shlib, RTLD_LAZY);
	if ((error = dlerror()) == NULL) {
		getBldInfo = (const char *(*)(void))dlsym(handle, "GetBldInfo");
		if ((error = dlerror()) == NULL) {
			printf("%s %s\n", shlib, getBldInfo());
			rc = 0;
		}
	}

	if (error) {
		fprintf(stderr, "ERROR[%s]: %s\n", shlib, error);
	}
	if (handle) {
		dlclose(handle);
	}
	return rc;
}

static struct ap_framework getbldinfoapp = {
	NULL,

	getbldinfo_run,
	0ul,

	NULL,
	NULL,
	NULL,

	getbldinfo_show_usage,
	NULL,
	getbldinfo_parse_args
};

#if defined(__cplusplus)
extern "C" {
#endif
struct ap_framework *register_ap(void)
{
	return &getbldinfoapp;
}
#if defined(__cplusplus)
}
#endif
