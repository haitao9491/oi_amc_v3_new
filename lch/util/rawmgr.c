/*
 *
 * rawmgr.c - To manage rawdata.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "os.h"
#include "aplog.h"
#include "pkt.h"
#include "list.h"
#include "hash.h"
#include "hashtbl.h"
#include "archive.h"
#include "rawmgr.h"
#ifdef WIN32
#pragma warning ( disable : 4996 )
#endif

#define RAWMGR_IDX_PREFIX     "rawdata"
#define RAWMGR_PKTIDX_PREFIX  "rawdata"
#define RAWMGR_RAWDATA_PREFIX "rawdata"

#define RAWMGR_IDX_SUFFIX     ".idx"
#define RAWMGR_PKTIDX_SUFFIX  ".pidx"
#define RAWMGR_RAWDATA_SUFFIX ".dat"

struct rawmgr_pktidx_record {
	unsigned int pkttime;
	unsigned int offset;
	unsigned int bytes;
};

struct rawmgr_ht_key {
	unsigned int starttime;
	unsigned int id;
};

struct rawmgr_ht_value {
	struct list_head list;

	struct rawmgr_ht_key *key;

	void *arch_idx;
	void *arch_pktidx;
};

struct rawmgr_ht_entry {
	struct list_head list;
	struct rawmgr_pktidx_record pktidx;
};

struct rawmgr_hd {
	void *ht;

	void *idx, *pktidx, *rawdata;

	char *path;
	int   period;
	int   concurrent;
};

static unsigned long rawmgr_ht_hash_func(void *key, unsigned int bits)
{
	struct rawmgr_ht_key *k = (struct rawmgr_ht_key *)key;
	unsigned long ul;

#if __WORDSIZE == 64
	ul = k->starttime;
	ul = (ul << 32) | k->id;
#else
	ul = (k->starttime & 0x00ffffff) | ((k->id & 0xff) << 24);
#endif

	return hash_long(ul, bits);
}

static int rawmgr_ht_cmp_func(void *a, void *b)
{
	struct rawmgr_ht_key *ka = (struct rawmgr_ht_key *)a;
	struct rawmgr_ht_key *kb = (struct rawmgr_ht_key *)b;

	if ((ka->starttime == kb->starttime) && (ka->id == kb->id))
		return 0;

	return 1;
}

static int rawmgr_ht_release(void *key, void *value)
{
	struct rawmgr_ht_key *k = (struct rawmgr_ht_key *)key;
	struct rawmgr_ht_value *v = (struct rawmgr_ht_value *)value;
	struct rawmgr_ht_entry *entry = NULL;
	struct rawmgr_idx_record idx;
	struct list_head *pos, *n;
	int off = -1;

	off = archive_write(v->arch_pktidx, v, 0, v->key->starttime);

	memset(&idx, 0, sizeof(idx));
	idx.id = v->key->id;
	idx.offset = (unsigned int)off;
	list_for_each_safe(pos, n, &(v->list)) {
		list_del(pos);
		entry = list_entry(pos, struct rawmgr_ht_entry, list);
		idx.pkts++;
		idx.bytes += entry->pktidx.bytes;
		free(entry);
	}

	if ((off != -1) && (idx.pkts > 0) && (idx.bytes > 0)) {
		archive_write(v->arch_idx, &idx, 0, v->key->starttime);
	}

	free(v);
	free(k);

	return 0;
}

static int rawmgr_write_idx(void *fp, void *buf, int len)
{
	struct rawmgr_idx_record *idx = (struct rawmgr_idx_record *)buf;

	fwrite(idx, sizeof(*idx), 1, (FILE *)fp);
#if defined(DEBUG)
	LOGDEBUG("RawMGR: Idx: id %u: offset %u, pkts %u, bytes %u",
			idx->id, idx->offset, idx->pkts, idx->bytes);
#endif

	return 0;
}

static int rawmgr_write_pktidx(void *fp, void *buf, int len)
{
	struct rawmgr_ht_value *v = (struct rawmgr_ht_value *)buf;
	struct rawmgr_ht_entry *pos;
	long fpos = ftell((FILE *)fp);
	long f;

	f = fpos;
	for (pos = list_entry((&(v->list))->next, struct rawmgr_ht_entry, list);
	     &pos->list != (&(v->list));
		 pos = list_entry(pos->list.next, struct rawmgr_ht_entry, list))
	{
		fwrite(&(pos->pktidx), sizeof(pos->pktidx), 1, (FILE *)fp);
#if defined(DEBUG)
		LOGDEBUG("RawMGR: Pktidx: starttime %u, id %u: %ld: "
				"pkttime %u, offset %u, bytes %u",
				v->key->starttime, v->key->id, f,
				pos->pktidx.pkttime, pos->pktidx.offset, pos->pktidx.bytes);
#endif
		f += sizeof(pos->pktidx);
	}

	return (int)fpos;
}

static int rawmgr_write_rawdata(void *fp, void *buf, int len)
{
	long fpos = ftell((FILE *)fp);

	fwrite(buf, len, 1, (FILE *)fp);
#if defined(DEBUG)
	LOGDEBUG("RawMGR: Rawdata: offset %ld, %d bytes", fpos, len);
#endif

	return (int)fpos;
}

static struct rawmgr_ht_entry *rawmgr_write_pkt(struct rawmgr_hd *rhd,
		pkt_hdr *ph)
{
	struct rawmgr_ht_entry *entry;
	int off;
	unsigned int pkttime;

	pkttime = pkthdr_get_ts_s(ph);
	pkttime = pkttime - (pkttime % rhd->period);
	off = archive_write(rhd->rawdata, ph, pkthdr_get_plen(ph), pkttime);
	if (off == -1) {
		return NULL;
	}

	if ((entry = (struct rawmgr_ht_entry *)malloc(sizeof(*entry))) == NULL) {
		return NULL;
	}

	entry->pktidx.pkttime = pkttime;
	entry->pktidx.offset = (unsigned int)off;
	entry->pktidx.bytes = pkthdr_get_plen(ph);
	INIT_LIST_HEAD(&(entry->list));

	return entry;
}

static struct rawmgr_ht_value *rawmgr_ht_find(struct rawmgr_hd *rhd,
		unsigned int starttime, unsigned int id)
{
	struct rawmgr_ht_key    key;

	key.starttime = starttime;
	key.id = id;

	return (struct rawmgr_ht_value *)hashtbl_find(rhd->ht, &key, NULL);
}

static struct rawmgr_ht_value *rawmgr_ht_find_insert(struct rawmgr_hd *rhd,
		unsigned int starttime, unsigned int id)
{
	struct rawmgr_ht_key   *k, key;
	struct rawmgr_ht_value *v;

	key.starttime = starttime;
	key.id = id;
	v = (struct rawmgr_ht_value *)hashtbl_find(rhd->ht, &key, NULL);
	if (v != NULL)
		return v;

	k = (struct rawmgr_ht_key *)malloc(sizeof(*k));
	if (k == NULL)
		return NULL;
	k->starttime = starttime;
	k->id = id;

	v = (struct rawmgr_ht_value *)malloc(sizeof(*v));
	if (v == NULL) {
		free(k);
		return NULL;
	}
	INIT_LIST_HEAD(&(v->list));
	v->key = k;
	v->arch_idx = rhd->idx;
	v->arch_pktidx = rhd->pktidx;

	if (hashtbl_insert(rhd->ht, k, v) < 0) {
		free(k);
		free(v);
		return NULL;
	}

	return v;
}

static int rawmgr_ht_delete(struct rawmgr_hd *rhd,
		unsigned int starttime, unsigned int id)
{
	struct rawmgr_ht_key   key;

	key.starttime = starttime;
	key.id = id;

	return hashtbl_delete(rhd->ht, &key);
}

static char *rawmgr_read_datestr(unsigned int timet, char *str)
{
	time_t    t = (time_t)timet;
	struct tm now;

#ifndef WIN32
	localtime_r((const time_t *)&t, &now);
#else
	_localtime64_s(&now, (const time_t *)&t);
#endif
	sprintf(str, "%04d%02d%02d%02d%02d",
			now.tm_year + 1900, now.tm_mon + 1, now.tm_mday,
			now.tm_hour, now.tm_min);

	return str;
}

static int rawmgr_read_idx(struct rawmgr_hd *rhd,
		unsigned int starttime, unsigned int id, struct rawmgr_idx_record *idx)
{
	int rc = -1;
	char datestr[32];
	char filename[256];
	FILE *fp;

	sprintf(filename, "%s%c%s%s%s",
			rhd->path, FILENAME_SEPERATOR, RAWMGR_IDX_PREFIX,
			rawmgr_read_datestr(starttime - (starttime % rhd->period), datestr),
			RAWMGR_IDX_SUFFIX);
	if ((fp = fopen(filename, "rb")) == NULL) {
		LOGERROR("RawMGR: Read: Failed open file %s", filename);
		return -1;
	}

	while (!feof(fp)) {
		if (fread(idx, sizeof(*idx), 1, fp) != 1) {
			LOGERROR("RawMGR: Idx: %s: failed reading record", filename);
			break;
		}

		if (idx->id != id)
			continue;

		rc = 0;
		break;
	}
	fclose(fp);

	return rc;
}

static int rawmgr_read_pkt(struct rawmgr_hd *rhd,
		struct rawmgr_pktidx_record *rec, unsigned char *buf)
{
	int rc = -1;
	char datestr[32];
	char filename[256];
	FILE *fp = NULL;

	sprintf(filename, "%s%c%s%s%s", rhd->path, FILENAME_SEPERATOR,
			RAWMGR_RAWDATA_PREFIX,
			rawmgr_read_datestr(rec->pkttime, datestr), RAWMGR_RAWDATA_SUFFIX);
	if ((fp = fopen(filename, "rb")) == NULL) {
		LOGERROR("RawMGR: ReadPkt: Failed open file %s", filename);
		goto rawmgr_read_pkt_error;
	}

	if (fseek(fp, rec->offset, SEEK_SET) == -1) {
		LOGERROR("RawMGR: ReadPkt: Failed locating %s to offset %u",
				filename, rec->offset);
		goto rawmgr_read_pkt_error;
	}

	if (fread(buf, rec->bytes, 1, fp) != 1) {
		LOGERROR("RawMGR: ReadPkt: Failed reading pkt from %s at offset %u",
				filename, rec->offset);
		goto rawmgr_read_pkt_error;
	}

	rc = 0;

rawmgr_read_pkt_error:
	if (fp)
		fclose(fp);

	return rc;
}

static int rawmgr_read_pktidx_from_file(struct rawmgr_hd *rhd,
		unsigned int starttime, struct rawmgr_idx_record *idx,
		struct rawmgr_pktidx_record *rec)
{
	char datestr[32];
	char filename[256];
	FILE *fp = NULL;

	sprintf(filename, "%s%c%s%s%s", rhd->path, FILENAME_SEPERATOR,
			RAWMGR_PKTIDX_PREFIX,
			rawmgr_read_datestr(starttime - (starttime % rhd->period), datestr),
			RAWMGR_PKTIDX_SUFFIX);
	if ((fp = fopen(filename, "rb")) == NULL) {
		LOGERROR("RawMGR: ReadPkts: Failed open %s", filename);
		return -1;
	}

	if (fseek(fp, idx->offset, SEEK_SET) == -1) {
		LOGERROR("RawMGR: ReadPkts: Failed locating %s to offset %u",
				filename, idx->offset);
		fclose(fp);
		return -1;
	}

	if (fread(rec, sizeof(*rec), idx->pkts, fp) != idx->pkts) {
		LOGERROR("RawMGR: ReadPkts: Failed reading pktidx from %s at offset %u",
				filename, idx->offset);
		fclose(fp);
		return -1;
	}

	fclose(fp);
	return 0;
}

static int rawmgr_read_pktidx_from_mem(struct rawmgr_hd *rhd,
		unsigned int starttime, struct rawmgr_idx_record *idx,
		struct rawmgr_pktidx_record *rec)
{
	struct list_head       *pos;
	struct rawmgr_ht_value *v;
	struct rawmgr_ht_entry *entry;
	unsigned int            pkts = 0;

	v = rawmgr_ht_find(rhd, starttime, idx->id);
	if (v == NULL)
		return -1;

	list_for_each(pos, &(v->list)) {
		entry = list_entry(pos, struct rawmgr_ht_entry, list);
		memcpy(rec, &(entry->pktidx), sizeof(*rec));
		rec++;
		pkts++;
		if (pkts >= idx->pkts)
			break;
	}

	return 0;
}

static struct rawmgr_pktidx_record *rawmgr_read_pktidx(
		struct rawmgr_hd *rhd,
		unsigned int starttime, struct rawmgr_idx_record *idx)
{
	struct rawmgr_pktidx_record *rec = NULL;

	rec = (struct rawmgr_pktidx_record *)malloc(sizeof(*rec) * idx->pkts);
	if (rec == NULL) {
		LOGEMERGENCY("RawMGR: ReadPkts: Insufficient memory.");
		return NULL;
	}

	if (rawmgr_read_pktidx_from_mem(rhd, starttime, idx, rec) < 0) {
		if (rawmgr_read_pktidx_from_file(rhd, starttime, idx, rec) < 0) {
			free(rec);
			return NULL;
		}
	}

	return rec;
}

static int rawmgr_read_pkts(struct rawmgr_hd *rhd, unsigned int starttime,
		struct rawmgr_idx_record *idx, unsigned char *buf)
{
	unsigned int pkt = 0;
	struct rawmgr_pktidx_record *rec = NULL, *p;

	rec = rawmgr_read_pktidx(rhd, starttime, idx);
	if (rec == NULL)
		return -1;

	for (pkt = 0, p = rec; pkt < idx->pkts; pkt++, p++) {
#if defined(DEBUG)
		LOGDEBUG("RawMGR: ReadPkts: pkt %d: pkttime %u, offset %u, bytes %u",
				pkt, p->pkttime, p->offset, p->bytes);
#endif
		if (rawmgr_read_pkt(rhd, p, buf) < 0) {
			LOGERROR("RawMGR: ReadPkts: Failed reading pkt %d: "
					"pkttime %u, offset %u, bytes %u",
					pkt, p->pkttime, p->offset, p->bytes);
			free(rec);
			return -1;
		}
		buf += p->bytes;
	}

	free(rec);
	return 0;
}

void *rawmgr_open(char *path, int period, int concurrent, int buckets)
{
	struct rawmgr_hd *hd;

	if (!path || !*path || (period <= 0) || (concurrent <= 0))
		return NULL;

	hd = (struct rawmgr_hd *)malloc(sizeof(*hd));
	if (hd == NULL) {
		return NULL;
	}
	memset(hd, 0, sizeof(*hd));

	if ((hd->path = strdup(path)) == NULL) {
		goto rawmgr_open_error;
	}
	hd->period = period * 60;
	hd->concurrent = concurrent;

	hd->ht = hashtbl_open(buckets, rawmgr_ht_hash_func,
			rawmgr_ht_cmp_func, rawmgr_ht_release, "RawMGR");
	if (hd->ht == NULL) {
		goto rawmgr_open_error;
	}

	hd->idx = archive_open(path, RAWMGR_IDX_PREFIX,
			RAWMGR_IDX_SUFFIX, 0, period, hd->concurrent, rawmgr_write_idx);
	hd->pktidx = archive_open(path, RAWMGR_PKTIDX_PREFIX,
			RAWMGR_PKTIDX_SUFFIX, 0, period, hd->concurrent,
			rawmgr_write_pktidx);
	hd->rawdata = archive_open(path, RAWMGR_RAWDATA_PREFIX,
			RAWMGR_RAWDATA_SUFFIX, 0, period, hd->concurrent,
			rawmgr_write_rawdata);
	if ((hd->idx == NULL) || (hd->pktidx == NULL) || (hd->rawdata == NULL)) {
		goto rawmgr_open_error;
	}
	archive_set_write_direct(hd->idx, 1);
	archive_set_write_direct(hd->pktidx, 1);
	archive_set_write_direct(hd->rawdata, 1);
	archive_set_datetime_fmt(hd->idx, "%04d%02d%02d%02d%02d", NULL);
	archive_set_datetime_fmt(hd->pktidx, "%04d%02d%02d%02d%02d", NULL);
	archive_set_datetime_fmt(hd->rawdata, "%04d%02d%02d%02d%02d", NULL);

	return hd;

rawmgr_open_error:
	rawmgr_close(hd);
	return NULL;
}

int rawmgr_write(void *hd, unsigned int starttime, unsigned int id,
		pkt_hdr *ph, int endflag)
{
	struct rawmgr_hd *rhd = (struct rawmgr_hd *)hd;
	struct rawmgr_ht_value *v;
	struct rawmgr_ht_entry *entry;

	if (!rhd)
		return -1;

	if (ph == NULL) /* cdr timer out, delete hashtable */
	{
		if (rawmgr_ht_delete(rhd, starttime, id) < 0) {
			LOGERROR("RawMGR: Closing: starttime %u, id %u: Error in hashtbl",
					starttime, id);
			return -1;
		}

		return 0;
	}

#if defined(DEBUG)
	LOGDEBUG("RawMGR: Writing: starttime %u, id %u: %p, %d bytes, end? %s",
			starttime, id, ph, pkthdr_get_plen(ph), endflag ? "Yes" : "No");
#endif

	entry = rawmgr_write_pkt(rhd, ph);
	if (entry == NULL) {
		LOGERROR("RawMGR: Writing: starttime %u, id %u: "
				"Failed: %p, %d bytes, end? %s",
				starttime, id, ph, pkthdr_get_plen(ph), endflag ? "Yes" : "No");
		return -1;
	}

	if ((starttime == 0) && (id == 0)) {
		free(entry);
		return 0;
	}

	v = rawmgr_ht_find_insert(rhd, starttime, id);
	if (v == NULL) {
		free(entry);
		LOGERROR("RawMGR: Writing: starttime %u, id %u: Error in hashtbl",
				starttime, id);
		return -1;
	}

	list_add_tail(&(entry->list), &(v->list));

	if (endflag) {
		if (rawmgr_ht_delete(rhd, starttime, id) < 0) {
			LOGERROR("RawMGR: Closing: starttime %u, id %u: Error in hashtbl",
					starttime, id);
			return -1;
		}
	}

	return 0;
}

static int rawmgr_read_info_in_mem(void *hd, unsigned int starttime,
		unsigned int id, struct rawmgr_idx_record *idx)
{
	struct rawmgr_hd *rhd = (struct rawmgr_hd *)hd;
	struct list_head       *pos;
	struct rawmgr_ht_value *v;
	struct rawmgr_ht_entry *entry;

	v = rawmgr_ht_find(rhd, starttime, id);
	if (v == NULL)
		return -1;

	memset(idx, 0, sizeof(*idx));
	idx->id = id;
	list_for_each(pos, &(v->list)) {
		entry = list_entry(pos, struct rawmgr_ht_entry, list);
		idx->pkts++;
		idx->bytes += entry->pktidx.bytes;
	}

	return 0;
}

int rawmgr_read_info(void *hd, unsigned int starttime,
	unsigned int id, struct rawmgr_idx_record *idx)
{
	struct rawmgr_hd *rhd = (struct rawmgr_hd *)hd;

	if (!rhd)
		return -1;

	if (rawmgr_read_info_in_mem(hd, starttime, id, idx) < 0) {
		if (rawmgr_read_idx(rhd, starttime, id, idx) < 0) {
			LOGERROR("RawMGR: Read: starttime %u, id %u: "
					"failed reading idx file.", starttime, id);
			return -1;
		}
	}

	return (int)idx->bytes;
}

int rawmgr_read(void *hd, unsigned int starttime, unsigned int id,
		unsigned char *buf, int *size)
{
	struct rawmgr_hd *rhd = (struct rawmgr_hd *)hd;
	struct rawmgr_idx_record idx;

	if (!rhd)
		return -1;

	if (rawmgr_read_info_in_mem(hd, starttime, id, &idx) < 0) {
		if (rawmgr_read_idx(rhd, starttime, id, &idx) < 0) {
			LOGERROR("RawMGR: Read: starttime %u, id %u: "
					"failed reading idx file.", starttime, id);
			return -1;
		}
	}

	if (*size < (int)(idx.bytes)) {
		LOGERROR("RawMGR: Read: starttime %u, id %u: "
				"Buffer too small: %d, %u bytes data.",
				starttime, id, *size, idx.bytes);
		return -1;
	}

	if (rawmgr_read_pkts(rhd, starttime, &idx, buf) < 0) {
		LOGERROR("RawMGR: Read: starttime %u, id %u: "
				"failed reading packets.", starttime, id);
		return -1;
	}
	*size = (int)(idx.bytes);
#if defined(DEBUG)
	LOGDEBUG("RawMGR: Read: starttime %u, id %u: offset %u, pkts %u, bytes %d",
			starttime, id, idx.offset, idx.pkts, idx.bytes);
#endif

	return (int)(idx.pkts);
}

void rawmgr_get_mem(void *hd, unsigned long long *cnt,
		unsigned long long *access, unsigned long long *compares)
{
	struct rawmgr_hd *rhd = (struct rawmgr_hd *)hd;

	if (rhd) {
		hashtbl_get_mem(rhd->ht, cnt, access, compares);
	}
}

void rawmgr_close(void *hd)
{
	struct rawmgr_hd *rhd = (struct rawmgr_hd *)hd;

	if (rhd) {
		if (rhd->ht) {
			hashtbl_close(rhd->ht);
			rhd->ht = NULL;
		}

		if (rhd->idx) {
			archive_close(rhd->idx);
			rhd->idx = NULL;
		}

		if (rhd->pktidx) {
			archive_close(rhd->pktidx);
			rhd->pktidx = NULL;
		}

		if (rhd->rawdata) {
			archive_close(rhd->rawdata);
			rhd->rawdata = NULL;
		}

		if (rhd->path) {
			free(rhd->path);
			rhd->path = NULL;
		}

		free(rhd);
	}
}
