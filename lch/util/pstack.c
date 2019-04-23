/*
 *
 * pstack.c - A brief description goes here.
 *
 */

/* 关于数据结构 ps_protocol_handle 的说明：
 *
 * name:
 *     本级协议的简要英文描述。
 *
 * pvid:
 *     通常在本级协议中有一个字段用于指明下一级封装的是什么协议。
 *     pvid就是用来定位这个字段的，根据这个字段名字去解码结果集中
 *     查找就能知道下一级是什么协议，进而可以选择相对应的下一级协议的
 *     解码函数去解码当前数据包的PAYLOAD部分的数据。
 *
 * protocol:
 *     唯一标志本级协议的ID值。
 *
 * sp_vid/sv_vid:
 *     正常情况下，在确定了下一级协议类型后，直接调用该下一级协议的
 *     解码程序来解码本级协议PAYLOAD部分的数据即可。
 *
 *     但也有可能出现这样的情况：本级协议的PAYLOAD部分封装了多个下一
 *     级协议的数据包。此时就不能简单地通过上述方法来完成解码。sp_vid和
 *     sv_vid就是为了解决这个问题而引入的，根据这两个字段名字去解码结果
 *     集中查找就可以得到两组，每组两个指针，一个指针指向下一级协议的协议
 *     类型，另一个指针指向下一级协议的协议数据。这样就可以对封装的多个
 *     下一级协议的数据包都进行相应的解码了。
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "os.h"
#include "aplog.h"
#include "hashtbl.h"
#include "pktdecode.h"
#include "pstack.h"

static int ps_result_ht_rel(void *key, void *value)
{
	struct fld_value *fv = (struct fld_value *)value;

	if (fv) {
		if (!fv->dontfree) {
			if ((fv->type == FLD_TYPE_STR) &&
			    (fv->data.str != NULL))
				free(fv->data.str);
			if ((fv->type == FLD_TYPE_USTR) &&
			    (fv->data.ustr != NULL))
				free(fv->data.ustr);
		}
		free(fv);
	}

	return 0;
}

struct ps_decoding_result *ps_result_init(void)
{
	struct ps_decoding_result *p;

	p = (struct ps_decoding_result *)malloc(sizeof(*p));
	if (p == NULL) {
		LOGERROR("ps_result_init: Insufficient memory.");
		return NULL;
	}

	p->valueht = hashtbl_open(128, NULL,NULL,ps_result_ht_rel, "psresult");
	if (p->valueht == NULL) {
		LOGERROR("ps_result_init: creating hashtbl failed.");
		free(p);
		return NULL;
	}

	return p;
}

int ps_result_clear(struct ps_decoding_result *result)
{
	if (result) {
		hashtbl_clear(result->valueht);
	}
	return 0;
}

void ps_result_exit(struct ps_decoding_result *result)
{
	if (result) {
		hashtbl_close(result->valueht);
		free(result);
	}
}

struct fld_value *ps_result_get_value(struct ps_decoding_result *result,
		unsigned long valueid, int seq)
{
	if (!result || !result->valueht || !valueid)
		return NULL;

	return (struct fld_value *)hashtbl_find_seq(result->valueht,
			(void *)valueid, NULL, seq);
}

void ps_result_delete_value(struct ps_decoding_result *result,
		unsigned long valueid)
{
	if (!result || !result->valueht || !valueid)
		return;

	hashtbl_delete(result->valueht, (void *)valueid);
}

int ps_result_push_value(struct ps_decoding_result *result,
		unsigned long vid, struct fld_value *value)
{
	struct fld_value *v;

	if (!result || !result->valueht || !vid || !value)
		return -1;

	v = (struct fld_value *)malloc(sizeof(*v));
	if (v == NULL) {
		LOGERROR("ps_result_push_value: Insufficient memory.");
		return -1;
	}
	memcpy(v, value, sizeof(*v));

	if (hashtbl_insert(result->valueht, (void *)vid, (void *)v) < 0) {
		LOGERROR("ps_result_push_value: insert into hashtbl failed.");
		free(v);
		return -1;
	}

	return 0;
}

struct ps_protocol_handle *ps_init_root(char *name, unsigned long pvid,
		unsigned long sp_vid, unsigned long sv_vid,
		int (*self_decode)(struct ps_protocol_handle *current,
			struct ps_decoding_result *result,
			unsigned char *data, int len, int *trailerlen),
		void *ctrl)
{
	struct ps_protocol_handle *p;

	if (!name || !self_decode) {
		LOGERROR1("ps_init_root: %s, invalid parameter.", name);
		return NULL;
	}

	p = (struct ps_protocol_handle *)malloc(sizeof(*p));
	if (p == NULL) {
		LOGERROR1("ps_init_root: %s, insufficient memory.", name);
		return NULL;
	}

	p->name = strdup(name);
	p->pvid = pvid;
	p->protocol = 0;
	p->level = 0;

	p->sp_vid = sp_vid;
	p->sv_vid = sv_vid;

	p->self_decode = self_decode;
	p->release = NULL;
	p->ctrl = ctrl;
	INIT_LIST_HEAD(&(p->sibling));
	INIT_LIST_HEAD(&(p->child));

	p->parent = NULL;

	return p;
}

struct ps_protocol_handle *ps_init(void)
{
	return ps_init_root("pkt", PVID_PKT_TYPE_SUBTYPE,
			0, 0, pkt_decode, NULL);
}

struct ps_protocol_handle *ps_register(struct ps_protocol_handle *parent,
		char *name, unsigned int protocol,
		unsigned long pvid,
		unsigned long sp_vid, unsigned long sv_vid,
		int (*self_decode)(struct ps_protocol_handle *current,
			struct ps_decoding_result *result,
			unsigned char *data, int len, int *trailerlen),
		void *ctrl)
{
	struct ps_protocol_handle *p;

	if (!parent || !name || !self_decode) {
		LOGERROR1("ps_register: %s, invalid parameter.", name);
		return NULL;
	}

	p = (struct ps_protocol_handle *)malloc(sizeof(*p));
	if (p == NULL) {
		LOGERROR1("ps_register: %s, insufficient memory.", name);
		return NULL;
	}

	p->name = strdup(name);
	p->pvid = pvid;
	p->protocol = protocol;
	p->level = parent->level + 1;

	p->sp_vid = sp_vid;
	p->sv_vid = sv_vid;

	p->self_decode = self_decode;
	p->release = NULL;
	p->ctrl = ctrl;
	INIT_LIST_HEAD(&(p->sibling));
	INIT_LIST_HEAD(&(p->child));

	p->parent = parent;

	list_add_tail(&(p->sibling), &(parent->child));

	return p;
}

static void ps_decode_next_list(struct ps_protocol_handle *p,
			struct ps_decoding_result *result)
{
	int rc, seq;
	struct fld_value *fvp = NULL, *fvd = NULL;
	struct list_head *pos;
	struct ps_protocol_handle *next;

	LOGDEBUG("L%d(%s 0x%x): sp_vid %x, sv_vid %x",
			p->level, p->name, p->protocol, p->sp_vid, p->sv_vid);

	seq = 0;
	while ((fvp = ps_result_get_value(result, p->sp_vid, seq)) != NULL) {
		LOGDEBUG("L%d(%s 0x%x): Payload's ULP: %x",
				p->level, p->name, p->protocol, fvp->data.ui);
		fvd = NULL;
		list_for_each(pos, &(p->child)) {
			next=list_entry(pos,struct ps_protocol_handle, sibling);
			LOGDEBUG("L%d(%s 0x%x): Registered ULP: %x",
					p->level, p->name, p->protocol,
					next->protocol);
			if (fvp->data.ui != next->protocol)
				continue;

			LOGDEBUG("L%d(%s 0x%x): Registered ULP: %x, matched",
					p->level, p->name, p->protocol,
					next->protocol);

			if (fvd == NULL)
				fvd = ps_result_get_value(result,p->sv_vid,seq);
			if (fvd == NULL) {
				LOGWARN("L%d(%s 0x%x): Seq %d (%s 0x%x)"
						", No payload data found.",
						p->level, p->name, p->protocol,
						seq,
						next->name, next->protocol);
				break;
			}
			LOGDEBUG("L%d(%s 0x%x): Registered ULP: %x"
					", Data %p, Length %d",
					p->level, p->name, p->protocol,
					next->protocol,
					fvd->data.ustr, fvd->length);

			rc=ps_decode(next, result, fvd->data.ustr, fvd->length);
			if (rc < 0) {
				LGWRWARN(fvd->data.ustr, fvd->length,
					"L%d(%s 0x%x): %d (%s 0x%x), failed:",
					p->level, p->name, p->protocol,
					seq, next->name, next->protocol);
			}
		}

		seq++;
	}
}

int ps_decode(struct ps_protocol_handle *p,
			struct ps_decoding_result *result,
			unsigned char *data, int len)
{
	int rc;
	int trailerlen;
	struct fld_value *fv = NULL;

	if (len <= 0)
		return 0;

	/* Decoding of the current level of protocol. */
	trailerlen = 0;
	if ((rc = p->self_decode(p, result, data, len, &trailerlen)) < 0) {
		LOGERROR3("L%d(%s 0x%x): self decoding failed.",
				p->level, p->name, p->protocol);
		return -1;
	}
	data += rc;
	len -= (rc + trailerlen);

	/* Decoding of multiple embedded payload data of next level
	 * protocols.
	 */
	if (p->sp_vid && p->sv_vid) {
		ps_decode_next_list(p, result);
	}

	/* Decoding of the next level of protocol. */
	if (p->pvid) {
		fv = ps_result_get_value(result, p->pvid, 0);
		if (fv) {
			struct list_head *pos;
			struct ps_protocol_handle *next;

			list_for_each(pos, &(p->child)) {
				next = list_entry(pos,
					struct ps_protocol_handle, sibling);
				if (fv->data.ui != next->protocol)
					continue;

				return ps_decode(next, result, data, len);
			}
		}
	}

	if (len <= 0)
		return 0;

	LGWRWARN(data, len, "L%d(%s 0x%x): protocol 0x%x: "
			"%d bytes of payload remain undecoded.",
			p->level, p->name, p->protocol,
			fv ? fv->data.ui : 0xffff, len);
	return len;
}

void ps_exit(struct ps_protocol_handle *psroot)
{
	struct list_head *pos, *n;
	struct ps_protocol_handle *p;

	if (psroot) {
		list_for_each_safe(pos, n, &(psroot->child)) {
			list_del(pos);
			p = list_entry(pos, struct ps_protocol_handle, sibling);
			ps_exit(p);
		}

		LOG3("L%d(%s 0x%x): handle released.",
				psroot->level, psroot->name, psroot->protocol);
		if (psroot->release)
			psroot->release();
		if (psroot->name)
			free(psroot->name);
		free(psroot);
	}
}
