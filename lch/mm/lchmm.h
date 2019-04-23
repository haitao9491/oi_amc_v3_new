/*
 *
 * lchmm.h - A brief description goes here.
 *
 */

#ifndef _HEAD_LCHMM_08BBF0CD_451A6D9C_71C333B5_H
#define _HEAD_LCHMM_08BBF0CD_451A6D9C_71C333B5_H

#define LCHMM_MALLOC 0
#define LCHMM_CUSTOM 1
#define LCHMM_LAST   LCHMM_CUSTOM

#if defined(__cplusplus)
extern "C" {
#endif

extern void *lchmm_init(int type, int size, int nelement);
extern void *lchmm_nmalloc(void *hd, int nelement);
extern void *lchmm_malloc(void *hd);
extern void  lchmm_nfree(void *hd, void *ptr, int nelement);
extern void  lchmm_free(void *hd, void *ptr);
extern void  lchmm_exit(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_LCHMM_08BBF0CD_451A6D9C_71C333B5_H */
