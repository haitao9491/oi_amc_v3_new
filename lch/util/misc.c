/*
 *
 * misc.c - A brief description goes here.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#ifndef WIN32
#include <arpa/inet.h>
#include <sys/vfs.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <netdb.h>
#else
#include <time.h>
#include "Winsock2.h"
#include "ws2tcpip.h"
#endif
#include "os.h"
#include "aplog.h"
#include "misc.h"

#include "md5c.i"

#ifdef __tilegx__
#include <stdint.h>
#include <tmc/perf.h>
#include <arch/cycle.h>

static double ns_per_cycle;
static uint64_t ons;
#endif

#ifdef WIN32

int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	FILETIME ft;
	unsigned __int64 tmpres = 0;
	static int tzflag;

	if (NULL != tv)
	{
		GetSystemTimeAsFileTime(&ft);

		tmpres |= ft.dwHighDateTime;
		tmpres <<= 32;
		tmpres |= ft.dwLowDateTime;

		/*converting file time to unix epoch*/
		tmpres /= 10;  /*convert into microseconds*/
		tmpres -= DELTA_EPOCH_IN_MICROSECS;
		tv->tv_sec = (long)(tmpres / 1000000UL);
		tv->tv_usec = (long)(tmpres % 1000000UL);
	}

	if (NULL != tz)
	{
		if (!tzflag)
		{
			_tzset();
			tzflag++;
		}
		tz->tz_minuteswest = _timezone / 60;
		tz->tz_dsttime = _daylight;
	}

	return 0;
}

int killprocessbypid(unsigned long pid)
{
	HANDLE hProcess;
	BOOL bRet;

	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess)
	{
		bRet = TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
		if(bRet)
			return 1;
	}
	return -1;
}
#endif

void init_timestamp()
{
#ifdef __tilegx__
	ns_per_cycle = 1000000000.0 / (double)tmc_perf_get_cpu_speed();
	ons = 0ull;
#endif
}

void set_timestamp()
{
#ifdef __tilegx__
	struct timeval tv;
	uint64_t now, cns, tns;

	if (gettimeofday(&tv, NULL) == 0) {
		now = get_cycle_count();
		cns = now * ns_per_cycle;
		tns = tv.tv_sec * 1000000000 + tv.tv_usec * 1000;

		if (tns >= cns) {
			ons = tns - cns;
		}
	}
#endif
}

void get_timestamp(unsigned int *s, unsigned int *ns)
{
#ifdef __tilegx__
	uint64_t now, tns;

	now = get_cycle_count();
	tns = ons + now * ns_per_cycle;

	if (s)
		*s = tns / 1000000000;
	if (ns)
		*ns = tns % 1000000000;
#else
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == 0) {
		if (s)
			*s = tv.tv_sec;
		if (ns)
			*ns = tv.tv_usec * 1000;
	}
#endif
}

long long time_diff(unsigned int as, unsigned int ans,
		unsigned int bs, unsigned int bns)
{
	long long a, b;

	a = ((long long)as * 1000000000ll) + (long long)ans;
	b = ((long long)bs * 1000000000ll) + (long long)bns;

	return (b - a);
}

int time_diff_us(unsigned int as, unsigned int ans,
		unsigned int bs, unsigned int bns)
{
	return (int)(time_diff(as, ans, bs, bns) / 1000);
}

int time_diff_ms(unsigned int as, unsigned int ans,
		unsigned int bs, unsigned int bns)
{
	return (int)(time_diff(as, ans, bs, bns) / 1000000);
}

unsigned int get_sys_time_s(unsigned int *s, unsigned int *us)
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) == 0) {
		if (s)
			*s = tv.tv_sec;
		if (us)
			*us = tv.tv_usec;
		return tv.tv_sec;
	}

	return 0;
}

#if !defined(OS_WINDOWS)

int fs_free_kbytes(const char *path)
{
#if defined(OS_LINUX)
	struct statfs   f;
#elif defined(OS_SUN) || defined(OS_HP)
	struct statvfs  f;
#endif

	if(!path || !*path)
		return(-1);
#if defined(OS_LINUX)
	if(statfs(path, &f) == -1)
		return(-1);
	return(f.f_bsize / 1024 * f.f_bavail);
#elif defined(OS_SUN) || defined(OS_HP)
	if(statvfs(path, &f) == -1)
		return(-1);
	return(f.f_frsize / 1024 * f.f_bavail);
#else
	return(-1);
#endif
}

int fs_total_kbytes(const char *path)
{
#if defined(OS_LINUX)
	struct statfs   f;
#elif defined(OS_SUN) || defined(OS_HP)
	struct statvfs  f;
#endif

	if(!path || !*path)
		return(-1);
#if defined(OS_LINUX)
	if(statfs(path, &f) == -1)
		return(-1);
	return(f.f_bsize / 1024 * f.f_blocks);
#elif defined(OS_SUN) || defined(OS_HP)
	if(statvfs(path, &f) == -1)
		return(-1);
	return(f.f_frsize / 1024 * f.f_blocks);
#else
	return(-1);
#endif
}

#else

int fs_free_kbytes(const char *path)
{
	ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;

	if (GetDiskFreeSpaceEx((LPCWSTR)path,
			&i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes))
		return((int)(i64FreeBytesToCaller.QuadPart / 1024));

	return(-1);
}

int fs_total_kbytes(const char *path)
{
	ULARGE_INTEGER i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;

	if (GetDiskFreeSpaceEx((LPCWSTR)path,
			&i64FreeBytesToCaller, &i64TotalBytes, &i64FreeBytes))
		return((int)(i64TotalBytes.QuadPart / 1024));

	return(-1);
}

#endif

/* bit: 8 7 6 5 4 3 2 1 */
unsigned int combine_bits_range(unsigned char *data,
		int sbyte, int sbit, int ebyte, int ebit)
{
	unsigned int  vi = 0;
	unsigned char vu = 0;

	data += sbyte;

	while (sbyte < ebyte) {
		vu = *data & (0xff >> (8 - sbit));
		vi = (vi << 8) | vu;

		sbyte++;
		data++;
		sbit = 8;
	}

	vu = *data & ((0xff >> (8 - sbit)) & (0xff << (ebit - 1)));
	vi = (vi << 8) | vu;
	vi = vi >> (ebit - 1);

	return vi;
}

/* bit: 8 7 6 5 4 3 2 1 */
unsigned int combine_bits_length(unsigned char *data,
		int sbyte, int sbit, int bits)
{
	unsigned int  vi = 0;
	unsigned char vu = 0;

	data += sbyte;

	while (sbit < bits) {
		vu = *data & (0xff >> (8 - sbit));
		vi = (vi << 8) | vu;

		data++;
		bits -= sbit;
		sbit = 8;
	}

	vu = *data & ((0xff >> (8 - sbit)) & (0xff << (sbit - bits)));
	vi = (vi << 8) | vu;
	vi = vi >> (sbit - bits);

	return vi;
}

/*
 * sbit: start from zero
 * bits: Number of bits to be combined together into an unsigned integer.
 */
unsigned int CombineBitsLE(unsigned char *data, int sbit, int bits)
{
	int sbyte, ebyte, ebit;
	int lbits;
	unsigned int  vi = 0;
	static unsigned char rmask[9] = {
		0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff
	};
	static unsigned char lmask[9] = {
		0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff
	};

	if (bits < 1)
		return 0;

	/* In this implementation the bit number is defined as follow:
	 *     MSB 8 7 6 5 4 3 2 1 LSB
	 */
	sbyte = (sbit >> 3);
	ebit = sbit + bits - 1;
	sbit = (sbit & 0x07) + 1;
	ebyte = (ebit >> 3);
	ebit = (ebit & 0x07) + 1;

	data += ebyte;

	while (ebyte > sbyte)
	{
		vi = (vi << 8) | (*data & rmask[ebit]);

		ebyte--;
		ebit = 8;
		data--;
	}

	lbits = ebit - sbit + 1;
	vi = (vi << lbits) | (((*data << (8 - ebit)) & lmask[lbits]) >> (8 - lbits));

	return vi;
}

/*
 * sbit: start from zero
 * bits: Number of bits to be combined together into an unsigned integer.
 */
unsigned int CombineBitsBE(unsigned char *data, int sbit, int bits)
{
	int sbyte;
	unsigned int  vi = 0;

	/* In this implementation the bit number is defined as follow:
	 *     MSB 8 7 6 5 4 3 2 1 LSB
	 */
	sbyte = (sbit >> 3);
	sbit = (sbit & 0x07) + 1;

	data += sbyte;

	if (sbit == 8) {
		while (bits >= 8) {
			vi = (vi << 8) | *data;
			data++;
			bits -= 8;
		}
		if (bits > 0) {
			vi = (vi << bits) | (*data >> (8 - bits));
		}
	}
	else {
		int lbits, rbits;

		while (sbit < bits) {
			vi = (vi << 8) | (*data & (0xff >> (8 - sbit)));
			data++;
			bits -= sbit;
			sbit = 8;
		}

		lbits = 8 - sbit;
		rbits = sbit - bits;
		vi = (vi << bits) | ((*data & ((0xff >> lbits) & (0xff << rbits))) >> rbits);
	}

	return vi;
}

int count_bits_octet(unsigned char byte)
{
	int count = 0;

	while (byte) {
		if (byte & 0x01)
			count++;
		byte >>= 1;
	}

	return count;
}

/* bit: 8 7 6 5 4 3 2 1 */
int count_bits_length(unsigned char *data, int sbyte, int sbit, int bits)
{
	int count = 0;
	unsigned char vu = 0;

	data += sbyte;

	while (sbit < bits) {
		vu = *data & (0xff >> (8 - sbit));
		count += count_bits_octet(vu);

		data++;
		bits -= sbit;
		sbit = 8;
	}

	vu = *data & ((0xff >> (8 - sbit)) & (0xff << (sbit - bits)));
	count += count_bits_octet(vu);

	return count;
}

/* bit: 8 7 6 5 4 3 2 1, bits starts from 1 */
int query_bit(unsigned char *data, int sbyte, int sbit, int bits)
{
	bits += (8 - sbit);
	while (bits > 8) {
		sbyte++;
		bits -= 8;
	}

	return (((*(data + sbyte)) >> (8 - bits)) & 0x01) ? 1 : 0;
}

unsigned char swab_byte(unsigned char data)
{
	unsigned char res = 0;
	int i;

	for (i = 0; data && (i < 8); i++) {
		if (data & 0x01) {
			res |= (1 << (7 - i));
		}
		data >>= 1;
	}

	return res;
}

char *ip4addr_str(unsigned int ipaddr, char *str)
{
	static char   ip4addr_buf[16];
	unsigned int  ip;
	char         *p;

	p = str ? str : ip4addr_buf;
	ip = ntohl(ipaddr);
	sprintf(p, "%d.%d.%d.%d",
			(ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff);

	return p;
}

char *ip4addr_str1(unsigned int ipaddr)
{
	static char ip4addr_buf1[16];

	return ip4addr_str(ipaddr, ip4addr_buf1);
}

char *ip4addr_str2(unsigned int ipaddr)
{
	static char ip4addr_buf2[16];

	return ip4addr_str(ipaddr, ip4addr_buf2);
}

unsigned int get_host_addr(void)
{
#ifndef WIN32
	unsigned int addr = htonl(INADDR_ANY);
	struct utsname uts;

	if (uname(&uts) == 0) {
		struct addrinfo *result, *rp;

		LOGDEBUG("get_host_addr: nodename %s", uts.nodename);

		if (getaddrinfo(uts.nodename, NULL, NULL, &result) == 0) {
			for (rp = result; rp != NULL; rp = rp->ai_next) {
				if (rp->ai_family == AF_INET) {
					struct sockaddr_in *sa = (struct sockaddr_in *)rp->ai_addr;

					addr = sa->sin_addr.s_addr;
					LOGDEBUG("get_host_addr: addr %s", ip4addr_str(addr, NULL));
					break;
				}
			}

			freeaddrinfo(result);
		}
	}

	return addr;
#else

	char hostname[256];
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	u_long retAddr;

	struct addrinfo aiHints;
	struct addrinfo *aiList = NULL;
	struct sockaddr_in* pSockaddr = NULL;
	int retVal;

	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		return (unsigned int)-1;
	}
 
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) {
		//WSACleanup();
		return (unsigned int)-1;
	}

	if (gethostname(hostname, sizeof(hostname)) != 0)
	{
		//WSACleanup();
		return (unsigned int)-1;
	}

	memset(&aiHints, 0, sizeof(aiHints));
	aiHints.ai_family = AF_INET;
	aiHints.ai_socktype = SOCK_STREAM;
	aiHints.ai_protocol = IPPROTO_TCP;
	if ((retVal = getaddrinfo(hostname, NULL, &aiHints, &aiList)) != 0) 
	{
		//WSACleanup();
		return (unsigned int)-1;
	}

	pSockaddr = (struct sockaddr_in*)aiList->ai_addr;
	retAddr = pSockaddr->sin_addr.S_un.S_addr;

	freeaddrinfo(aiList);
	//WSACleanup();

	return retAddr;


#endif
}

unsigned int get_addr(char *hostname)
{
	unsigned int addr = htonl(INADDR_ANY);
	struct addrinfo *result, *rp;

	if (hostname) {
		if (getaddrinfo(hostname, NULL, NULL, &result) == 0) {
			for (rp = result; rp != NULL; rp = rp->ai_next) {
				if (rp->ai_family == AF_INET) {
					struct sockaddr_in *sa = (struct sockaddr_in *)rp->ai_addr;

					addr = sa->sin_addr.s_addr;
					break;
				}
			}

			freeaddrinfo(result);
		}
	}

	return addr;
}

void decode_bit7(unsigned char *p, int len, unsigned char *q)
{
	int i, j;
	unsigned char temp = 0;

	for (i = j = 0; i < len; i++)
	{
		q[j++] = ((p[i] & ((1 << (8 - (i % 7) - 1)) - 1)) << i % 7) + temp;
		temp = p[i] >> (8 - (i % 7) - 1);
		if((i % 7) == 6)
		{
			q[j++] = temp;
			temp = 0;
		}
	}

	q[j] = 0;
}

void decode_ucs2(unsigned char *p, int len, unsigned char *q)
{
	wchar_t wchar[160];
	int i;

#ifndef WIN32
	setlocale(LC_ALL, "zh_CN.gbk");
#else
	setlocale(LC_ALL, ".936");
#endif

	for (i = 0; i < len / 2; i++)
	{
		wchar[i] = *p++ << 8;
		wchar[i] |= *p++;
	}

	wcstombs((char *)q, wchar, len);
}

void md5sum(unsigned char *p, int len, unsigned char *q)
{
	MD5_CTX ctx;

	MD5Init(&ctx);
	MD5Update(&ctx, p, len);
	MD5Final(q, &ctx);
}

