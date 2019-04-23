/*
 *
 * cgiparse.c - Brief description of this file
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define REQUEST_METHOD_GET  1
#define REQUEST_METHOD_POST 2

#define CONTENT_TYPE_FORM_URLENCODED     1
#define CONTENT_TYPE_MULTIPART_FORM_DATA 2

struct cgiparse_hd {
	int   method;
	int   contentlen;
	int   contenttype;
	char *boundary;
	char *data;
};

static char *memstr(char *v, char *end, char *needle)
{
	char *p;

	while (1) {
		if (v >= end)
			break;

		if ((p = strstr(v, needle)) != NULL)
			return p;

		v += strlen(v);

		while ((v < end) && (*v == 0))
			v++;
	}

	return NULL;
}

static void release_hd(struct cgiparse_hd *hd)
{
	if (hd) {
		if (hd->boundary)
			free(hd->boundary);
		if (hd->data)
			free(hd->data);
		free(hd);
	}
}

static int parse_hex(char *hex, char *v)
{
	char hex1;
	char hex2;

	if (!isxdigit(*hex) || !isxdigit(*(hex + 1)))
		return -1;

	hex1 = *hex;
	hex2 = *(hex + 1);

	if ((hex1 >= 'A') && (hex1 <= 'F'))
		*v = ((hex1 - 'A') + 10) * 16;
	else if ((hex1 >= 'a') && (hex1 <= 'f'))
		*v = ((hex1 - 'a') + 10) * 16;
	else
		*v = (hex1 - '0') * 16;

	if ((hex2 >= 'A') && (hex2 <= 'F'))
		*v += ((hex2 - 'A') + 10);
	else if ((hex2 >= 'a') && (hex2 <= 'f'))
		*v += ((hex2 - 'a') + 10);
	else
		*v += (hex2 - '0');

	return 0;
}

static int parse_content_type(struct cgiparse_hd *hd)
{
	char *str, *ptr, *p;

	if ((str = getenv("CONTENT_LENGTH")) == NULL) {
		fprintf(stderr, "No CONTENT_LENGTH defined.\n");
		return -1;
	}

	hd->contentlen = strtol(str, &ptr, 0);
	if (*ptr || (hd->contentlen <= 0)) {
		fprintf(stderr, "Invalid CONTENT_LENGTH.\n");
		return -1;
	}

	if ((str = getenv("CONTENT_TYPE")) == NULL) {
		fprintf(stderr, "No CONTENT_TYPE defined.\n");
		return -1;
	}

	if (strcmp(str, "application/x-www-form-urlencoded") == 0) {
		hd->contenttype = CONTENT_TYPE_FORM_URLENCODED;
	}
	else if (strncmp(str, "multipart/form-data;", 20) == 0) {
		ptr = strstr(str, "boundary=");
		if (!ptr ||
		    (strstr(ptr + 1, "boundary=") != NULL) ||
		    (*(ptr + 9) == 0)) {
			fprintf(stderr, "CONTENT_TYPE %s not supported.\n",str);
			return -1;
		}

		ptr += 9;
		while(*ptr == '-')
			ptr++;
		p = ptr;
		while(*p)
			p++;
		if ((*ptr == 0) || *p) {
			fprintf(stderr, "CONTENT_TYPE %s not supported.\n",str);
			return -1;
		}

		hd->boundary = strdup(ptr);
		if (!hd->boundary) {
			fprintf(stderr, "Insufficient memory.\n");
			return -1;
		}
		hd->contenttype = CONTENT_TYPE_MULTIPART_FORM_DATA;
	}
	else {
		fprintf(stderr, "CONTENT_TYPE %s not supported.\n", str);
		return -1;
	}

	return 0;
}

void *cgiparse_open(char *method, char *data)
{
	struct cgiparse_hd *hd;

	hd = (struct cgiparse_hd *)malloc(sizeof(struct cgiparse_hd));
	if (!hd) {
		fprintf(stderr, "Insufficient memory.\n");
		return NULL;
	}
	memset(hd, 0, sizeof(struct cgiparse_hd));

	/* parse request_method */
	if (!method)
		method = getenv("REQUEST_METHOD");

	if (strcmp(method, "GET") == 0)
		hd->method = REQUEST_METHOD_GET;
	else if (strcmp(method, "POST") == 0)
		hd->method = REQUEST_METHOD_POST;
	else {
		fprintf(stderr, "Invalid request_method %s.\n", method);
		goto free_hd_exit;
	}

	if (hd->method == REQUEST_METHOD_GET) {
		if (!data) {
			data = getenv("QUERY_STRING");
			if (!data) {
				fprintf(stderr, "No QUERY_STRING defined.\n");
				goto free_hd_exit;
			}
		}
		hd->data = strdup(data);
	}
	else { /* POST */
		if (parse_content_type(hd) < 0)
			goto free_hd_exit;

		if (hd->contentlen > 0) {
			hd->data = (char *)malloc(hd->contentlen + 1);
			if (hd->data) {
				if (fread(hd->data, hd->contentlen, 1, stdin) != 1) {
					fprintf(stderr, "Read stdin failed.\n");
					goto free_hd_exit;
				}
				*(hd->data + hd->contentlen) = 0;
			}
		}
		else
			hd->data = NULL;
	}

	if (!hd->data) {
		fprintf(stderr, "Insufficient memory.\n");
		goto free_hd_exit;
	}

	return hd;

free_hd_exit:
	release_hd(hd);
	return NULL;
}

static int cgiparse_getvalue_multipart(struct cgiparse_hd *hd,
		char *fieldname, char *value, int size, int seq)
{
	char *p, *v, *b;
	int   flen, qlen, blen;
	int   found = 0;
	int   cnt = 0;

	flen = strlen(fieldname);
	blen = strlen(hd->boundary);

	v = hd->data;
	while(*v) {
		/* search for boundary */
		while(*v == '-')
			v++;
		if (strncmp(v, hd->boundary, blen))
			return -1;
		v += blen;
		if (*v++ != 0x0d)
			return -1;
		if (*v++ != 0x0a)
			return -1;

		/* Process Content-Disposition directives */
		if (strncmp(v, "Content-Disposition: form-data; name=\"", 38))
			return -1;
		v += 38;
		if (*v == 0)
			return -1;
		p = v;
		while (*p && (*p != '"'))
			p++;
		if (*p != '"')
			return -1;
		if ((flen == (p - v)) && (strncmp(v, fieldname, flen) == 0)) {
			cnt++;
			if (cnt == seq)
				found = 1;
		}
		v = p + 1;
		while (*v && (*v != 0x0d))
			v++;
		if (*v != 0x0d)
			return -1;
		v++;
		if (*v++ != 0x0a)
			return -1;
		if (*v == 0)
			return -1;

		/* Process other Content-* directives */
		while(*v != 0x0d) {
			while (*v && (*v != 0x0d))
				v++;
			if (*v != 0x0d)
				return -1;
			v++;
			if (*v++ != 0x0a)
				return -1;
			if (*v == 0)
				return -1;
		}
		v++;
		if (*v++ != 0x0a)
			return -1;
		if (*v == 0)
			return -1;

		/* Now v points to the value */
		if ((p = memstr(v, hd->data + hd->contentlen, hd->boundary)) == NULL)
			return -1;
		b = p;
		p--;
		while ((p > v) && (*p == '-'))
			p--;
		if (p <= v)
			return -1;
		if (*p-- != 0x0a)
			return -1;
		if (*p != 0x0d)
			return -1;
		if (p < v)
			return -1;

		if (found) {
			if (p == v)
				return 0;

			qlen = p - v;

			if (value) {
				if (qlen >= size)
					return -1;
				memcpy(value, v, qlen);
			}
			else {
				if (fwrite(v, qlen, 1, stdout) != 1)
					return -1;
			}

			return qlen;
		}

		v = b;
	}

	return 0;
}

static int cgiparse_getvalue_urlencoded(struct cgiparse_hd *hd,
		char *fieldname, char *value, int size, int seq)
{
	char *tmpvalue, *p, *v;
	int   flen, qlen;
	int   cnt = 0;

	flen = strlen(fieldname);
	qlen = strlen(hd->data);

	p = v = hd->data;
	while(*v) {
		p = strstr(v, fieldname);
		if (!p)
			return -1;

		if (((p > hd->data) && (*(p - 1) != '&')) ||
		    (*(p + flen) != '=')) {
			v = p + 1;
			continue;
		}

		cnt++;
		if (cnt == seq)
			break;

		v = p + 1;
	}

	if (!(*v))
		return -1;

	p += (flen + 1);

	tmpvalue = (char *)malloc(qlen);
	if (!tmpvalue)
		return -1;
	v = tmpvalue;

	memset(tmpvalue, 0, qlen);

	while(*p && (*p != '&')) {
		if (*p == '+') {
			*v = ' ';
		}
		else if (*p == '%') {
			if (parse_hex(p + 1, v)) {
				free(tmpvalue);
				return -1;
			}
			p += 2;
		}
		else
			*v = *p;

		p++;
		v++;
	}

	flen = strlen(tmpvalue);
	if (flen > 0) {
		if (value) {
			if (flen >= size) {
				free(tmpvalue);
				return -1;
			}
			strcpy(value, tmpvalue);
		}
		else {
			printf("%s\n", tmpvalue);
		}
	}
	free(tmpvalue);

	return flen;
}

int cgiparse_getvalue(void *h, char *fieldname, char *value, int size, int seq)
{
	struct cgiparse_hd *hd = (struct cgiparse_hd *)h;

	if (!h || !fieldname)
		return -1;

	if ((hd->method == REQUEST_METHOD_GET) ||
	    ((hd->method == REQUEST_METHOD_POST) &&
	     (hd->contenttype == CONTENT_TYPE_FORM_URLENCODED)))
		return cgiparse_getvalue_urlencoded(hd,
				fieldname, value, size, seq);

	if ((hd->method == REQUEST_METHOD_POST) &&
	    (hd->contenttype == CONTENT_TYPE_MULTIPART_FORM_DATA))
		return cgiparse_getvalue_multipart(hd,
				fieldname, value, size, seq);

	return -1;
}

void cgiparse_close(void *h)
{
	struct cgiparse_hd *hd = (struct cgiparse_hd *)h;

	if (hd) {
		if (hd->data)
			free(hd->data);
		free(hd);
	}
}
