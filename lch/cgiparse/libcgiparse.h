/*
 *
 * libcgiparse.h - A brief description goes here.
 *
 */

#ifndef _HEAD_LIBCGIPARSE_21BC08B2_3F7BD33F_1E6CBB88_H
#define _HEAD_LIBCGIPARSE_21BC08B2_3F7BD33F_1E6CBB88_H

#if defined(__cplusplus)
extern "C" {
#endif

extern void *cgiparse_open(char *method, char *data);
extern int   cgiparse_getvalue(void *h, char *fieldname, char *value, int size, int seq);
extern void  cgiparse_close(void *h);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_LIBCGIPARSE_21BC08B2_3F7BD33F_1E6CBB88_H */
