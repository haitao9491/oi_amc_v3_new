/*
 *
 * rawsock.h - A brief description goes here.
 *
 */

#ifndef _HEAD_RAWSOCK_637F3C9A_44F6022F_4960CD03_H
#define _HEAD_RAWSOCK_637F3C9A_44F6022F_4960CD03_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void *rawsock_open(char *device,
		char *enet_dst, char *enet_src, unsigned short enet_type);
extern int rawsock_send(void *hd, unsigned char *data, int len);
extern int rawsock_copy_send(void *hd, unsigned char *data, int len);
extern void rawsock_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_RAWSOCK_637F3C9A_44F6022F_4960CD03_H */
