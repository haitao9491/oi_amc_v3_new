/*
 *
 * archive.h - A brief description goes here.
 *
 */

#ifndef _HEAD_ARCHIVE_08CFFAA9_4C80D603_2CB3953C_H
#define _HEAD_ARCHIVE_08CFFAA9_4C80D603_2CB3953C_H

#define ARCHIVE_PERIOD 0x0000000F
#define ARCHIVE_HOURLY 0x00000001
#define ARCHIVE_DAILY  0x00000002

#if defined(__cplusplus)
extern "C" {
#endif
extern void *archive_open(char *path, char *prefix, char *suffix,
		int quota, int period, int concurrent,
		int (*format_output)(void *fp, void *buf, int len));
extern int archive_set_datetime_fmt(void *hd, char *fmt_min, char *fmt_sec);
extern int archive_set_write_direct(void *hd, int direct);
extern int archive_write(void *hd, void *buf, int len, unsigned int timet);
extern void archive_close(void *hd);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_ARCHIVE_08CFFAA9_4C80D603_2CB3953C_H */
