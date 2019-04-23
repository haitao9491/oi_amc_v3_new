/*
 *
 * cmdsvr.c - A brief description to describe this file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include "os.h"
#include "aplog.h"
#include "apfrm.h"
#include "cconfig.h"
#include "adapter.h"
#include "adapter_file.h"
#include "adapter_cs.h"
#include "adapter_ss.h"
#include "pktcmd.h"
#define CMDSVR_APFRM
#include "cmdsvr.h"

char *cfgfile = NULL;
char *section = "Command Server";

#define ADAPTER_TYPE_FILE     0
#define ADAPTER_TYPE_CS       1
#define ADAPTER_TYPE_SS       2

int   adap_type = ADAPTER_TYPE_SS;
void *adap = NULL;
void *pktcmd = NULL;
struct cmdsvr_device *dev = NULL;

static void cmdsvr_show_usage(char *progname)
{
	printf("        --cfgfile <filename>: configuration file\n");
	printf("        --type <file|cs|ss>: type, default to ss\n");
	printf("        --section <name>: name of the configuration section\n");
}

static void cmdsvr_show_version(char *progname)
{
	printf("CmdSvr - %s\n", "V0.1");
}

static int cmdsvr_parse_args(int argc, char **argv)
{
	int i = 0;

	while (i < argc) {
		if (strcmp(argv[i], "--cfgfile") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			cfgfile = argv[i];
		}
		else if (strcmp(argv[i], "--section") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			section = argv[i];
		}
		else if (strcmp(argv[i], "--type") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

			i++;
			if (strcmp(argv[i], "file") == 0)
				adap_type = ADAPTER_TYPE_FILE;
			else if (strcmp(argv[i], "cs") == 0)
				adap_type = ADAPTER_TYPE_CS;
			else if (strcmp(argv[i], "ss") == 0)
				adap_type = ADAPTER_TYPE_SS;
			else {
				fprintf(stderr, "Invalid option: %s\n",argv[i]);
				return -1;
			}
		}
		else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			return -1;
		}
		i++;
	}

	if (cfgfile == NULL) {
		fprintf(stderr, "No cfgfile specified.");
		return -1;
	}

	return 0;
}

int cmdsvr_init(void)
{
	unsigned long hd;
	struct cmdsvr_callback *cb;

	if ((hd = CfgInitialize(cfgfile)) == 0ul) {
		LOGERROR("Parsing configuration file [%s] failed.", cfgfile);
		return -1;
	}

	if (adap_type == ADAPTER_TYPE_FILE)
		adap = adapter_register_file(hd, section);
	else if (adap_type == ADAPTER_TYPE_CS)
		adap = adapter_register_cs(hd, section, ap_is_running);
	else if (adap_type == ADAPTER_TYPE_SS)
		adap = adapter_register_ss(hd, section, ap_is_running);
	if (adap == NULL) {
		CfgInvalidate(hd);
		return -1;
	}
	adapter_open(adap);

	pktcmd = pktcmd_open(adap, dev->device);
	if (pktcmd == NULL) {
		adapter_close(adap);
		CfgInvalidate(hd);
		return -1;
	}
	for (cb = dev->devcbs; cb && cb->id; cb++) {
		pktcmd_register_devhdl(pktcmd, cb->id, cb->cmdcb, cb->ackcb);
	}

	CfgInvalidate(hd);
	return 0;
}

void cmdsvr_exit(void)
{
	if (pktcmd) {
		pktcmd_close(pktcmd);
		pktcmd = NULL;
	}
	if (adap) {
		adapter_close(adap);
		adap = NULL;
	}
}

static int cmdsvr_run(long instance, unsigned long data)
{
	dev = cmdsvr_register_device();
	if (dev == NULL) {
		LOGERROR("Registering device to cmdsvr failed.");
		return -1;
	}

	if (cmdsvr_init() < 0) {
		cmdsvr_release_device(dev);
		return -1;
	}

	while (ap_is_running()) {
		if ((pktcmd_process(pktcmd) <= 0) &&
				((dev->send && (*(dev->send))(pktcmd)) <= 0)) {
			SLEEP_US(100000);
		}
	}

	cmdsvr_exit();
	cmdsvr_release_device(dev);

	return 0;
}

static struct ap_framework cmdsvrapp = {
	NULL,
	cmdsvr_run,
	0ul,
	NULL,
	NULL,
	NULL,
	cmdsvr_show_usage,
	cmdsvr_show_version,
	cmdsvr_parse_args,
};

struct ap_framework *register_ap(void)
{
	return &cmdsvrapp;
}
