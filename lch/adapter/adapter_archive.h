/*
 * (C) Copyright 2008
 * Hu Chunlin <chunlin.hu@gmail.com>
 *
 * adapter_archive.h - A brief description goes here.
 *
 * There must be a configuration section to run this adapter. An
 * example config is listed below.
 *

[Adapter.Archive]
# This config will generate files: /tmp/rawdata200808221700.dat
path       = /tmp
prefix     = rawdata
datefmt    = %04d%02d%02d%02d%02d
#secfmt     =
suffix     = .dat
quota      = 0
period     = 30
concurrent = 1
disabled   = no

 *
 */

#ifndef _HEAD_ADAPTER_ARCHIVE_3A4AB998_6ABF27A3_4EE9C8D6_H
#define _HEAD_ADAPTER_ARCHIVE_3A4AB998_6ABF27A3_4EE9C8D6_H

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

DLL_APP void *adapter_register_archive(unsigned long cfghd, char *section,
		int (*format_output)(void *fp, void *buf, int len));

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_ARCHIVE_3A4AB998_6ABF27A3_4EE9C8D6_H */
