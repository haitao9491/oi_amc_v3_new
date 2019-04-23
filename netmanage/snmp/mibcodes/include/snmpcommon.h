/*
 * snmpcommon.h
 */

#ifndef _HEAD_SNMPCOMMON_0E1D34C8_3ABF1190_017E695F_H 
#define _HEAD_SNMPCOMMON_0E1D34C8_3ABF1190_017E695F_H 

#define SNMP_MAX_BOARD_CNT       256

#define DEFINE_TABLE_ENTRY_DELETE_INVALID_FUNC(prefix) \
struct prefix ## _entry *prefix ## _del_invalid_entrys(struct prefix ## _entry *head) \
{ \
	struct prefix ## _entry *p, *prev, *n; \
 \
	if (!head) \
		return NULL; \
 \
	n = NULL; \
	while (head) { \
		n = head; \
		if (n->valid == 0) { \
			head = n->next; \
			free(n); \
			prefix ## _count--; \
		} \
		else { \
			break; \
		}	 \
	} \
 \
	if (!head) { \
		prefix ## _count = 0; \
		return NULL; \
	} \
 \
	prev = head; \
	p    = head->next; \
	while (p) { \
		n = p; \
		p = p->next; \
		if (n->valid == 0) { \
 \
			prev->next = p; \
			free(n); \
			prefix ## _count--; \
		} \
		else { \
			prev = n; \
		} \
	} \
 \
	return head; \
}\

#define DEFINE_TABLE_ENTRY_DELETE_FUNC(prefix) \
struct prefix ## _entry *prefix ## _delete_entry(struct prefix ## _entry *head, in_addr_t po, \
			int (*cmp_func)(struct prefix ## _entry *, in_addr_t )) \
{ \
	struct prefix ## _entry *p, *prev, *n; \
 \
	if (!head) \
		return NULL; \
 \
	n = NULL; \
	while (head) { \
		n = head; \
		if ((*cmp_func)(n, po) == 0) { \
			head = n->next; \
			free(n); \
			prefix ## _count--; \
		} \
		else { \
			break; \
		}	 \
	} \
 \
	if (!head) { \
		prefix ## _count = 0; \
		return NULL; \
	} \
 \
	prev = head; \
	p    = head->next; \
	while (p) { \
		n = p; \
		p = p->next; \
		if ((*cmp_func)(n, po) == 0) { \
 \
			prev->next = p; \
			free(n); \
			prefix ## _count--; \
		} \
		else { \
			prev = n; \
		} \
	} \
 \
	return head; \
}


#define DEFINE_TABLE_DELETE_FUNC(prefix) \
int prefix ## _delete(in_addr_t po) \
{ \
	prefix ## _head = prefix ## _delete_entry(prefix ## _head, po, prefix ## _compare); \
 \
	return 0; \
}


#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_SNMPCOMMON_0E1D34C8_3ABF1190_017E695F_H */

