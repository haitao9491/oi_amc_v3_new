/*
 *
 * hashtbl.h - A brief description goes here.
 *
 */

#ifndef _HEAD_HASHTBL_2AD9A77F_69948251_4CD506DB_H
#define _HEAD_HASHTBL_2AD9A77F_69948251_4CD506DB_H

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

extern DLL_APP void *hashtbl_open(int buckets,
		unsigned long (*hash_func)(void *key, unsigned int bits),
		int (*cmp_func)(void *a, void *b),
		int (*release)(void *key, void *value), char *name);
extern DLL_APP void *hashtbl_find(void *ht, void *key, int (*cmp)(void *a, void *b));
extern void *hashtbl_find_seq(void *ht,
		void *key, int (*cmp)(void *a, void *b), int seq);
extern DLL_APP int   hashtbl_insert(void *ht, void *key, void *value);
extern int   hashtbl_delete(void *ht, void *key);
extern int   hashtbl_delete_keepdata(void *ht, void *key);
extern int   hashtbl_pop(void *ht, void **key, void **value);
extern int   hashtbl_traverse(void *ht,
		void (*action)(void *key, void *value, void *arg), void *arg);
extern int hashtbl_traverse_delete(void *ht,
		int (*ok_to_del)(void *key, void *value, void *arg), void *arg);
extern void  hashtbl_get_mem(void *ht, unsigned long long *cnt,
		unsigned long long *access, unsigned long long *compares);
extern void  hashtbl_clear(void *ht);
extern DLL_APP void  hashtbl_close(void *ht);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_HASHTBL_2AD9A77F_69948251_4CD506DB_H */
