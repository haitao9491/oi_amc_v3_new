/*
 *
 * utimer.h - A brief description goes here.
 *
 */

#ifndef _HEAD_UTIMER_4F8FCB4C_57912F23_2C5DE234_H
#define _HEAD_UTIMER_4F8FCB4C_57912F23_2C5DE234_H

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

#define UTIMER_INTERNAL_TIMESOURCE 0x01

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *utimer_open(int type,
		void (*callback)(unsigned int, unsigned int, void *));
extern DLL_APP void *utimer_add(void *hd,
		unsigned int expire_s, unsigned int expire_ns, void *data);
extern DLL_APP void *utimer_add_by_offset(void *hd,
		unsigned int expire_s, unsigned int expire_ns, void *data);
extern DLL_APP void utimer_delete(void *timer);
extern DLL_APP void utimer_get_counter(void *hd, unsigned int *count,
		unsigned long long *add, unsigned long long *compare);
extern DLL_APP void utimer_run(void *hd, unsigned int s, unsigned int ns);
extern DLL_APP void  utimer_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_UTIMER_4F8FCB4C_57912F23_2C5DE234_H */
