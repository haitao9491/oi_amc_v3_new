/*
 *
 * hash.h - A brief description goes here.
 *
 */

#ifndef _HEAD_HASH_732968DD_45737B9F_16867DAF_H
#define _HEAD_HASH_732968DD_45737B9F_16867DAF_H
#ifndef WIN32
#include <bits/wordsize.h>
#else
#pragma warning(disable : 4311)
#ifndef __WORDSIZE
//#define __WORDSIZE (sizeof(long) * 8)
#define __WORDSIZE 32
#endif

#endif
#if !defined(__WORDSIZE)
#error __WORDSIZE not defined!
#endif

#if __WORDSIZE == 32
/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME 0x9e370001UL
#elif __WORDSIZE == 64
/*  2^63 + 2^61 - 2^57 + 2^54 - 2^51 - 2^18 + 1 */
#define GOLDEN_RATIO_PRIME 0x9e37fffffffc0001UL
#else
#error Define GOLDEN_RATIO_PRIME for your __WORDSIZE.
#endif

static __inline unsigned long hash_long(unsigned long val, unsigned int bits)
{
	unsigned long hash = val;

#if __WORDSIZE == 64
	/*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
	unsigned long n = hash;
	n <<= 18; hash -= n;
	n <<= 33; hash -= n;
	n <<= 3;  hash += n;
	n <<= 3;  hash -= n;
	n <<= 4;  hash += n;
	n <<= 2;  hash += n;
#else
	/* On some cpus multiply is faster, on others gcc will do shifts */
	hash *= GOLDEN_RATIO_PRIME;
#endif

	/* High bits are more random, so use them. */
	return hash >> (__WORDSIZE - bits);
}

static __inline unsigned int SDBMHash(char* str, unsigned int len)
{
	unsigned int hash = 0;
	unsigned int i    = 0;

	for(i = 0; i < len; str++, i++)
	{
		hash = (*str) + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

static __inline unsigned long SDBMHashUL(unsigned long val, unsigned int bits)
{
	unsigned int v = (unsigned int)val;
	unsigned int hash;

	hash = SDBMHash((char *)&v, sizeof(v));

	return (hash & ((1 << bits) - 1));
}

static __inline unsigned long hash_ptr(void *ptr, unsigned int bits)
{
	return hash_long((unsigned long)ptr, bits);
}

#endif /* #ifndef _HEAD_HASH_732968DD_45737B9F_16867DAF_H */
