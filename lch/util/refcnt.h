/*
 *
 * refcnt.h - A brief description goes here.
 *
 */

#ifndef _HEAD_REFCNT_5D652D74_5EC26E2C_148F9465_H
#define _HEAD_REFCNT_5D652D74_5EC26E2C_148F9465_H

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

DLL_APP void *refcnt_open(int refcnt);
DLL_APP int refcnt_add(void *rhd, int refcnt);
DLL_APP int refcnt_sub(void *rhd, int refcnt);
DLL_APP int refcnt_inc(void *rhd);
DLL_APP int refcnt_dec(void *rhd);
DLL_APP void refcnt_close(void *rhd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_REFCNT_5D652D74_5EC26E2C_148F9465_H */
