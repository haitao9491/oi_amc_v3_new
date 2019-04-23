/*
 *
 * bcdnumber.h - Support a string with up to 19 BCD numbers.
 *
 */

#ifndef _HEAD_BCDNUMBER_106FCC7C_44B1D5DF_5363935D_H
#define _HEAD_BCDNUMBER_106FCC7C_44B1D5DF_5363935D_H

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

typedef unsigned long long int bcdnumber;

#define BCDNUMBER_WILDCARD ((bcdnumber)0llu)
#define BCDNUMBER_VALID(bcd) ((bcd) != BCDNUMBER_WILDCARD)

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP bcdnumber  str2bcdnumber(char *str);
extern DLL_APP bcdnumber  str2bcdnumber_ex(char *str, int base);
extern char      *bcdnumber2str(bcdnumber bcdnum, char *str);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_BCDNUMBER_106FCC7C_44B1D5DF_5363935D_H */
