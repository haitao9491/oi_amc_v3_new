/*
 * (C) Copyright 2007
 * Hu Chunlin <chunlin.hu@gmail.com>
 *
 * mutexrw.h - A brief description goes here.
 *
 */

#ifndef _HEAD_MUTEXRW_17175554_7711CC80_1956C87D_H
#define _HEAD_MUTEXRW_17175554_7711CC80_1956C87D_H

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

extern DLL_APP void *mutexrw_open(void *t);
extern DLL_APP void  mutexrw_close(void *hd);

/* Return Value:
 * If the specified action, lock or unlock, succeeded, the return
 * value is zero. If an error occurred, the return value is non-zero.
 */
extern DLL_APP int   mutexrw_rlock(void *hd);
extern DLL_APP int   mutexrw_wlock(void *hd);
extern DLL_APP int   mutexrw_unlock(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_MUTEXRW_17175554_7711CC80_1956C87D_H */
