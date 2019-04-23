/*
 *
 * pkt.c - Manage packets
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#if defined(HAVE_ZLIB)
#include <zlib.h>
#endif
#else
#pragma warning(disable : 4996)
#include "zlib.h"
#include "zconf.h"
#include "misc.h"
#endif
#if defined(HAVE_PCAP)
#include <pcap.h>
#endif

#include "os.h"
#include "aplog.h"
#include "pkt.h"

#define PKT_FILETYPE_NORMAL 0
#define PKT_FILETYPE_PCAP   1

struct pkt_filep {
	char *filename;

	void *fp;
	int   compression;

	int     filetype;

#if defined(HAVE_PCAP)
	pcap_t *pcap_fp;
	char    errbuf[PCAP_ERRBUF_SIZE];
	char    filter_exp[128];
	struct bpf_program bpf;

	int     pcap_file_hdr_written;
#endif
};


char *display_time(unsigned int s, unsigned int ns, char *usrbuf)
{
	static char buffer[100];
	char *p;
	time_t tt = (time_t)s;

	struct tm *t = localtime(&tt);
	if (t == NULL)
		return NULL;

	p = usrbuf ? usrbuf : buffer;

	sprintf(p, "%d/%02d/%02d %02d:%02d:%02d.%u",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec,
			ns);

	return p;
}

void pkt_dump(char *file, char *data, int len)
{
	void          *lgwr;
	pkt_hdr       *ph;
	unsigned int   s, ns;
	int            rlen;
	char           timestr[64];

	if(!file || !data || (len < sizeof(pkt_hdr))) {
		LOGERROR("pkt_dump: invalid parameters.");
		return;
	}

	if((rlen = pkt_verify(data, len)) <= 0) {
		LOGERROR("pkt_dump: pkt_verify failed.");
		return;
	}

	lgwr = lgwr_open(file, LGWRLEVELDEBUG, 100);
	if (!lgwr) {
		LOGERROR("pkt_dump: open log file for writing failed.");
		return;
	}

	ph = (pkt_hdr *)data;

	pkthdr_get_ts(ph, &s, &ns);
	display_time(s, ns, timestr);

	lgwr_prt(lgwr, data, rlen, NULL, 0,
			LGWRSTRDEBUG "%s plen %d, type (0x%02x, 0x%02x), "
			"proto %u, sc %02x, device %u, channel %u",
			timestr,
			pkthdr_get_plen(ph),
			pkthdr_get_type(ph),
			pkthdr_get_subtype(ph),
			pkthdr_get_protocol(ph),
			pkthdr_get_sc(ph),
			pkthdr_get_device(ph),
			pkthdr_get_channel(ph));
	lgwr_close(lgwr);
}

void *pkt_open_file(char *filename, char *mode, int compression)
{
	int i;
	struct pkt_filep *pktfp;

	if (!filename || !mode || (compression < 0) || (compression > 9)) {
		LOGERROR("pkt_open_file: invalid parameters");
		return NULL;
	}

	if (strlen(mode) > 3) {
		LOGERROR("pkt_open_file: invalid mode: %s", mode);
		return NULL;
	}
	LOGDEBUG("pkt_open_file: file %s, mode %s, compression %d",
			filename, mode, compression);

	pktfp = (struct pkt_filep *)malloc(sizeof(struct pkt_filep));
	if (!pktfp) {
		LOGERROR("pkt_open_file: insufficient memory.");
		return NULL;
	}
	memset(pktfp, 0, sizeof(*pktfp));

	pktfp->filename = strdup(filename);

	i = (int)strlen(filename);
	if ((i <= 3) || (strcmp(filename + i - 3, ".gz") != 0))
		compression = 0;
	pktfp->compression = compression;

	pktfp->filetype = PKT_FILETYPE_NORMAL;
#if defined(HAVE_PCAP)
	if (((i > 5) && (strcmp(filename + i - 5, ".pcap") == 0)) ||
	    ((i > 4) && (strcmp(filename + i - 4, ".cap") == 0))) {
		pktfp->filetype = PKT_FILETYPE_PCAP;
		pktfp->pcap_file_hdr_written = 0;
		pktfp->compression = 0;
	}
#endif

	if (pktfp->compression) {
		char zmode[8];

		sprintf(zmode, "%s%d", mode, compression);
#if defined(HAVE_ZLIB)
		pktfp->fp = (void *)gzopen(filename, zmode);
#endif
	}
	else {
		pktfp->fp = (void *)fopen(filename, mode);
#if defined(HAVE_PCAP)
		if (pktfp->fp && (pktfp->filetype == PKT_FILETYPE_PCAP)) {
			pktfp->pcap_fp = pcap_fopen_offline((FILE *)pktfp->fp, pktfp->errbuf);
			if (pktfp->pcap_fp) {
				bpf_u_int32 mask = 0;

				strcpy(pktfp->filter_exp, "ip");
				if (pcap_compile(pktfp->pcap_fp,
							&(pktfp->bpf), pktfp->filter_exp, 0, mask) == -1) {
					LOGERROR("Unable to parse filter %s: %s",
							pktfp->filter_exp, pcap_geterr(pktfp->pcap_fp));
					pcap_freecode(&(pktfp->bpf));
					pcap_close(pktfp->pcap_fp);
					pktfp->pcap_fp = NULL;
				}
			}

			if (pktfp->pcap_fp) {
				if (pcap_setfilter(pktfp->pcap_fp, &(pktfp->bpf)) == -1) {
					LOGERROR("Unable to install filter %s: %s",
							pktfp->filter_exp, pcap_geterr(pktfp->pcap_fp));
					pcap_freecode(&(pktfp->bpf));
					pcap_close(pktfp->pcap_fp);
					pktfp->pcap_fp = NULL;
				}
			}
		}
#endif
	}

	if (pktfp->fp == NULL) {
		LOGERROR("pkt_open_file: open %s failed.", filename);
		if (pktfp->filename)
			free(pktfp->filename);
		free(pktfp);
		return NULL;
	}

	return pktfp;
}

static void *pkt_read_file_normal(FILE *fp)
{
	int      len;
	char    *buf;
	pkt_hdr  ph;

	if(fread(&ph, sizeof(ph), 1, fp) != 1) {
		if (feof(fp))
			return NULL;

		LOGERROR3("pkt_read_file_normal: fp %p, pos %lld, len %d, "
				"reading packet header failed.",
				fp, FTELL(fp), sizeof(pkt_hdr));
		return NULL;
	}
	len = pkthdr_get_plen(&ph);

	if ((buf = (char *)malloc(len)) == NULL) {
		LOGERROR("pkt_read_file_normal: Insufficient memory.");
		return NULL;
	}
	memcpy(buf, &ph, sizeof(ph));

	if(len == sizeof(pkt_hdr))
		return buf;

	if(fread(buf + sizeof(pkt_hdr), len - sizeof(pkt_hdr), 1, fp) != 1) {
		LOGERROR3("pkt_read_file_normal: fp %p, pos %lld, len %d, "
				"reading packet payload failed.",
				fp, FTELL(fp), len - sizeof(pkt_hdr));
		return NULL;
	}

	return buf;
}

#if defined(HAVE_ZLIB)
static void *pkt_read_file_gz(gzFile gzfp)
{
	pkt_hdr ph;
	char *buf;
	int len, rlen;

	rlen = gzread(gzfp, &ph, sizeof(ph));
	if (rlen == 0)
		return NULL;

	if (rlen != sizeof(pkt_hdr)) {
		LOGERROR("pkt_read_file_gz: error reading file.");
		return NULL;
	}
	len = pkthdr_get_plen(&ph);

	if ((buf = (char *)malloc(len)) == NULL) {
		LOGERROR("pkt_read_file_gz: Insufficient memory.");
		return NULL;
	}
	memcpy(buf, &ph, sizeof(ph));

	if (len == sizeof(pkt_hdr))
		return buf;

	rlen = gzread(gzfp, buf + sizeof(pkt_hdr), len - sizeof(pkt_hdr));
	if (rlen != (len - sizeof(pkt_hdr))) {
		LOGERROR("pkt_read_file: reading packet payload failed.");
		free(buf);
		return NULL;
	}

	return buf;
}
#endif

#if defined(HAVE_PCAP)
static void *pkt_read_file_pcap(struct pkt_filep *pktfp)
{
	int rc;
	pkt_hdr *ph = NULL;
	struct pcap_pkthdr *pcaphdr = NULL;
	unsigned char *pcapdata = NULL;

	if (pktfp->pcap_fp == NULL)
		return NULL;

	rc = pcap_next_ex(pktfp->pcap_fp, &pcaphdr, (const u_char **)&pcapdata);
	if (rc == 1) {
		ph = (pkt_hdr *)malloc(sizeof(pkt_hdr) + 2 + pcaphdr->caplen);
		if (ph) {
			ph->sc = 0;
			pkthdr_set_sync(ph);
			pkthdr_set_dlen(ph, pcaphdr->caplen + 2);
			pkthdr_set_type(ph, PKT_TYPE_DATA);
			pkthdr_set_subtype(ph, PKT_SUBTYPE_ETHERNET);
			pkthdr_set_protocol(ph, PKT_PT_AUTO);
			pkthdr_set_device(ph, 0);
			pkthdr_set_channel(ph, 0);
			pkthdr_set_ts(ph, pcaphdr->ts.tv_sec, pcaphdr->ts.tv_usec * 1000);
			memcpy(pkthdr_get_data(ph) + 2, pcapdata, pcaphdr->caplen);
		}
		else {
			LOGERROR("file %s: insufficient memory.", pktfp->filename);
		}
	}
	else {
		if (rc == 0) {
			LOGDEBUG("file %s: timeout expired.", pktfp->filename);
		}
		else if (rc == -2) {
			LOGINFO("file %s: no more packets.", pktfp->filename);
		}
		else {
			LOGERROR("file %s: error occurred.", pktfp->filename);
		}
	}

	return ph;
}
#endif

void *pkt_read_file(void *pfp)
{
	struct pkt_filep *pktfp = (struct pkt_filep *)pfp;

	if (!pktfp || !pktfp->fp) {
		LOGERROR1("pkt_read_file: Corrupted data structure: pfp %p",
				pktfp);
		return NULL;
	}

#if defined(HAVE_PCAP)
	if (pktfp->filetype == PKT_FILETYPE_PCAP)
		return pkt_read_file_pcap(pktfp);
#endif

	if (pktfp->compression)
#if defined(HAVE_ZLIB)
		return pkt_read_file_gz((gzFile)pktfp->fp);
#else
		return NULL;
#endif

	return pkt_read_file_normal((FILE *)pktfp->fp);
}

static int pkt_write_file_normal(FILE *fp, char *data, int len)
{
	int rlen;

	if((rlen = pkt_verify(data, len)) <= 0) {
		LOGERROR("pkt_write_file_normal: pkt_verify failed.");
		return -1;
	}

	if(fwrite(data, rlen, 1, fp) != 1) {
		LOGERROR3("pkt_write_file_normal: fp %p, data %p, len %d, write failed.",
				fp, data, rlen);
		return -1;
	}

	return rlen;
}

#if defined(HAVE_ZLIB)
static int pkt_write_file_gz(gzFile gzfp, char *data, int len)
{
	int rlen, wlen;

	if ((rlen = pkt_verify(data, len)) <= 0) {
		LOGERROR("pkt_write_file_gz: pkt_verify failed.");
		return -1;
	}

	wlen = gzwrite(gzfp, data, rlen);
	if (wlen == 0) {
		LOGERROR2("pkt_write_file_gz: data %p, len %d, write failed.",
				data, rlen);
		return -1;
	}

	return rlen;
}
#endif

#if defined(HAVE_PCAP)
static void pkt_write_file_pcap_header(struct pkt_filep *pktfp, pkt_hdr *ph)
{
	struct pcap_file_header hdr;
	size_t rc;
	unsigned char subtype;

	hdr.magic = 0xa1b2c3d4;
	hdr.version_major = 0x2;
	hdr.version_minor = 0x4;
	hdr.thiszone = 0;
	hdr.sigfigs = 0;
	hdr.snaplen = 0x00002000;

	subtype = pkthdr_get_subtype(ph);
	if (subtype == PKT_SUBTYPE_MTP)
		hdr.linktype = 0x8c;
	else if (subtype == PKT_SUBTYPE_LAPD)
		hdr.linktype = 177;
	else if (subtype == PKT_SUBTYPE_FRAMERELAY)
		hdr.linktype = 0x6b;
	else if (subtype == PKT_SUBTYPE_ETHERNET)
		hdr.linktype = 1;
	else
		hdr.linktype = 0x8c;

	rc = fwrite(&hdr, 1, sizeof(hdr), pktfp->fp);
	if (rc != sizeof(hdr)) {
		LOGERROR("file %s: write pcap file header failed: %d (%d).",
				pktfp->filename, rc, sizeof(hdr));
	}
}

static int pkt_write_file_pcap(struct pkt_filep *pktfp, char *data, int len)
{
	pkt_hdr *ph = (pkt_hdr *)data;
	size_t rc;
	int offset;
	struct pcap_pkthdr_32 {
		unsigned int sec;
		unsigned int usec;
		unsigned int caplen;
		unsigned int len;
	} pcaphdr;

	if (!data || (len <= 0))
		return 0;

	if (pktfp->fp == NULL)
		return 0;

	if (pktfp->pcap_file_hdr_written == 0) {
		pkt_write_file_pcap_header(pktfp, ph);
		pktfp->pcap_file_hdr_written = 1;
	}

	pcaphdr.sec = pkthdr_get_ts_s(ph);
	pcaphdr.usec = pkthdr_get_ts_ns(ph) / 1000;
	offset = (pkthdr_get_subtype(ph) == PKT_SUBTYPE_ETHERNET) ? 2 : 0;
	pcaphdr.caplen = pkthdr_get_dlen(ph) - offset;
	pcaphdr.len = pkthdr_get_dlen(ph) - offset;

	rc = fwrite(&pcaphdr, 1, sizeof(pcaphdr), pktfp->fp);
	if (rc != sizeof(pcaphdr)) {
		LOGERROR("file %s: write pcaphdr failed: %d (%d).",
				pktfp->filename, rc, sizeof(pcaphdr));
	}
	rc = fwrite(pkthdr_get_data(ph) + offset, 1, pcaphdr.len, pktfp->fp);
	if (rc != pcaphdr.len) {
		LOGERROR("file %s: write packet failed: %d (%d).",
				pktfp->filename, rc, pcaphdr.len);
	}

	return len;
}
#endif

int pkt_write_file(void *pfp, char *data, int len)
{
	struct pkt_filep *pktfp = (struct pkt_filep *)pfp;

	if (!pktfp || !pktfp->fp) {
		LOGERROR1("pkt_write_file: Corrupted data structure: pfp %p",
				pktfp);
		return -1;
	}

	if (!data || (len < sizeof(pkt_hdr))) {
		LOGERROR2("pkt_write_file: Invalid args: data %p, len %d",
				data, len);
		return -1;
	}

#if defined(HAVE_PCAP)
	if (pktfp->filetype == PKT_FILETYPE_PCAP)
		return pkt_write_file_pcap(pktfp, data, len);
#endif

	if (pktfp->compression)
#if defined(HAVE_ZLIB)
		return pkt_write_file_gz((gzFile)pktfp->fp, data, len);
#else
		return -1;
#endif

	return pkt_write_file_normal((FILE *)pktfp->fp, data, len);
}

void pkt_close_file(void *pfp)
{
	struct pkt_filep *pktfp = (struct pkt_filep *)pfp;

	if (!pktfp)
		return;

	if (pktfp->fp) {
		if (pktfp->compression) {
#if defined(HAVE_ZLIB)
			gzclose((gzFile)pktfp->fp);
#endif
		}
		else {
#if defined(HAVE_PCAP)
			if (pktfp->filetype == PKT_FILETYPE_PCAP) {
				if (pktfp->pcap_fp) {
					pcap_freecode(&(pktfp->bpf));
					pcap_close(pktfp->pcap_fp);
					pktfp->pcap_fp = NULL;
				}
				else {
					fclose((FILE *)pktfp->fp);
				}
			} else
#endif
			{
				fclose((FILE *)pktfp->fp);
			}
		}
	}

	if (pktfp->filename)
		free(pktfp->filename);

	free(pktfp);
}

int pkt_generate(unsigned char *buf, int size)
{
	int i;
	int len, ch;
	pkt_hdr *ph = (pkt_hdr *)buf;
	struct timeval t;

	ch = (int)(256.0 * (rand() / (RAND_MAX + 1.0)));
	do {
		len = 1 + (int)((float)size * (rand() / (RAND_MAX + 1.0)));
	} while (len < sizeof(pkt_hdr));

	for(i = 0; i < len; i++)
		*(buf + i) = (char)(ch + i);
	memset(ph, 0, sizeof(pkt_hdr));

	pkthdr_set_sync(ph);
	pkthdr_set_plen(ph, len);
	pkthdr_set_device(ph, 9);
	pkthdr_set_channel(ph, ch);
	gettimeofday(&t, NULL);
	pkthdr_set_ts(ph, (unsigned int)t.tv_sec, (unsigned int)t.tv_usec);

	return pkthdr_get_plen(ph);
}

int pkthdr_get_pkt_len(pkt_hdr *ph)
{
	return pkthdr_get_plen(ph);
}
