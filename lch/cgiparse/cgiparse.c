/*
 *
 * cgiparse.c - Brief description of this file
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "libcgiparse.h"

char *formdata = NULL;
char *fieldname = NULL;
char *reqmethod = "GET";
int   fieldseq = 1;

void cgiparse_exit(const char *msg, int ret)
{
	if (msg)
		fprintf(stderr, "%s\n", msg);

	if (fieldname)
		free(fieldname);

	if (formdata)
		free(formdata);

	exit(ret);
}

void usage(void)
{
	printf("Usage: cgiparse --help\n");
	printf("       cgiparse --field name [method] [seq] [data]\n");
}

void parse_args(int argc, char **argv)
{
	if (argc <= 1)
		cgiparse_exit("No arguments.", 1);

	if (strcmp(argv[1], "--help") == 0) {
		usage();
		cgiparse_exit(NULL, 0);
	}
	else if (strcmp(argv[1], "--field") == 0) {
		if ((argc < 3) || (argc > 6))
			cgiparse_exit("Invalid option --field.", 1);

		fieldname = strdup(argv[2]);
		if (!fieldname)
			cgiparse_exit("Insufficient memory.", 1);

		if (argc >= 4)
			reqmethod = argv[3];

		if (argc >= 5)
			fieldseq = atoi(argv[4]);

		if (argc >= 6) {
			formdata = strdup(argv[5]);
			if (!formdata)
				cgiparse_exit("Insufficient memory.", 1);
		}
	}
	else {
		cgiparse_exit("Unrecognized arguments.", 1);
	}
}

int
main(int argc, char **argv)
{
	void *hd;
	int   len;

	parse_args(argc, argv);

	hd = cgiparse_open(reqmethod, formdata);
	if (!hd) {
		cgiparse_exit("cgiparse_open failed.", 1);
	}

	len = cgiparse_getvalue(hd, fieldname, NULL, 0, fieldseq);
	if (len < 0) {
		cgiparse_exit("cgiparse_getvalue failed.", 1);
	}

	cgiparse_close(hd);

	return 0;
}
