/*
 *
 * adapter_z006.h - Adapter interface of ip probe.
 *
 */

#ifndef _HEAD_ADAPTER_Z006_6C81E618_6BA239D1_237C43EC_H
#define _HEAD_ADAPTER_Z006_6C81E618_6BA239D1_237C43EC_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void *adapter_register_z006s(int startdevnum, int devnum, 
		int (*check_running)(void));
extern void *adapter_register_z006(int devnum, int (*check_running)(void));

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ADAPTER_Z006_6C81E618_6BA239D1_237C43EC_H */
