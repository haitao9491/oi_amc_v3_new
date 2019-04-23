/*
 *
 * filter.c
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "os.h"
#include "aplog.h"
#include "string.h"
#include "filter.h"
#include "cconfig.h"

#define COMPOSE_HEAD_SIZE		4
#define SINGLE_HEAD_SIZE		8

typedef struct FILTER {
	unsigned char  type;
	unsigned char  non;
	unsigned int   len;
	unsigned short option_type;
	unsigned char  op;
	unsigned char  value_type;
	unsigned int   value_len;
	unsigned char *value;
	struct FILTER *lchild;
	struct FILTER *rchild;
} sFilter;

enum {
	FILTER_SINGLE		 = 1,
	FILTER_COMPOSITE_AND,
	FILTER_COMPOSITE_OR
};

static const char *DataTypeString[] = {
	"---INVALID---",
	"Byte1",
	"Byte2",
	"Byte4",
	"Char String",
	"Octet String",
	NULL
};

static const char *OperString[] = {
	"---INVALID---",
	"==",
	"!=",
	"<",
	"<=",
	">",
	">=",
	"STRCMP",
	"STRNCMP",
	"OCTCMP",
	"OCTNCMP",
	NULL
};

static const char *TypeString[] = {
	"---INVALID---",
	"IMEI",
	"IMSI",
	"TMSI",
	"DLCI",
	"BVCI",
	"SRC_IP_ADDR",
	"DST_IP_ADDR",
	"CDR_TYPE",
	"CDR_RESULT",
	"OPC",
	"DPC",
	"OPP_NO",
	"SRC_LAC",
	"SRC_CI",
	"DST_LAC",
	"DST_CI",
	"TS",
	"CDRID",
	"DEVICE",
	"CHANNEL",
	"OGT",
	"DGT",
	"OPCODE",
	"SVRID",
	"SMS_CENTER",
	"MTP_SIO",
	NULL
};

char *filter_get_datatype(unsigned char datatype)
{
	if (datatype > sizeof(DataTypeString))
		return NULL;

	return (char *) DataTypeString[datatype];
}

char *filter_get_oper(unsigned char op)
{
	if (op > sizeof(OperString))
		return NULL;

	return (char *) OperString[op];
}

char *filter_get_type(enum filter_option type)
{
	if (type > sizeof(TypeString))
		return NULL;

	return (char *) TypeString[type];
}

static void filter_single_logic(sFilter *filter,
		char *buf, int *len, int *size)
{
	unsigned int i = 0;
	int pos = 0;
	char soct[64];
	int slot = 0;
	char tmp[128];
	unsigned long long value = 0;

	if (*size < 30)
		return;

	memset(tmp, 0, sizeof(tmp));
	buf[slot] = '(';
	slot++;
	if (filter->value_type == DATATYPE_CHAR_STRING) {
		sprintf(tmp, "%s %s %s", filter_get_type(filter->option_type),
				filter_get_oper(filter->op), filter->value);
	}
	else if (filter->value_type == DATATYPE_OCTET_STRING) {
		for (i = 0; i < filter->value_len; i++) {
			pos += sprintf(soct + pos, "%x", *(filter->value + i));
		}
		soct[pos] = 0;
		sprintf(tmp, "%s %s %s", filter_get_type(filter->option_type),
				filter_get_oper(filter->op), soct);
	} else {
		if (filter->value_type == DATATYPE_BYTE1) {
			value = (unsigned long long)(*(unsigned char *)filter->value);
		} else if (filter->value_type == DATATYPE_BYTE2) {
			value = (unsigned long long)(*(unsigned short *)filter->value);
		} else if (filter->value_type == DATATYPE_BYTE4) {
			value = (unsigned long long)(*(unsigned int *)filter->value);
		} else if (filter->value_type == DATATYPE_BYTE8) {
			value = *(unsigned long long *)filter->value;
		} else {
			return;
		}
		sprintf(tmp, "%s %s %llu", filter_get_type(filter->option_type),
				filter_get_oper(filter->op), value);
	}

	slot += (int)strlen(tmp);
	if (slot <= *size) {
		memcpy(buf + 1, tmp, strlen(tmp));
	}
	if (slot + 1 <= *size) {
		buf[slot] = ')';
		slot++;
	}
	*len = slot;
	*size -= slot;
}

static void filter_traversal_logic(sFilter *filter,
		char *buf, int *len, int *size);

static void filter_or_logic(sFilter *filter,
		char *buf, int *len, int *size)
{
	int slot = 0;

	buf[slot] = '(';
	slot++;
	if (filter->lchild) {
		filter_traversal_logic(filter->lchild, buf + slot, len, size);
	}
	slot += *len;
	buf[slot] = ' ';
	buf[slot + 1] = '|';
	buf[slot + 2] = '|';
	buf[slot + 3] = ' ';
	slot += 4;
	if (filter->lchild) {
		filter_traversal_logic(filter->rchild, buf + slot, len, size);
	}
	slot += *len;
	buf[slot] = ')';
	slot++;
	*len = slot;
}

static void filter_and_logic(sFilter *filter,
		char *buf, int *len, int *size)
{
	int slot = 0;

	buf[slot] = '(';
	slot++;
	if (filter->lchild) {
		filter_traversal_logic(filter->lchild, buf + slot, len, size);
	}
	slot += *len;
	buf[slot] = ' ';
	buf[slot + 1] = '&';
	buf[slot + 2] = '&';
	buf[slot + 3] = ' ';
	slot += 4;
	if (filter->lchild) {
		filter_traversal_logic(filter->rchild, buf + slot, len, size);
	}
	slot += *len;
	buf[slot] = ')';
	slot++;
	*len = slot;
}

static void filter_traversal_logic(sFilter *filter,
		char *buf, int *len, int *size)
{
	int i = 0;

	if (filter == NULL)
		return;

	if (filter->non == 1) {
		buf[0] = '!';
		i = 1;
	}

	if (filter->type == FILTER_SINGLE) {
		filter_single_logic(filter, buf + i, len, size);
	}
	else if (filter->type == FILTER_COMPOSITE_OR) {
		filter_or_logic(filter, buf + i, len, size);
	}
	else if (filter->type == FILTER_COMPOSITE_AND) {
		filter_and_logic(filter, buf + i, len, size);
	}

	*len += i;
	*size -= i;

	return;
}

char *filter_get_expression(void *filter, char *buf, int size)
{
	int len = 0;
	sFilter *f = (sFilter *)filter;

	filter_traversal_logic(f, buf, &len, &size);
	buf[len] = 0;

	return buf;
}

void *filter_single(enum filter_option type, unsigned char op,
				unsigned char datatype, void *value, void *value_ex)
{
	sFilter *single = NULL;

	single = (sFilter *)malloc(sizeof(sFilter));
	if (single == NULL) {
		LOGERROR("Filter Single: Allocate Memory Failed.");
		return NULL;
	}

	single->type		 = FILTER_SINGLE;
	single->non 		 = 0;
	single->option_type	 = type;
	single->op			 = op;
	single->value_type	 = datatype;

	single->lchild = NULL;
	single->rchild = NULL;
	single->value  = NULL;

	if (datatype == DATATYPE_BYTE1) {
		single->value_len = 1;
	} else if (datatype == DATATYPE_BYTE2) {
		single->value_len = 2;
	} else if (datatype == DATATYPE_BYTE4) {
		single->value_len = 4;
	} else if (datatype == DATATYPE_BYTE8) {
		single->value_len = 8;
	} else if (datatype == DATATYPE_CHAR_STRING) {
		single->value_len = (unsigned int)strlen(value) + 1;
	} else if (datatype == DATATYPE_OCTET_STRING) {
		if (value_ex == NULL) {
			filter_release(single);
			return NULL;
		}
		single->value_len = (unsigned int)(*(unsigned int *)value_ex);
	}
	single->value  = (unsigned char *)malloc(single->value_len);
	if (single->value == NULL) {
		filter_release(single);
		LOGERROR("Filter Single: Allocate Memory Failed.");
		return NULL;
	}
	memcpy(single->value, value, single->value_len);

	single->len = single->value_len + SINGLE_HEAD_SIZE;

	return (void *)single;
}

void *filter_non(void *filter)
{
	sFilter *f = (sFilter *)filter;
	if (f != NULL)
		f->non = 1;

	return (void *)f;
}

static int filter_compose_nr(sFilter *fl, sFilter *fc1,
		sFilter *fr, sFilter *fc2)
{
	if ((fc1 == NULL) && (fc2 == NULL))
		return -1;

	if (fc1) {
		fl = (sFilter *)malloc(fc1->len);
		if (fl == NULL) {
			LOGERROR("Filter Compose lchild: Allocate Memory Failed.");
			return -1;
		}
		memcpy(fl, fc1, fc1->len);
		if (fl->type == FILTER_SINGLE) {
			fl->value = (unsigned char *)malloc(fc1->value_len);
			if (fl->value == NULL) {
				LOGERROR("Filter Compose value: Allocate Memory Failed.");
				filter_release(fl);
				return -1;
			}
			memcpy(fl->value, fc1->value, fc1->value_len);
		} else if (filter_compose_nr(fl->lchild, fc1->lchild,
					fl->rchild, fc1->rchild) < 0) {
			filter_release(fl);
			return -1;
		}
	}

	if (fc2) {
		fr = (sFilter *)malloc(fc2->len);
		if (fr == NULL) {
			LOGERROR("Filter Compose rchild: Allocate Memory Failed.");
			return -1;
		}
		memcpy(fr, fc2, fc2->len);
		if (fr->type == FILTER_SINGLE) {
			fr->value = (unsigned char *)malloc(fc2->value_len);
			if (fr->value == NULL) {
				LOGERROR("Filter Compose value: Allocate Memory Failed.");
				filter_release(fr);
				return -1;
			}
			memcpy(fr->value, fc2->value, fc2->value_len);
		} else if (filter_compose_nr(fr->lchild, fc2->lchild,
					fr->rchild, fc2->rchild) < 0) {
			filter_release(fr);
			return -1;
		}
	}

	return 0;
}

static void *filter_compose(void *fc1, void *fc2, int release)
{
	sFilter *cps = NULL;
	sFilter *fcl = (sFilter *)fc1;
	sFilter *fcr = (sFilter *)fc2;

	if ((fc1 == NULL) || (fc2 == NULL)) {
		if (release > 0) {
			filter_release(fc1);
			filter_release(fc2);
		}
		return NULL;
	}

	cps = (sFilter *)malloc(sizeof(sFilter));
	if (cps == NULL) {
		LOGERROR("Filter Compose: Allocate Memory Failed.");
		if (release > 0) {
			filter_release(fc1);
			filter_release(fc2);
		}
		return NULL;
	}
	memset(cps, 0, sizeof(sFilter));

	cps->len = COMPOSE_HEAD_SIZE + fcl->len + fcr->len;
	if (release > 0) {
		cps->lchild = fcl;
		cps->rchild = fcr;
	} else if (filter_compose_nr(cps->lchild, fcl, cps->rchild, fcr) < 0) {
		filter_release(fc1);
		filter_release(fc2);
		filter_release(cps);
		return NULL;
	}

	return cps;
}

void *filter_or(void *fc1, void *fc2, int release)
{
	sFilter *or = NULL;

	or = filter_compose(fc1, fc2, release);
	if (or != NULL) {
		or->type = FILTER_COMPOSITE_OR;
		or->non  = 0;
	}

	return or;
}

void *filter_and(void *fc1, void *fc2, int release)
{
	sFilter *and = NULL;

	and = filter_compose(fc1, fc2, release);
	if (and != NULL) {
		and->type = FILTER_COMPOSITE_AND;
		and->non = 0;
	}

	return and;
}

static void *filter_traversal_decode(sFilter *f, unsigned char *buf, int *len)
{
	*len = 0;
	f->type = buf[0];
	f->non = buf[1];
	f->len  = ((unsigned int)(buf[2] << 8)) + buf[3];

	if (f->type == FILTER_SINGLE) {
		f->lchild = NULL;
		f->rchild = NULL;
		f->value_type  = (buf[4] >> 4) & 0x0f;
		f->op		   = buf[4] & 0x0f;
		f->option_type = ((buf[5] << 8) & 0xff) | (buf[6] & 0xff);
		f->value_len   = buf[7] & 0xff;
		f->value = (unsigned char *)malloc(f->value_len);
		if (f->value == NULL) {
			LOGERROR("Filter Decode: Allocate Memory Failed.");
			filter_release(f);
			return NULL;
		}
		memcpy(f->value, buf + 8, f->value_len);

		*len = f->len;
		if (f->len != (f->value_len + SINGLE_HEAD_SIZE)) {
			LOGERROR("Filter Decode: Allocate Memory Failed.");
			filter_release(f);
			return NULL;
		}

		return (void *)f;
	}

	if ((f->type == FILTER_COMPOSITE_OR) || (f->type == FILTER_COMPOSITE_AND))
	{
		f->lchild = (sFilter *)malloc(sizeof(sFilter));
		if (f->lchild == NULL) {
			LOGERROR("Filter Decode: Allocate Memory Failed.");
			filter_release(f);
			return NULL;
		}
		memset(f->lchild, 0, sizeof(sFilter));
		if ((f->lchild = filter_traversal_decode(f->lchild,
					buf + *len + COMPOSE_HEAD_SIZE, len)) == NULL) {
			filter_release(f);
			return NULL;
		}

		f->rchild = (sFilter *)malloc(sizeof(sFilter));
		if (f->rchild == NULL) {
			LOGERROR("Filter Decode: Allocate Memory Failed.");
			filter_release(f);
			return NULL;
		}
		memset(f->rchild, 0, sizeof(sFilter));
		if ((f->rchild = filter_traversal_decode(f->rchild,
					buf + *len + COMPOSE_HEAD_SIZE, len)) == NULL) {
			filter_release(f);
			return NULL;
		}

		*len = f->len;

		if (f->len != (f->lchild->len + f->rchild->len + COMPOSE_HEAD_SIZE))
		{
			LOGERROR("Filter Length Error");
			filter_release(f);
			return NULL;
		}

		return (void *)f;
	}

	return NULL;
}

static unsigned char *filter_traversal_encode(sFilter *f,
		unsigned char *buf, int *len)
{
	if (f->type == FILTER_SINGLE) {
		buf[0] = FILTER_SINGLE & 0xff;
		buf[1] = f->non & 0xff;
		buf[2] = (f->len >> 8) & 0xff;
		buf[3] = f->len & 0xff;
		buf[4] = (((f->value_type & 0x0f) << 4) | (f->op & 0x0f));
		buf[5] = (f->option_type >> 8) & 0xff;
		buf[6] = f->option_type & 0xff;
		buf[7] = f->value_len & 0xff;
		memcpy(buf + 8, f->value, f->value_len);
		*len += f->len;

		return buf;
	}

	if ((f->type != FILTER_COMPOSITE_OR) && (f->type != FILTER_COMPOSITE_AND))
	{
		return NULL;
	}

	buf[0] = f->type & 0xff;
	buf[1] = f->non & 0xff;
	buf[2] = (f->len >> 8) & 0xff;
	buf[3] = f->len & 0xff;
	*len += COMPOSE_HEAD_SIZE;

	if (f->lchild) {
		filter_traversal_encode(f->lchild, buf + COMPOSE_HEAD_SIZE, len);
	}
	if (f->rchild) {
		filter_traversal_encode(f->rchild, buf +
				f->lchild->len + COMPOSE_HEAD_SIZE, len);
	}

	return buf;
}

unsigned char *filter_encode(void *filter, int reserved, int *len)
{
	sFilter *f = (sFilter *)filter;
	unsigned char *buf = NULL;

	*len = 0;

	if (f == NULL)
		return NULL;

	buf = (unsigned char *)malloc(reserved + f->len);
	if (buf == NULL) {
		LOGERROR("Filter Encode: Allocate Memory Failed.");
		return NULL;
	}
	memset(buf, 0, reserved + f->len);

	return filter_traversal_encode(f, buf + reserved, len);
}

int filter_encode_ext(void *filter, unsigned char *buf, int size)
{
	unsigned char *p;
	int len = 0;
	
	p = filter_encode(filter, 0, &len);
	if (p) {
		if (len <= size) {
			memcpy(buf, p, len);
		}
		else {
			len = 0;
		}
		free(p);
	}

	return len;
}

void *filter_decode(unsigned char *buf, int len)
{
	int flen = 0;
	sFilter *f = NULL;

	if ((buf == NULL) || (len <= 0))
		return NULL;

	f = (sFilter *)malloc(sizeof(sFilter));
	if (f == NULL) {
		LOGERROR("Filter Decode: Allocate Memory Failed.");
		return NULL;
	}
	memset(f, 0, sizeof(sFilter));

	f = (sFilter *)filter_traversal_decode(f, buf, &flen);
	if (len != flen) {
		LGWRERROR(buf, len, "Decode Filter Error! %u != %u", len, flen);
		filter_release(f);
		return NULL;
	}

	return (void *)f;
}

void filter_release(void *fc)
{
	sFilter *f = (sFilter *)fc;

	if (f == NULL)
		return;

	if (f->type == FILTER_SINGLE) {
		if (f->value)
			free(f->value);
	}

	if (f->lchild) {
		filter_release(f->lchild);
		f->lchild = NULL;
	}

	if (f->rchild) {
		filter_release(f->rchild);
		f->rchild = NULL;
	}

	free(f);
	f = NULL;
}

int filter_traversing(void *filter, void *arg,
			void (*callback)(enum filter_option type, void *arg))
{
	sFilter	*f = (sFilter *)filter;

	if (f == NULL)
		return -1;

	if (f->type == FILTER_SINGLE) {
		(*callback)((enum filter_option)f->option_type, arg);
		return 0;
	}

	if (f->lchild)
		filter_traversing(f->lchild, arg, callback);

	if (f->rchild)
		filter_traversing(f->rchild, arg, callback);

	return 0;
}

int filter_logic_result(void *filter, void *arg,
		int (*compare)(void *arg, enum filter_option type,
			unsigned char op, void *value, void *value_ex))
{
	sFilter	*f = (sFilter *)filter;
	int rc = 0;

	if (f == NULL)
		return -1;

	if (f->type == FILTER_SINGLE) {
		rc = (*compare)(arg, (enum filter_option)f->option_type, f->op,
				(void *)f->value, (void *)&(f->value_len));
	}
	else if (f->type == FILTER_COMPOSITE_OR) {
		if ((f->lchild == NULL) || (f->rchild == NULL))
			return 0;

		rc = (filter_logic_result(f->lchild, arg, compare) ||
				filter_logic_result(f->rchild, arg, compare));
	}
	else if (f->type == FILTER_COMPOSITE_AND) {
		if ((f->lchild == NULL) || (f->rchild == NULL))
			return 0;

		rc = (filter_logic_result(f->lchild, arg, compare) &&
				filter_logic_result(f->rchild, arg, compare));
	}
	else {
		return 0;
	}

	if (f->non == 1)
		return !rc;
	else
		return rc;
}

static int oper_result(unsigned int loperate,
		unsigned char op, unsigned int roperate)
{
	switch (op)
	{
		case OP_EQ:
			return (loperate == roperate);
		case OP_NEQ:
			return (loperate != roperate);
		case OP_LT:
			return (loperate < roperate);
		case OP_LE:
			return (loperate <= roperate);
		case OP_GT:
			return (loperate > roperate);
		case OP_GE:
			return (loperate >= roperate);
		default:
			break;
	}
	return 0;
}

int filter_compare_byte1(void *lvalue, unsigned char op, void *rvalue)
{
	return oper_result((unsigned int)(*(unsigned char *)lvalue), op,
			(unsigned int)(*(unsigned char *)rvalue));
}

int filter_compare_byte2(void *lvalue, unsigned char op, void *rvalue)
{
	return oper_result((unsigned int)(*(unsigned short *)lvalue), op,
			(unsigned int)(*(unsigned short *)rvalue));
}

int filter_compare_byte4(void *lvalue, unsigned char op, void *rvalue)
{
	return oper_result(*(unsigned int *)lvalue, op, *(unsigned int *)rvalue);
}

int filter_compare_byte8(void *lvalue, unsigned char op, void *rvalue)
{
	unsigned long long loperate;
	unsigned long long roperate;

	loperate = *(unsigned long long *)lvalue;
	roperate = *(unsigned long long *)rvalue;
	switch (op)
	{
		case OP_EQ:
			return (loperate == roperate);
		case OP_NEQ:
			return (loperate != roperate);
		case OP_LT:
			return (loperate < roperate);
		case OP_LE:
			return (loperate <= roperate);
		case OP_GT:
			return (loperate > roperate);
		case OP_GE:
			return (loperate >= roperate);
		default:
			break;
	}

	return 0;
}

int filter_compare_char_string(void *lvalue, unsigned char op, void *rvalue)
{
	int rc = 0;

	if ((op == OP_STRCMP) &&
			(strcmp((char *)lvalue, (char *)rvalue) == 0))
		rc = 1;

	if ((op == OP_STRNCMP) &&
			(strlen(lvalue) >= strlen(rvalue)) &&
			(strncmp((char *)lvalue, (char *)rvalue, strlen(rvalue)) == 0))
		rc = 1;

	return rc;
}

int filter_compare_octet_string(void *lvalue, void *lvalue_ex,
				unsigned char op, void *rvalue, void *rvalue_ex)
{
	int rc = 0;
	unsigned int lvl = *(unsigned int *)lvalue_ex;
	unsigned int rvl = *(unsigned int *)rvalue_ex;

	if ((op == OP_OCTCMP) &&
			(lvl == rvl) &&
			(memcmp((char *)lvalue, (char *)rvalue, rvl) == 0))
		rc = 1;

	if ((op == OP_OCTNCMP) &&
			(lvl >= rvl) &&
			(memcmp((char *)lvalue, (char *)rvalue, rvl) == 0))
		rc = 1;

	return rc;
}

static unsigned int get_ip_addr(char *ip)
{
	unsigned int a, b, c, d;

	if(4 == sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d)) {
		if ((a > 0) && (a < 255) && (b >= 0) && (b < 255) &&
			(c >= 0) && (c < 255) && (d > 0) && (d < 255))
			return (unsigned int)(((a & 0xff) << 24) |
								  ((b & 0xff) << 16) |
							      ((c & 0xff) << 8)  |
							       (d & 0xff));
	}

	return (unsigned int)-1;
}

static int filter_cfg_load(unsigned long hd, const char *section, void **filter)
{
	const char *sname = NULL;
	char value[128];
	int i = 0;
	int ip_count = 0;
	unsigned int ip = (unsigned int)-1;
	int spc_count = 0;
	unsigned int spc = (unsigned int)-1;
	int sgt_count = 0;
	unsigned long long sgt = (unsigned long long)-1;
	int sio_count = 0;
	int sio = -1;

	*filter = NULL;
	sname = section ? section : "Filter Condition";
	if (!CfgExistSection(hd, sname, 1)) {
		LOGERROR("Filter: Can't find section [%s]", sname);
		return 0;
	}

	if (CfgGetValue(hd, sname, "disabled", value, 1, 1) == -1) {
		LOGERROR("Filter: Loading 'disabled' failed.");
		return -1;
	}
	if (strcmp(value, "no") != 0) {
		LOG("%s: Option [disabled]: %s", sname, value);
		return 0;
	}

	ip_count = CfgGetCount(hd, sname, "ip", 1);
	for (i = 1; i <= ip_count; i++) {
		if (CfgGetValue(hd, sname, "ip", value, i, 1) == -1) {
			LOGERROR("Filter: Loading 'ip' failed.");
			return -1;
		}
		LOG("%s: ip = %s", sname, value);

		ip = get_ip_addr(value);
		if (ip == (unsigned int)-1) {
			LOGERROR("Filter: Loading 'ip %s' failed.", value);
			return -1;
		}

		if (*filter == NULL) {
			*filter = filter_single(FILTER_OPTION_SRC_IP_ADDR, OP_EQ,
					DATATYPE_BYTE4, &ip, NULL);
			*filter = filter_or(*filter,
					filter_single(FILTER_OPTION_DST_IP_ADDR,
						OP_EQ, DATATYPE_BYTE4, &ip, NULL), 1);
		}
		else {
			*filter = filter_or(*filter,
					filter_single(FILTER_OPTION_SRC_IP_ADDR,
						OP_EQ, DATATYPE_BYTE4, &ip, NULL), 1);
			*filter = filter_or(*filter,
					filter_single(FILTER_OPTION_DST_IP_ADDR,
						OP_EQ, DATATYPE_BYTE4, &ip, NULL), 1);
		}
	}
	if (*filter != NULL)
		return 0;

	spc_count = CfgGetCount(hd, sname, "spc", 1);
	for (i = 1; i <= spc_count; i++) {
		char *ptr;
		unsigned long int ul;

		if (CfgGetValue(hd, sname, "spc", value, i, 1) == -1) {
			LOGERROR("Filter: Loading 'spc' failed.");
			return -1;
		}
		LOG("%s: spc = %s", sname, value);

		ul = strtoul(value, &ptr, 0);
		if (*ptr != 0) {
			LOGERROR("Filter: Loading 'spc %s' failed.", value);
			return -1;
		}
		spc = (unsigned int)ul;

		if (*filter == NULL) {
			*filter = filter_single(FILTER_OPTION_OPC, OP_EQ,
					DATATYPE_BYTE4, &spc, NULL);
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_DPC, OP_EQ,
						DATATYPE_BYTE4, &spc, NULL), 1);
		}
		else {
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_OPC,
						OP_EQ, DATATYPE_BYTE4, &spc, NULL), 1);
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_DPC,
						OP_EQ, DATATYPE_BYTE4, &spc, NULL), 1);
		}
	}
	sio_count = CfgGetCount(hd, sname, "sio", 1);
	for (i = 1; i <= sio_count; i++) {
		char *ptr;
		unsigned long int ul;

		if (CfgGetValue(hd, sname, "sio", value, i, 1) == -1) {
			LOGERROR("Filter: Loading 'sio' failed.");
			return -1;
		}
		LOG("%s: sio = %s", sname, value);

		ul = strtoul(value, &ptr, 0);
		if (*ptr != 0) {
			LOGERROR("Filter: Loading 'sio %s' failed.", value);
			return -1;
		}
		sio = (int)ul;

		if (*filter == NULL) {
			*filter = filter_single(FILTER_OPTION_MTP_SIO, OP_EQ,
					DATATYPE_BYTE4, &sio, NULL);
		}
		else {
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_MTP_SIO,
						OP_EQ, DATATYPE_BYTE4, &sio, NULL), 1);
		}
	}
	sgt_count = CfgGetCount(hd, sname, "sgt", 1);
	for (i = 1; i <= sgt_count; i++) {
		char *ptr;

		if (CfgGetValue(hd, sname, "sgt", value, i, 1) == -1) {
			LOGERROR("Filter: Loading 'sgt' failed.");
			return -1;
		}
		LOG("%s: sgt = %s", sname, value);

#ifndef WIN32
		sgt = strtoull(value, &ptr, 0);
#else
		sgt = _strtoui64(value, &ptr, 0);
#endif
		if (*ptr != 0) {
			LOGERROR("Filter: Loading 'sgt %s' failed.", value);
			return -1;
		}

		if (*filter == NULL) {
			*filter = filter_single(FILTER_OPTION_OGT, OP_EQ,
					DATATYPE_BYTE8, &sgt, NULL);
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_DGT, OP_EQ,
						DATATYPE_BYTE8, &sgt, NULL), 1);
		}
		else {
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_OGT,
						OP_EQ, DATATYPE_BYTE8, &sgt, NULL), 1);
			*filter = filter_or(*filter, filter_single(FILTER_OPTION_DGT,
						OP_EQ, DATATYPE_BYTE8, &sgt, NULL), 1);
		}
	}

	return 0;
}

int filter_cfghd(unsigned long cfghd, char *section, void **filter)
{
	if (cfghd == 0ul)
		return -1;

	if (filter_cfg_load(cfghd, section, filter) < 0) {
		if (*filter)
			filter_release(*filter);
		*filter = NULL;
		return -1;
	}

	return 0;
}

int filter_cfgfile(char *cfgfile, char *section, void **filter)
{
	unsigned long cfghd;

	cfghd = CfgInitialize(cfgfile);
	if (cfghd == 0ul)
		return -1;

	if (filter_cfg_load(cfghd, section, filter) < 0) {
		if (*filter)
			filter_release(*filter);
		*filter = NULL;
		CfgInvalidate(cfghd);

		return -1;
	}

	CfgInvalidate(cfghd);

	return 0;
}

int filter_cfgstr(char *cfgstr, char *section, void **filter)
{
	unsigned long cfghd;

	cfghd = CfgInitializeString(cfgstr);
	if (cfghd == 0ul)
		return -1;

	if (filter_cfg_load(cfghd, section, filter) < 0) {
		if (*filter)
			filter_release(*filter);
		*filter = NULL;
		CfgInvalidate(cfghd);

		return -1;
	}

	CfgInvalidate(cfghd);

	return 0;
}

