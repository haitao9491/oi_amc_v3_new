/*
 *
 * cthread.h - A brief description goes here.
 *
 */

#ifndef _HEAD_CTHREAD_2AE33A7C_2D207BC1_13FB83C8_H
#define _HEAD_CTHREAD_2AE33A7C_2D207BC1_13FB83C8_H

#include "os.h"

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

struct thread_arg {
	LPVOID arg;
	int    flag;
};

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *thread_open(LPTHREAD_START_ROUTINE thrfunc, LPVOID arg);
DLL_APP void thread_close(void *hd);
DLL_APP int thread_set_concurrency(int newlevel);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_CTHREAD_2AE33A7C_2D207BC1_13FB83C8_H */
