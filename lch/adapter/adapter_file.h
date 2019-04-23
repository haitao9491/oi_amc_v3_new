/*
 *
 * adapter_file.h - A brief description goes here.
 *
 * There must be a configuration section to run this adapter. An
 * example config is listed below.
 *

[Adapter.FILE]
# To determine how fast we will read packets from datafiles.
# speed =  1      : Strictly follow the pace of the datafiles.
# speed =  [2, 99]: 
# speed >= 100    : As fast as possible.
speed = 30

# To determine whether to repeat or not.
# loop = 1
# loop = 0
loop = 0

# To wait the specified number of seconds before returning the first packet
wait = 0

# To define where the packets are from. Multiple definitions allowed.
datafile = /path/to/datafiles/datafile.dat

 */

#ifndef _HEAD_ADAPTER_FILE_71A972AD_1AD7D98F_6B881560_H
#define _HEAD_ADAPTER_FILE_71A972AD_1AD7D98F_6B881560_H

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

DLL_APP void *adapter_register_file(unsigned long cfghd, char *section);
DLL_APP void *adapter_register_file_cfgfile(char *cfgfile, char *section);
DLL_APP void *adapter_register_file_cfgstr(char *cfgstr, char *section);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_FILE_71A972AD_1AD7D98F_6B881560_H */
