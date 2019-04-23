/*
 *
 * mutex.h - A brief description goes here.
 *
 */

#ifndef _HEAD_MUTEX_17175554_7711CC80_1956C87D_H
#define _HEAD_MUTEX_17175554_7711CC80_1956C87D_H

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

extern DLL_APP void *mutex_open(void *t);
extern DLL_APP void  mutex_close(void *hd);
extern DLL_APP int   mutex_lock(void *hd);
extern DLL_APP int   mutex_unlock(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_MUTEX_17175554_7711CC80_1956C87D_H */
