/*
 *
 * misc.h - A brief description goes here.
 *
 */

#ifndef _HEAD_MISC_6EB38C1A_398FB171_008F79FE_H
#define _HEAD_MISC_6EB38C1A_398FB171_008F79FE_H

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

#define lchmin(a, b) (((a) <= (b)) ? (a) : (b))
#define lchmax(a, b) (((a) >= (b)) ? (a) : (b))

#if defined(__cplusplus)
extern "C" {
#endif

static __inline int sys_is_le(void)
{
	int i = 1;

	return *((unsigned char *)&i) == 1;
}

#ifdef WIN32
#define strtoll _strtoi64
#define strtoull _strtoui64
#endif

#ifdef WIN32

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
struct timezone
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
DLL_APP int gettimeofday(struct timeval *tv, struct timezone *tz);
DLL_APP int killprocessbypid(unsigned long pid);

#endif	/* --end of #ifdef WIN32 */

DLL_APP void init_timestamp();
DLL_APP void set_timestamp();
DLL_APP void get_timestamp(unsigned int *s, unsigned int *ns);

extern long long time_diff(unsigned int as, unsigned int ans, unsigned int bs, unsigned int bns);
extern DLL_APP int time_diff_us(unsigned int as, unsigned int ans, unsigned int bs, unsigned int bns);
extern DLL_APP int time_diff_ms(unsigned int as, unsigned int ans, unsigned int bs, unsigned int bns);

extern unsigned int get_sys_time_s(unsigned int *s, unsigned int *us);

extern int fs_free_kbytes(const char *path);
extern int fs_total_kbytes(const char *path);

extern unsigned int combine_bits_range(unsigned char *data,
		int sbyte, int sbit, int ebyte, int ebit);
extern unsigned int combine_bits_length(unsigned char *data,
		int sbyte, int sbit, int bits);
extern int count_bits_octet(unsigned char byte);
extern int count_bits_length(unsigned char *data, int sbyte, int sbit, int bits);
extern int query_bit(unsigned char *data, int sbyte, int sbit, int bits);
extern DLL_APP unsigned char swab_byte(unsigned char data);

extern DLL_APP unsigned int CombineBitsLE(unsigned char *data, int sbit, int bits);
extern DLL_APP unsigned int CombineBitsBE(unsigned char *data, int sbit, int bits);

extern DLL_APP char *ip4addr_str(unsigned int ipaddr, char *str);
extern char *ip4addr_str1(unsigned int ipaddr);
extern char *ip4addr_str2(unsigned int ipaddr);

extern DLL_APP unsigned int get_host_addr(void);
extern DLL_APP unsigned int get_addr(char *hostname);

extern DLL_APP void decode_bit7(unsigned char *p, int len, unsigned char *q);
extern DLL_APP void decode_ucs2(unsigned char *p, int len, unsigned char *q);

extern DLL_APP void md5sum(unsigned char *p, int len, unsigned char *q);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_MISC_6EB38C1A_398FB171_008F79FE_H */
