/*
 *
 * pstack.h - Protocol Stack.
 *
 */

#ifndef _HEAD_PSTACK_64278998_1E202CCB_09A47B3B_H
#define _HEAD_PSTACK_64278998_1E202CCB_09A47B3B_H

#include "list.h"
#include "fldvalue.h"

struct ps_decoding_result {
	void *valueht;
};

struct ps_protocol_handle {
	char         *name;
	unsigned long pvid;
	unsigned int  protocol;
	int           level;

	unsigned long sp_vid;
	unsigned long sv_vid;

	int (*self_decode)(struct ps_protocol_handle *current,
			struct ps_decoding_result *result,
			unsigned char *data, int len, int *trailerlen);
	void (*release)(void);

	void *ctrl;

	struct list_head sibling;
	struct list_head child;

	struct ps_protocol_handle *parent;
};

#define ps_result_push_value_g(result, fv, fldt, fld, vid, v, rc) { \
	(fv).type = (fldt); \
	(fv).length = sizeof((fv).data.fld); \
	(fv).data.fld = (v); \
	(fv).dontfree = 0; \
	if (ps_result_push_value((result), (vid), &(fv)) < 0) \
		return (rc); \
}

#define ps_result_push_value_gn(result, fv, fldt, fld, vid, v, rc) { \
	(fv).type = (fldt); \
	(fv).length = sizeof((fv).data.fld); \
	(fv).data.fld = (v); \
	(fv).dontfree = 0; \
	rc = ps_result_push_value((result), (vid), &(fv)); \
}

#define ps_result_push_value_s(result, fv, fldt, fldl, fld, vid, v, rc) { \
	(fv).type = (fldt); \
	(fv).length = (fldl); \
	(fv).data.fld = (v); \
	(fv).dontfree = 0; \
	rc = ps_result_push_value((result), (vid), &(fv)); \
}

#define ps_result_push_value_sk(result, fv, fldt, fldl, fld, vid, v, rc) { \
	(fv).type = (fldt); \
	(fv).length = (fldl); \
	(fv).data.fld = (v); \
	(fv).dontfree = 1; \
	rc = ps_result_push_value((result), (vid), &(fv)); \
}

#if defined(__cplusplus)
extern "C" {
#endif

extern struct ps_decoding_result *ps_result_init(void);
extern int ps_result_clear(struct ps_decoding_result *result);
extern void ps_result_exit(struct ps_decoding_result *result);

extern struct fld_value *ps_result_get_value(struct ps_decoding_result *result,
		unsigned long valueid, int seq);
extern void ps_result_delete_value(struct ps_decoding_result *result,
		unsigned long valueid);
extern int ps_result_push_value(struct ps_decoding_result *result,
		unsigned long vid, struct fld_value *value);

extern struct ps_protocol_handle *ps_init_root(char *name, unsigned long pvid,
		unsigned long sp_vid, unsigned long sv_vid,
		int (*self_decode)(struct ps_protocol_handle *current,
			struct ps_decoding_result *result,
			unsigned char *data, int len, int *trailerlen),
		void *ctrl);
extern struct ps_protocol_handle *ps_init(void);
extern struct ps_protocol_handle *ps_register(struct ps_protocol_handle *parent,
		char *name, unsigned int protocol,
		unsigned long pvid,
		unsigned long sp_vid, unsigned long sv_vid,
		int (*self_decode)(struct ps_protocol_handle *current,
			struct ps_decoding_result *result,
			unsigned char *data, int len, int *trailerlen),
		void *ctrl);
extern int ps_decode(struct ps_protocol_handle *p,
			struct ps_decoding_result *result,
			unsigned char *data, int len);
extern void ps_exit(struct ps_protocol_handle *psroot);

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PSTACK_64278998_1E202CCB_09A47B3B_H */
