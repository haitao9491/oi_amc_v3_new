/*
 *
 * filter.h
 *
 */

#ifndef _HEAD_FILTER_1B49230D_52E00F49_5AF0C50F_H
#define _HEAD_FILTER_1B49230D_52E00F49_5AF0C50F_H

#ifndef DLL_APP
#ifdef WIN32
#ifdef _USRDLL
#define DLL_APP _declspec(dllexport)
#else
#define DLL_APP _declspec(dllimport)
#endif
#else
#define DLL_APP
#endif
#endif

#ifdef WIN32
#pragma warning(disable : 4996)
#endif

enum filter_option {
	FILTER_OPTION_IMEI = 1,
	FILTER_OPTION_IMSI,
	FILTER_OPTION_TMSI,
	FILTER_OPTION_DLCI,
	FILTER_OPTION_BVCI,
	FILTER_OPTION_SRC_IP_ADDR,
	FILTER_OPTION_DST_IP_ADDR,
	FILTER_OPTION_CDR_TYPE,
	FILTER_OPTION_CDR_RESULT,
	FILTER_OPTION_OPC,
	FILTER_OPTION_DPC,
	FILTER_OPTION_OPP_NO,
	FILTER_OPTION_SRC_LAC,
	FILTER_OPTION_SRC_CI,
	FILTER_OPTION_DST_LAC,
	FILTER_OPTION_DST_CI,
	FILTER_OPTION_TS,
	FILTER_OPTION_CDRID,
	FILTER_OPTION_DEVICE,
	FILTER_OPTION_CHANNEL,
	FILTER_OPTION_OGT,
	FILTER_OPTION_DGT,
	FILTER_OPTION_OP_CODE,
	FILTER_OPTION_SVRID,
	FILTER_OPTION_SMS_CENTER,
	FILTER_OPTION_MTP_SIO,
};

enum {
	DATATYPE_BYTE1 = 1,
	DATATYPE_BYTE2,
	DATATYPE_BYTE4,
	DATATYPE_BYTE8,
	DATATYPE_CHAR_STRING,
	DATATYPE_OCTET_STRING,
};

enum {
	OP_EQ = 1,
	OP_NEQ,
	OP_LT,
	OP_LE,
	OP_GT,
	OP_GE,
	OP_STRCMP,    /* Precise comparision: compare both length and content. */
	OP_STRNCMP,
	OP_OCTCMP,
	OP_OCTNCMP,
};

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *filter_single(enum filter_option type, unsigned char op,
				unsigned char datatype, void *value, void *value_len);
DLL_APP void *filter_or(void *fc1, void *fc2, int release);
DLL_APP void *filter_and(void *fc1, void *fc2, int release);
DLL_APP void *filter_non(void *filter);

DLL_APP unsigned char *filter_encode(void *filter, int reserved, int *len);
DLL_APP int filter_encode_ext(void *filter, unsigned char *buf, int size);
DLL_APP void *filter_decode(unsigned char *buf, int len);
DLL_APP void  filter_release(void *fc);

DLL_APP int filter_traversing(void *filter, void *arg,
				void (*callback)(enum filter_option type, void *arg));

DLL_APP int filter_logic_result(void *filter, void *arg,
		int (*compare)(void *arg, enum filter_option type,
			unsigned char op, void *value, void *value_ex));

DLL_APP int filter_compare_byte1(void *lvalue, unsigned char op, void *rvalue);
DLL_APP int filter_compare_byte2(void *lvalue, unsigned char op, void *rvalue);
DLL_APP int filter_compare_byte4(void *lvalue, unsigned char op, void *rvalue);
DLL_APP int filter_compare_byte8(void *lvalue, unsigned char op, void *rvalue);
DLL_APP int filter_compare_char_string(void *lvalue,
		unsigned char op, void *rvalue);
DLL_APP int filter_compare_octet_string(void *lvalue, void *lvalue_ex,
				unsigned char op, void *rvalue, void *rvalue_ex);

DLL_APP char *filter_get_datatype(unsigned char datatype);
DLL_APP char *filter_get_op(unsigned char op);
DLL_APP char *filter_get_type(enum filter_option type);
DLL_APP char *filter_get_expression(void *filter, char *buf, int size);

DLL_APP int filter_cfghd(unsigned long cfghd, char *section, void **filter);
DLL_APP int filter_cfgfile(char *cfgfile, char *section, void **filter);
DLL_APP int filter_cfgstr(char *cfgstr, char *section, void **filter);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FILTER_1B49230D_52E00F49_5AF0C50F_H */
