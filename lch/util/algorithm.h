/*
 *
 * algorithm.h - A brief description goes here.
 *
 */

#ifndef _HEAD_ALGORITHM_413777B5_36E46472_46D9F803_H
#define _HEAD_ALGORITHM_413777B5_36E46472_46D9F803_H

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

#if defined(__cplusplus)
extern "C" {
#endif

static __inline int before32(unsigned int s1, unsigned int s2)
{
	return (int)(s1 - s2) < 0;
}
#define after32(s2, s1)  before32(s1, s2)

static __inline int equal32(unsigned int s1, unsigned int s2)
{
	return s1 == s2;
}

/* Is s2 <= s1 <= s3 ? */
static __inline int between32(unsigned int s1, unsigned int s2, unsigned int s3)
{
	return (s3 - s2) >= (s1 - s2);
}

static __inline int before16(unsigned short s1, unsigned short s2)
{
	return (short)(s1 - s2) < 0;
}
#define after16(s2, s1)  before16(s1, s2)

static __inline int equal16(unsigned short s1, unsigned short s2)
{
	return s1 == s2;
}

/* Is s2 <= s1 <= s3 ? */
static __inline int between16(unsigned short s1, unsigned short s2, unsigned short s3)
{
	return (s3 - s2) >= (s1 - s2);
}

static __inline int before8(unsigned char s1, unsigned char s2)
{
	return (signed char)(s1 - s2) < 0;
}
#define after8(s2, s1)  before8(s1, s2)

static __inline int equal8(unsigned char s1, unsigned char s2)
{
	return s1 == s2;
}

/* Is s2 <= s1 <= s3 ? */
static __inline int between8(unsigned char s1, unsigned char s2, unsigned char s3)
{
	return (s3 - s2) >= (s1 - s2);
}

extern unsigned short fcs16(unsigned char *data, int len);
extern unsigned int fcs32(unsigned char *data, int len);
DLL_APP extern unsigned int crc32c(unsigned char *data, int len);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ALGORITHM_413777B5_36E46472_46D9F803_H */
