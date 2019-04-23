/*
 * (C) Copyright 2012
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * adapter_udp.h - A description goes here.
 *
 */

#ifndef _HEAD_ADAPTER_UDP_63AA6A4F_1B081C9B_14443047_H
#define _HEAD_ADAPTER_UDP_63AA6A4F_1B081C9B_14443047_H

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

#define ADAPTER_UDP_IOCTL_GETSTATS	1
#define ADAPTER_UDP_IOCTL_SETOPKTHDR	2
#define ADAPTER_UDP_IOCTL_SETMCINTF	3
#define ADAPTER_UDP_IOCTL_SETMCMEMBER	4

struct udp_stats {
	unsigned long long rpkts;
	unsigned long long rdrop;
	unsigned long long rok;
	unsigned long long rfull;

	unsigned long long tpkts;
	unsigned long long tdrop;
	unsigned long long tsent;
	unsigned long long tfull;
};

struct udp_mcinfo {
	char intf[32];
	char addr[32];
};

#if defined(__cplusplus)
extern "C" {
#endif

DLL_APP void *adapter_register_udp(int port, int (*running)(void));

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_UDP_63AA6A4F_1B081C9B_14443047_H */
