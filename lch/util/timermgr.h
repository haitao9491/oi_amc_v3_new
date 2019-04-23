/*
 * (C) Copyright 2010
 * Beijing HLYT Technology Co., Ltd.
 *
 * timermgr.h - A brief description to describe this file.
 *
 */

#ifndef _HEAD_TIMERMGR_34B76B10_2E045C45_37721BD0_H
#define _HEAD_TIMERMGR_34B76B10_2E045C45_37721BD0_H

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

#define TIMERMGR_SOURCE_LOCAL    0
#define TIMERMGR_SOURCE_EXTERNAL 1
#define TIMERMGR_SOURCE_MAXIMUM  2

typedef void (*timer_callback)(unsigned int s, unsigned int ns,
		void *data, void *arg);

#if defined(__cplusplus)
extern "C" void *__timer_mgr_handle;
#else
extern void *__timer_mgr_handle;
#endif

#define TIMERLIST_CREATE(source, callback, arg, name) \
	timer_mgr_create_timerlist(__timer_mgr_handle, source, callback, arg, name)
#define TIMERLIST_RELEASE(timerlist) \
	timer_mgr_release_timerlist(__timer_mgr_handle, timerlist)
#define TIMERLIST_RUN(timerlist, s, ns) \
	timer_mgr_run_timerlist(__timer_mgr_handle, timerlist, s, ns)

#define TIMERLIST_LOG(timerlist) \
	timer_mgr_timerlist_log(timerlist)
#define TIMERLIST_ADD_TIMER_PERIODIC(timerlist, interval, data) \
	timer_mgr_add_timer_periodic(timerlist, interval, data)
#define TIMERLIST_ADD_TIMER(timerlist, expire_s, expire_ns, data) \
	timer_mgr_add_timer(timerlist, expire_s, expire_ns, data)
#define TIMERLIST_ADD_TIMER_BY_OFFSET(timerlist, expire_offset_s, expire_offset_ns, data) \
	timer_mgr_add_timer_by_offset(timerlist, expire_offset_s, expire_offset_ns, data)
#define TIMERLIST_ADD_TIMER_PERIODIC_NOLOCK(timerlist, interval, data) \
	timer_mgr_add_timer_periodic_nolock(timerlist, interval, data)
#define TIMERLIST_ADD_TIMER_NOLOCK(timerlist, expire_s, expire_ns, data) \
	timer_mgr_add_timer_nolock(timerlist, expire_s, expire_ns, data)
#define TIMERLIST_ADD_TIMER_BY_OFFSET_NOLOCK(timerlist, expire_offset_s, expire_offset_ns, data) \
	timer_mgr_add_timer_by_offset_nolock(timerlist, expire_offset_s, expire_offset_ns, data)
#define TIMERLIST_DEL_TIMER(timerlist, timer) \
	timer_mgr_delete_timer(timerlist, timer)

#define TIMER_MGR_ADD_PKTQ_TIMER(interval, data) \
	timer_mgr_add_pktq_timer(__timer_mgr_handle, interval, data)
#define TIMER_MGR_DEL_PKTQ_TIMER(timer) \
	timer_mgr_del_pktq_timer(__timer_mgr_handle, timer)

#define TIMER_MGR_ADD_HASHTBL_TIMER(interval, data) \
	timer_mgr_add_hashtbl_timer(__timer_mgr_handle, interval, data)
#define TIMER_MGR_DEL_HASHTBL_TIMER(timer) \
	timer_mgr_del_hashtbl_timer(__timer_mgr_handle, timer)

#if defined(__cplusplus)
extern "C" {
#endif

extern DLL_APP void *timer_mgr_create_timerlist(void *hd, int timesource,
		timer_callback callback, void *arg, const char *name);
extern DLL_APP void timer_mgr_release_timerlist(void *hd, void *timerlist);
extern DLL_APP void timer_mgr_timerlist_log(void *timerlist);
extern DLL_APP void *timer_mgr_add_timer_periodic(void *timerlist,
		unsigned int interval, void *data);
extern DLL_APP void *timer_mgr_add_timer_periodic_nolock(void *timerlist,
		unsigned int interval, void *data);
extern DLL_APP void *timer_mgr_add_timer(void *timerlist,
		unsigned int expire_s, unsigned int expire_ns, void *data);
extern DLL_APP void *timer_mgr_add_timer_nolock(void *timerlist,
		unsigned int expire_s, unsigned int expire_ns, void *data);
extern DLL_APP void *timer_mgr_add_timer_by_offset(void *timerlist,
		unsigned int expire_offset_s, unsigned int expire_offset_ns,
		void *data);
extern DLL_APP void *timer_mgr_add_timer_by_offset_nolock(void *timerlist,
		unsigned int expire_offset_s, unsigned int expire_offset_ns,
		void *data);
extern DLL_APP void timer_mgr_delete_timer_nolock(void *timerlist, void *timer);
extern DLL_APP void timer_mgr_delete_timer(void *timerlist, void *timer);
extern DLL_APP void *timer_mgr_update_timer(void *timerlist, void *timer,
		unsigned int expire_s, unsigned int expire_ns);
extern DLL_APP void *timer_mgr_update_timer_nolock(void *timerlist, void *timer,
		unsigned int expire_s, unsigned int expire_ns);
extern DLL_APP void timer_mgr_run_timerlist(void *hd, void *timerlist,
		unsigned int s, unsigned int ns);
extern DLL_APP void timer_mgr_force_timeout(void *hd, void *timerlist,
		unsigned int s, unsigned int ns, timer_callback callback);

extern DLL_APP void *timer_mgr_open(int interval);
extern DLL_APP void timer_mgr_run(void *hd);
extern DLL_APP void timer_mgr_close(void *hd);

extern DLL_APP void *timer_mgr_add_pktq_timer(void *hd,
		unsigned int interval, void *data);
extern DLL_APP void timer_mgr_del_pktq_timer(void *hd, void *timer);
extern DLL_APP void *timer_mgr_add_hashtbl_timer(void *hd,
		unsigned int interval, void *data);
extern DLL_APP void timer_mgr_del_hashtbl_timer(void *hd, void *timer);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_TIMERMGR_34B76B10_2E045C45_37721BD0_H */
