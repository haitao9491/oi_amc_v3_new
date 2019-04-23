/*
 *
 * fldvalue.h - An union structure to hold all elementary data structures.
 *
 */

#ifndef _HEAD_FLDVALUE_24BDA279_43356283_46BC3678_H
#define _HEAD_FLDVALUE_24BDA279_43356283_46BC3678_H

#define FLD_TYPE_CHAR      1
#define FLD_TYPE_UCHAR     2
#define FLD_TYPE_SHORT     3
#define FLD_TYPE_USHORT    4
#define FLD_TYPE_INT       5
#define FLD_TYPE_UINT      6
#define FLD_TYPE_LONG      7
#define FLD_TYPE_ULONG     8
#define FLD_TYPE_LONGLONG  9
#define FLD_TYPE_ULONGLONG 10
#define FLD_TYPE_STR       11
#define FLD_TYPE_USTR      12

struct fld_value {
	int type;
	int length;
	union {
		char c;
		unsigned char uc;
		short s;
		unsigned short us;
		int i;
		unsigned int ui;
		long l;
		unsigned long ul;
		long long int ll;
		unsigned long long int ull;
		char *str;
		unsigned char *ustr;
	} data;

	int dontfree;
};

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FLDVALUE_24BDA279_43356283_46BC3678_H */
