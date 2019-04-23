/*
 *
 * hashtbl.c - A brief description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aplog.h"
#include "hash.h"
#include "list.h"
#include "hashtbl.h"

#ifdef WIN32
#pragma warning(disable : 4996 ; disable : 4018 )
#endif

struct hashtbl_hlentry {
	struct list_head hlnode;
	void *key;
	void *value;
};

struct hashtbl_hlhead {
	struct list_head hlhead;
};

struct hashtbl_handle {
	int order;
	int buckets;
	unsigned long (*hash_func)(void *key, unsigned int bits);
	int (*cmp_func)(void *a, void *b);
	int (*release)(void *key, void *value);

	struct hashtbl_hlhead *heads;

	unsigned long long cnt;
	char *name;

	unsigned long long num_access;
	unsigned long long num_access_compare;
};

static int hashtbl_def_cmp_func(void *a, void *b)
{
	unsigned long ua = (unsigned long)a;
	unsigned long ub = (unsigned long)b;

	if (ua == ub)
		return 0;
	if (ua < ub)
		return -1;
	return 1;
}

void *hashtbl_open(int buckets,
		unsigned long (*hash_func)(void *key, unsigned int bits),
		int (*cmp_func)(void *a, void *b),
		int (*release)(void *key, void *value), char *name)
{
	struct hashtbl_handle *hh;
	struct hashtbl_hlhead *head;
	int i = 1, order = 0;

	while (i < buckets) {
		i <<= 1;
		order++;
	}

	hh = malloc(sizeof(struct hashtbl_handle));
	if (hh == NULL)
		return NULL;
	memset(hh, 0, sizeof(struct hashtbl_handle));

	hh->heads = malloc(sizeof(struct hashtbl_hlhead) * i);
	if (hh->heads == NULL) {
		free(hh);
		return NULL;
	}
	for (head = hh->heads; head < hh->heads + i; head++) {
		INIT_LIST_HEAD(&(head->hlhead));
	}

	hh->order = order;
	hh->buckets = i;
	hh->hash_func = hash_func ? hash_func : hash_ptr;
	hh->cmp_func  = cmp_func ? cmp_func : hashtbl_def_cmp_func;
	hh->release   = release;
	hh->cnt = 0;
	hh->name = name ? strdup(name) : NULL;

	return hh;
}

static void *do_hashtbl_find(void *ht, void *key,
		int (*cmp_func)(void *a, void *b), int seq)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	struct list_head       *pos;
	unsigned long  bucket;
	int (*docmp)(void *a, void *b);
	int i = 0;

	bucket = hh->hash_func(key, hh->order);
	if (bucket >= hh->buckets)
		return NULL;

	head = hh->heads + bucket;
	if (list_empty(&(head->hlhead)))
		return NULL;

	docmp = cmp_func ? cmp_func : hh->cmp_func;

	hh->num_access++;
	/* hash collision, search the list */
	list_for_each(pos, &(head->hlhead)) {
		hh->num_access_compare++;
		entry = list_entry(pos, struct hashtbl_hlentry, hlnode);
		if (docmp(key, entry->key) == 0) {
			if (i == seq)
				return entry->value;
			i++;
		}
	}

	return NULL;
}

void *hashtbl_find(void *ht, void *key, int (*cmp_func)(void *a, void *b))
{
	return do_hashtbl_find(ht, key, cmp_func, 0);
}

void *hashtbl_find_seq(void *ht,
		void *key, int (*cmp_func)(void *a, void *b), int seq)
{
	return do_hashtbl_find(ht, key, cmp_func, seq);
}

int hashtbl_insert(void *ht, void *key, void *value)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	unsigned long  bucket;

	bucket = hh->hash_func(key, hh->order);
	if (bucket >= hh->buckets)
		return -1;
	head = hh->heads + bucket;

	entry = malloc(sizeof(struct hashtbl_hlentry));
	if (!entry)
		return -1;

	entry->key = key;
	entry->value = value;

	list_add_tail(&(entry->hlnode), &(head->hlhead));
	hh->cnt++;

	return 0;
}

static int hashtbl_do_delete(void *ht, void *key, int keepdata)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	struct list_head       *pos, *n;
	unsigned long  bucket;

	bucket = hh->hash_func(key, hh->order);
	if (bucket >= hh->buckets)
		return -1;

	head = hh->heads + bucket;
	if (list_empty(&(head->hlhead)))
		return -1;

	/* hash collision, search the list */
	hh->num_access++;
	list_for_each_safe(pos, n, &(head->hlhead)) {
		hh->num_access_compare++;
		entry = list_entry(pos, struct hashtbl_hlentry, hlnode);
		if (hh->cmp_func(key, entry->key) == 0) {
			list_del(pos);
			if (!keepdata) {
				if (hh->release)
					hh->release(entry->key, entry->value);
			}
			free(entry);
			hh->cnt--;
			return 0;
		}
	}

	return -1;
}

int hashtbl_delete(void *ht, void *key)
{
	return hashtbl_do_delete(ht, key, 0);
}

int hashtbl_delete_keepdata(void *ht, void *key)
{
	return hashtbl_do_delete(ht, key, 1);
}

int hashtbl_pop(void *ht, void **key, void **value)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	struct list_head       *pos, *n;

	if (!hh)
		return -1;

	for (head = hh->heads; head < hh->heads + hh->buckets; head++) {
		list_for_each_safe(pos, n, &(head->hlhead)) {
			entry = list_entry(pos, struct hashtbl_hlentry,hlnode);
			list_del(pos);
			*key = entry->key;
			*value = entry->value;
			free(entry);
			hh->cnt--;
			return 0;
		}
	}

	return -1;
}

int hashtbl_traverse(void *ht,
		void (*action)(void *key, void *value, void *arg), void *arg)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	struct list_head       *pos;
	int cnt = 0;

	if (!hh || !action)
		return cnt;

	for (head = hh->heads; head < hh->heads + hh->buckets; head++) {
		list_for_each(pos, &(head->hlhead)) {
			entry = list_entry(pos, struct hashtbl_hlentry,hlnode);
			action(entry->key, entry->value, arg);
			cnt++;
		}
	}

	return cnt;
}

int hashtbl_traverse_delete(void *ht,
		int (*ok_to_del)(void *key, void *value, void *arg), void *arg)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	struct list_head       *pos, *n;
	int cnt = 0, deleted = 0;

	if (!hh || !ok_to_del)
		return cnt;

	for (head = hh->heads; head < hh->heads + hh->buckets; head++) {
		list_for_each_safe(pos, n, &(head->hlhead)) {
			entry = list_entry(pos, struct hashtbl_hlentry,hlnode);
			if (ok_to_del(entry->key, entry->value, arg)) {
				list_del(pos);
				if (hh->release)
					hh->release(entry->key, entry->value);
				free(entry);
				hh->cnt--;
				deleted++;
			}
			else {
				cnt++;
			}
		}
	}
	if (deleted > 0) {
		LOG2("hashtbl(%s): traversing: %d items deleted.",
				hh->name ? hh->name : "null", deleted);
	}

	return cnt;
}

void hashtbl_get_mem(void *ht, unsigned long long *cnt,
		unsigned long long *access, unsigned long long *compares)
{
	struct hashtbl_handle *hh = (struct hashtbl_handle *)ht;

	if (hh) {
		if (cnt)
			*cnt = hh->cnt;
		if (access)
			*access = hh->num_access;
		if (compares)
			*compares = hh->num_access_compare;
	}
}

void hashtbl_clear(void *ht)
{
	struct hashtbl_handle  *hh = (struct hashtbl_handle *)ht;
	struct hashtbl_hlhead  *head;
	struct hashtbl_hlentry *entry;
	struct list_head       *pos, *n;

	if (!hh)
		return;

	for (head = hh->heads; head < hh->heads + hh->buckets; head++) {
		list_for_each_safe(pos, n, &(head->hlhead)) {
			entry = list_entry(pos, struct hashtbl_hlentry,hlnode);
			list_del(pos);
			if (hh->release)
				hh->release(entry->key, entry->value);
			free(entry);
		}
	}

	hh->cnt = 0;
}

void hashtbl_close(void *ht)
{
	struct hashtbl_handle *hh = (struct hashtbl_handle *)ht;

	LOG2("hashtbl(%s): %llu items to be removed.",
			hh->name ? hh->name : "null", hh->cnt);
	hashtbl_clear(ht);
	if (hh) {
		if (hh->name)
			free(hh->name);
		free(hh->heads);
		free(hh);
	}
}
