/*
 * (C) Copyright 2017
 * Xu Ronghui <glory.hsu@gmail.com>
 *
 * grpsm.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grpsm.h"

#define DEBUG

#if defined(DEBUG)
#define LOGERROR(fmt, args...)		fprintf(stderr, "[ERROR] " fmt "\n", ##args)
#define LOGINFO(fmt, args...)		fprintf(stderr, "[INFO] " fmt "\n", ##args)
#define LOGDEBUG(fmt, args...)		fprintf(stderr, "[DEBUG] " fmt "\n", ##args)
#else
#define LOGERROR(fmt, args...)		do { } while (0)
#define LOGINFO(fmt, args...)		do { } while (0)
#define LOGDEBUG(fmt, args...)		do { } while (0)
#endif

#define GRPSM_SQ_MAX	256

#define GRPSM_SQ_STATUS_DEFAULT		0x00
#define GRPSM_SQ_STATUS_GOT		0x01
#define GRPSM_SQ_STATUS_FINISHED	0x02

#define GRPSM_LO_MFI_BITS		5
#define GRPSM_HO_MFI_BITS		12
#define GRPSM_LO_MFI_DELTA_THRESHOLD	5
#define GRPSM_HO_MFI_DELTA_THRESHOLD	10


struct grpsm_sqlink {
	struct link_info **links;
	unsigned int	   num_links;
};

struct grpsm_sqlinks {
	struct grpsm_sqlink sqlinks[GRPSM_SQ_MAX];
};

struct grpsm_svc {
	unsigned char	   channel_rate;
	unsigned char	   svc_type;
	struct link_info **links;
	unsigned int       num_links;

	struct grpsm_sqlink sqlinks[GRPSM_SQ_MAX];
	/*
	unsigned int       sq_count[GRPSM_SQ_MAX];
	*/
	unsigned char	   sq_status;
	unsigned char	   sq_pre;
};

struct grpsm_fiber {
	unsigned char	   fiber;

	/* invalid links */
	struct link_info **nlinks;
	unsigned int       num_nlinks;

	/* success links which pass detection */
	struct link_info **olinks;
	unsigned int       num_olinks;

	/* with VC */
	struct grpsm_svc  *vcs;
	unsigned int       num_vcs;

	/* with LCAS */
	struct grpsm_svc  *lcas;
	unsigned int       num_lcas;
};

struct grpsm_ctl {
	struct link_info   *links;
	unsigned int        num_links;

	struct grpsm_fiber *fibers;
	unsigned int        num_fibers;

	struct group_cfg   *vgroups;
	unsigned int        num_vgroups;

	unsigned char	    lcas_got;
	unsigned char       vc_partial;
};


void *grpsm_open()
{
	struct grpsm_ctl *ctl;

	ctl = (struct grpsm_ctl *)malloc(sizeof(*ctl));
	if (ctl == NULL) {
		LOGERROR("grpsm(open): failed to allocate handle.");
		return NULL;
	}

	memset(ctl, 0, sizeof(*ctl));
	return ctl;
}

static void grpsm_exit_cfg(struct group_cfg *cfg)
{
	if (cfg && cfg->chs) {
		free(cfg->chs);
		cfg->chs = NULL;
		cfg->num_chs = 0;
	}
}

static void grpsm_exit_svc(struct grpsm_svc *svc)
{
	struct grpsm_sqlink *sqlink;
	int i;

	if (!svc)
		return;

	if (svc->links) {
		free(svc->links);
		svc->links = NULL;
		svc->num_links = 0;
	}

	for (i = 0, sqlink = &svc->sqlinks[0]; i < GRPSM_SQ_MAX; i++, sqlink++) {
		if (sqlink->links) {
			free(sqlink->links);
			sqlink->links = NULL;
			sqlink->num_links = 0;
		}
	}
}

static void grpsm_exit_fiber(struct grpsm_fiber *fiber)
{
	struct grpsm_svc *svc;
	int i;

	if (!fiber)
		return;

	if (fiber->lcas) {
		for (i = 0, svc = fiber->lcas; i < fiber->num_lcas; i++, svc++) {
			grpsm_exit_svc(svc);
		}
		free(fiber->lcas);
		fiber->lcas = NULL;
		fiber->num_lcas = 0;
	}

	if (fiber->vcs) {
		for (i = 0, svc = fiber->vcs; i < fiber->num_vcs; i++, svc++) {
			grpsm_exit_svc(svc);
		}
		free(fiber->vcs);
		fiber->vcs = NULL;
		fiber->num_vcs = 0;
	}

	if (fiber->olinks) {
		free(fiber->olinks);
		fiber->olinks = NULL;
		fiber->num_olinks = 0;
	}

	if (fiber->nlinks) {
		free(fiber->nlinks);
		fiber->nlinks = NULL;
		fiber->num_nlinks = 0;
	}
}

static void grpsm_exit(struct grpsm_ctl *ctl)
{
	struct group_cfg *cfg;
	struct grpsm_fiber *fiber;
	int i;

	ctl->lcas_got = 0;

	if (ctl->vgroups) {
		for (i = 0, cfg = ctl->vgroups; i < ctl->num_vgroups; i++, cfg++) {
			grpsm_exit_cfg(cfg);
		}
		free(ctl->vgroups);
		ctl->vgroups = NULL;
		ctl->num_vgroups = 0;
	}

	if (ctl->fibers) {
		for (i = 0, fiber = ctl->fibers; i < ctl->num_fibers; i++, fiber++) {
			grpsm_exit_fiber(fiber);
		}
		free(ctl->fibers);
		ctl->fibers = NULL;
		ctl->num_fibers = 0;
	}

	if (ctl->links) {
		free(ctl->links);
		ctl->links = NULL;
		ctl->num_links = 0;
	}
}

static int is_link_in_group(struct link_info *link, struct group_cfg *groupcfg, int num_groups, int *index)
{
	struct group_cfg *cfg;
	struct group_channel *channel;
	int i, j;

	if (!link || !groupcfg)
		return 0;

	for (i = 0, cfg = groupcfg; i < num_groups; i++, cfg++) {
		if (!cfg->chs)
			continue;

		for (j = 0, channel = cfg->chs; j < cfg->num_chs; j++, channel++) {
			if ((link->fiber == channel->fiber) && (link->channel == channel->channel)) {
				if (index) {
					*index = i;
				}
				return 1;
			}
		}
	}

	return 0;
}

static int grpsm_init_svc(struct grpsm_svc *svc)
{
	struct link_info *link;
	struct grpsm_sqlink *sqlink;
	int i;

	for (i = 0; i < svc->num_links; i++) {
		link = svc->links[i];
		sqlink = &svc->sqlinks[link->sq];
		sqlink->num_links++;
	}

	for (i = 0; i < GRPSM_SQ_MAX; i++) {
		sqlink = &svc->sqlinks[i];
		if (sqlink->num_links > 0) {
			sqlink->links = (struct link_info **)malloc(sizeof(struct link_info *) * sqlink->num_links);
			if (sqlink->links == NULL) {
				LOGERROR("grpsm(init_svc): failed to allocate links for sqlink[%d]", i);
				return -1;
			}
			memset(sqlink->links, 0, sizeof(struct link_info *) * sqlink->num_links);
			sqlink->num_links = 0;
		}
	}

	for (i = 0; i < svc->num_links; i++) {
		link = svc->links[i];
		sqlink = &svc->sqlinks[link->sq];
		sqlink->links[sqlink->num_links++] = link;
	}

#if 0
	for (i = 0; i < GRPSM_SQ_MAX; i++) {
		LOGDEBUG("SQ[%d]: %d links", i, svc->sqlinks[i].num_links);
	}
#endif

	LOGDEBUG("grpsm(init_svc): svc[%d, %d]: Total %d links loaded.",
			svc->channel_rate, svc->svc_type, svc->num_links);

	return 0;
}

static int grpsm_init_fiber(struct grpsm_fiber *fiber,
		struct grpsm_ctl *ctl, struct group_cfg *groupcfg, int num_groups)
{
	unsigned char  vcs[256], lcas[256], *p;
	unsigned short vcnt[256], lcnt[256], *q;
	unsigned int  num_nlinks, num_olinks, num_vcs, num_lcas, *num, num_links;
	struct link_info *link;
	struct grpsm_svc *svc;
	int i, j, found;

	num_nlinks = 0;
	num_olinks = 0;
	num_vcs = 0;
	num_lcas = 0;

	memset(vcnt, 0, sizeof(vcnt));
	memset(lcnt, 0, sizeof(lcnt));

	for (i = 0, link = ctl->links; i < ctl->num_links; i++, link++) {
		if (link->fiber != fiber->fiber)
			continue;

		if (link->vc_valid == 0) {
			num_nlinks++;
		}
		else {
			if (is_link_in_group(link, groupcfg, num_groups, NULL)) {
				num_olinks++;
			}
			else {
				if (!link->is_lcas) {
					p = vcs;
					q = vcnt;
					num = &num_vcs;
				}
				else {
					p = lcas;
					q = lcnt;
					num = &num_lcas;
				}

				found = 0;
				for (j = 0; j < *num; j++) {
					unsigned char cr = (p[j] >> 4) & 0xf;
					unsigned char st = (p[j] >> 0) & 0xf;
					if ((link->channel_rate == cr) && (link->svc_type == st)) {
						found = 1;
						break;
					}
				}
				if (found) {
					q[j]++;
				}
				else {
					p[*num] = ((link->channel_rate & 0xf) << 4) | (link->svc_type & 0xf);
					q[*num] = 1;
					(*num)++;
				}
			}
		}
	}

	if (num_nlinks > 0) {
		fiber->nlinks = (struct link_info **)malloc(sizeof(struct link_info *) * num_nlinks);
		if (fiber->nlinks == NULL) {
			LOGERROR("grpsm(init_fiber): failed to allocate invalid links.");
			return -1;
		}
		memset(fiber->nlinks, 0, sizeof(struct link_info *) * num_nlinks);
	}

	if (num_olinks > 0) {
		fiber->olinks = (struct link_info **)malloc(sizeof(struct link_info *) * num_olinks);
		if (fiber->olinks == NULL) {
			LOGERROR("grpsm(init_fiber): failed to allocate ok links.");
			return -1;
		}
		memset(fiber->olinks, 0, sizeof(struct link_info *) * num_olinks);
	}

	if (num_vcs > 0) {
		fiber->vcs = (struct grpsm_svc *)malloc(sizeof(struct grpsm_svc) * num_vcs);
		if (fiber->vcs == NULL) {
			LOGERROR("grpsm(init_fiber): failed to allocate vc service|channelrate bundles.");
			return -1;
		}
		memset(fiber->vcs, 0, sizeof(struct grpsm_svc) * num_vcs);
		fiber->num_vcs = num_vcs;
	}

	if (num_lcas > 0) {
		fiber->lcas = (struct grpsm_svc *)malloc(sizeof(struct grpsm_svc) * num_lcas);
		if (fiber->lcas == NULL) {
			LOGERROR("grpsm(init_fiber): failed to allocate lcas service|channelrate bundles.");
			return -1;
		}
		memset(fiber->lcas, 0, sizeof(struct grpsm_svc) * num_lcas);
		fiber->num_lcas = num_lcas;
	}

	for (i = 0, svc = fiber->vcs; i < num_vcs; i++, svc++) {
		svc->channel_rate = (vcs[i] >> 4) & 0xf;
		svc->svc_type     = (vcs[i] >> 0) & 0xf;
		num_links = vcnt[i];
		svc->links = (struct link_info **)malloc(sizeof(struct link_info *) * num_links);
		if (svc->links == NULL) {
			LOGERROR("grpsm(init_fiber): failed to allocate links for VC[%d](%d, %d).",
					i, svc->channel_rate, svc->svc_type);
			return -1;
		}
		memset(svc->links, 0, sizeof(struct link_info *) * num_links);
	}
	for (i = 0, svc = fiber->lcas; i < num_lcas; i++, svc++) {
		svc->channel_rate = (lcas[i] >> 4) & 0xf;
		svc->svc_type     = (lcas[i] >> 0) & 0xf;
		num_links = lcnt[i];
		svc->links = (struct link_info **)malloc(sizeof(struct link_info *) * num_links);
		if (svc->links == NULL) {
			LOGERROR("grpsm(init_fiber): failed to allocate links for LCAS[%d](%d, %d).",
					i, svc->channel_rate, svc->svc_type);
			return -1;
		}
		memset(svc->links, 0, sizeof(struct link_info *) * num_links);
	}

	for (i = 0, link = ctl->links; i < ctl->num_links; i++, link++) {
		if (link->fiber != fiber->fiber)
			continue;

		if (link->vc_valid == 0) {
			fiber->nlinks[fiber->num_nlinks++] = link;
		}
		else {
			if (is_link_in_group(link, groupcfg, num_groups, NULL)) {
				fiber->olinks[fiber->num_olinks++] = link;
			}
			else {
				if (!link->is_lcas) {
					svc = fiber->vcs;
					num = &fiber->num_vcs;
				}
				else {
					svc = fiber->lcas;
					num = &fiber->num_lcas;
				}

				found = 0;
				for (j = 0; j < *num; j++, svc++) {
					if ((link->channel_rate == svc->channel_rate) &&
					    (link->svc_type == svc->svc_type)) {
						found = 1;
						break;
					}
				}
				if (found) {
					svc->links[svc->num_links++] = link;
				}
			}
		}
	}

	for (i = 0, svc = fiber->vcs; i < fiber->num_vcs; i++, svc++) {
		if (grpsm_init_svc(svc) != 0) {
			LOGERROR("grpsm(init_fiber): failed to init VC bundle[%d]", i);
			return -1;
		}
	}
	for (i = 0, svc = fiber->lcas; i < fiber->num_lcas; i++, svc++) {
		if (grpsm_init_svc(svc) != 0) {
			LOGERROR("grpsm(init_fiber): failed to init LCAS bundle[%d]", i);
			return -1;
		}
	}

	LOGDEBUG("grpsm(init_fiber): fiber[%d]: Total %d invalid links, %d ok links,"
			"%d vcs, %d lcas loaded.",
			fiber->fiber, fiber->num_nlinks, fiber->num_olinks,
			fiber->num_vcs, fiber->num_lcas);

	return 0;
}

static int grpsm_init(struct grpsm_ctl *ctl, struct group_cfg *groupcfg, int num_groups)
{
	unsigned char fibs[256];
	unsigned int  num_fibs = 0;
	struct link_info *link;
	struct grpsm_fiber *fiber;
	int i, j, found;

	for (i = 0, link = ctl->links; i < ctl->num_links; i++, link++) {
		found = 0;
		for (j = 0; j < num_fibs; j++) {
			if (link->fiber == fibs[j]) {
				found = 1;
				break;
			}
		}
		if (!found) {
			fibs[num_fibs++] = link->fiber;
		}
	}

	ctl->fibers = (struct grpsm_fiber *)malloc(sizeof(struct grpsm_fiber) * num_fibs);
	if (ctl->fibers == NULL) {
		LOGERROR("grpsm(init): failed to allocate fibers.");
		return -1;
	}
	memset(ctl->fibers, 0, sizeof(struct grpsm_fiber) * num_fibs);
	ctl->num_fibers = num_fibs;

	for (i = 0, fiber = ctl->fibers; i < num_fibs; i++, fiber++) {
		fiber->fiber = fibs[i];

		if (grpsm_init_fiber(fiber, ctl, groupcfg, num_groups) != 0) {
			LOGERROR("grpsm(init): failed to init fiber[%d]", fiber->fiber);
			goto grpsm_init_error;
		}
	}

	LOGDEBUG("grpsm(init): Total %d fibers loaded.", ctl->num_fibers);

	return 0;

grpsm_init_error:
	return -1;
}

/* Parameters:
 *   groupcfg   : Successful group configuration list;
 *   num_groups : Number of successful groups;
 *
 * Return:
 *   rc         : -1 - internal error
 *                 0 - pushed
 */
int grpsm_push(void *hd, struct link_info *linkinfo, int num_links, struct group_cfg *groupcfg, int num_groups)
{
	struct grpsm_ctl *ctl = (struct grpsm_ctl *)hd;

	if (!ctl || !linkinfo || (num_links <= 0)) {
		LOGERROR("grpsm(push): invalid parameters.");
		return -1;
	}

	grpsm_exit(ctl);

	ctl->links = (struct link_info *)malloc(sizeof(struct link_info) * num_links);
	if (ctl->links == NULL) {
		LOGERROR("grpsm(push): failed to allocate links.");
		return -1;
	}
	memcpy(ctl->links, linkinfo, sizeof(struct link_info) * num_links);
	ctl->num_links = num_links;

	LOGDEBUG("grpsm(push): Total %d links loaded.", ctl->num_links);

	return grpsm_init(ctl, groupcfg, num_groups);
}

static void grpsm_update_vgroups(struct grpsm_ctl *ctl, struct group_cfg *groupcfg, int num_groups)
{
	int i, num = 0;
	struct group_cfg *cfg, *dcfg;

	for (i = 0, cfg = groupcfg; i < num_groups; i++, cfg) {
		if (cfg->result)
			num++;
	}

	if (num <= 0)
		return;

	cfg = ctl->vgroups;
	num += ctl->num_vgroups;

	ctl->vgroups = (struct group_cfg *)malloc(sizeof(struct group_cfg) * num);
	if (ctl->vgroups == NULL) {
		LOGERROR("grpsm(update_vgroups): failed to allocate %d groups.", num);
		ctl->vgroups = cfg;
		return;
	}

	memcpy(ctl->vgroups, cfg, sizeof(struct group_cfg) * ctl->num_vgroups);
	free(cfg);

	for (i = 0, cfg = groupcfg; i < num_groups; i++, cfg) {
		if (cfg->result) {
			dcfg = &ctl->vgroups[ctl->num_vgroups];
			dcfg->chs = (struct group_channel *)malloc(sizeof(struct group_channel) * cfg->num_chs);
			if (dcfg->chs == NULL) {
				LOGERROR("grpsm(update_vgroups): failed to allocate channels: %d.", cfg->num_chs);
			}
			else {
				memcpy(dcfg->chs, cfg->chs, cfg->num_chs);
				dcfg->num_chs = cfg->num_chs;
				ctl->num_vgroups++;
			}
		}
	}
}

static void grpsm_append_cfg_channels(struct group_cfg **m, int *num_m,
		struct group_channel *chs, int num_chs)
{
	struct group_cfg *cfg;
	struct group_channel *ch;
	int num, i, j;

	if (!m || !num_m || !chs || (num_chs <= 0))
		return;

	LOGDEBUG("grpsm(append_cfg_channels): pre: base %p", *m);

	cfg = *m;
	num = *num_m;
	num += 1;

	*m = (struct group_cfg *)malloc(sizeof(struct group_cfg) * num);
	if (*m == NULL) {
		LOGERROR("grpsm(group_cfg_append): failed to allocate group: %d", num);
		*m = cfg;
		goto append_release_chs;
	}
	if (cfg) {
		memcpy(*m, cfg, sizeof(struct group_cfg) * (*num_m));
		free(cfg);
	}

	cfg = (*m) + (*num_m);
	cfg->chs = chs;
	cfg->num_chs = num_chs;
	cfg->result = 0;
	*num_m = num;

	for (i = 0, cfg = *m; i < *num_m; i++, cfg++) {
		for (j = 0, ch = cfg->chs; j < cfg->num_chs; j++, ch++) {
			LOGDEBUG("grpsm(append_cfg_channels): group[%d], channel[%d]: <%d, %d>",
					i, j, ch->fiber, ch->channel);
		}
	}

	LOGDEBUG("grpsm(append_cfg_channels): post: base %p", *m);

	return;

append_release_chs:
	free(chs);
}

static int grpsm_get_lcas_from_svc(struct grpsm_svc *svc, struct group_cfg **cfg)
{
	struct grpsm_sqlink *sqlink;
	struct link_info **lmlinks, *link, *lmlink;
	struct group_channel *chs;
	unsigned char sq_max = 0;
	unsigned int num_lmlinks = 0;
	unsigned int num_chs = 0;
	int i, j, m;
	unsigned char link_match, group_match;

	struct group_cfg *grpcfg = NULL;
	int num_groups = 0;

	/* Found max SQ */
	for (i = 0; i < GRPSM_SQ_MAX; i++) {
		sqlink = &svc->sqlinks[i];
		if (sqlink->num_links == 0)
			break;
	}
	if (i == 0) {
		/* No link with SQ of [0] */
		return 0;
	}
	else {
		sq_max = i - 1;
	}

	/* Collect links with last_member setted. */
	lmlinks = (struct link_info **)malloc(sizeof(struct link_info *) * svc->num_links);
	if (lmlinks == NULL) {
		LOGERROR("grpsm(get_lcas_from_svc): failed to allcoate lm links: %d", svc->num_links);
		return -1;
	}
	memset(lmlinks, 0, sizeof(struct link_info *) * svc->num_links);
	num_lmlinks = 0;

	for (i = sq_max; i >= 0; i--) {
		sqlink = &svc->sqlinks[i];
		for (j = 0; j < sqlink->num_links; j++) {
			link = sqlink->links[j];
			if (link->is_last_member) {
				lmlinks[num_lmlinks++] = link;
			}
		}
	}

	LOGDEBUG("grpsm(get_lcas_from_svc): Total %d links with last member setted.",
			num_lmlinks);

	/* Match group */
	num_groups = 0;
	for (i = 0; i < num_lmlinks; i++) {
		lmlink = lmlinks[i];
		chs = (struct group_channel *)malloc(sizeof(struct group_channel) * (lmlink->sq + 1));
		if (chs == NULL) {
			LOGERROR("grpsm(get_lcas_from_svc): failed to allocate channels: %d, SQ %d", i, lmlink->sq);
			continue;
		}
		memset(chs, 0, sizeof(struct group_channel) * (lmlink->sq + 1));
		num_chs = 0;

		LOGDEBUG("grpsm(get_lcas_from_svc): lmlink[%d]@%p: <%d, %d>, sq %d, pre %d, cur %d",
				i, lmlink, lmlink->fiber, lmlink->channel,
				lmlink->sq, lmlink->pre_gid, lmlink->cur_gid);

		chs[lmlink->sq].fiber = lmlink->fiber;
		chs[lmlink->sq].fiber_rate = lmlink->fiber_rate;
		chs[lmlink->sq].channel = lmlink->channel;
		chs[lmlink->sq].channel_rate = lmlink->channel_rate;
		num_chs = 1;

		group_match = 1;
		for (j = lmlink->sq - 1; j >= 0; j--) {
			sqlink = &svc->sqlinks[j];
			link_match = 0;
			for (m = 0; m < sqlink->num_links; m++) {
				link = sqlink->links[m];
				if ((lmlink->pre_gid == link->pre_gid) ||
				    (lmlink->pre_gid == link->cur_gid) ||
				    (lmlink->cur_gid == link->pre_gid) ||
				    (lmlink->cur_gid == link->cur_gid)) {
					if (lmlink->mfi == link->mfi) {
						chs[j].fiber = link->fiber;
						chs[j].fiber_rate = link->fiber_rate;
						chs[j].channel = link->channel;
						chs[j].channel_rate = link->channel_rate;
						num_chs++;
						link_match = 1;

						LOGDEBUG("grpsm(get_lcas_from_svc): link[%d]@%p: <%d, %d>, sq %d, pre %d, cur %d",
								m, link, link->fiber, link->channel,
								link->sq, link->pre_gid, link->cur_gid);

						break;
					}
				}
			}
			if (link_match == 0) {
				group_match = 0;
				break;
			}
		}
		if (group_match) {
			grpsm_append_cfg_channels(&grpcfg, &num_groups, chs, num_chs);
		}
		else {
			free(chs);
		}
	}

	free(lmlinks);

	if (num_groups > 0) {
		if (cfg)
			*cfg = grpcfg;
	}

	return num_groups;
}

static void grpsm_append_cfg(struct group_cfg **m, int *num_m, struct group_cfg *n, int num_n)
{
	struct group_cfg *cfg;
	int num, i;

	if (!m || !num_m || !n || (num_n <= 0))
		return;

	cfg = *m;
	num = *num_m;
	num += num_n;

	*m = (struct group_cfg *)malloc(sizeof(struct group_cfg) * num);
	if (*m == NULL) {
		LOGERROR("grpsm(group_cfg_append): failed to allocate group: %d", num);
		*m = cfg;
		goto append_release_n;
	}
	if (cfg) {
		memcpy(*m, cfg, sizeof(struct group_cfg) * (*num_m));
		free(cfg);
	}
	memcpy((*m) + (*num_m), n, sizeof(struct group_cfg) * num_n);
	*num_m = num;

	free(n);
	return;

append_release_n:
	for (i = 0, cfg = n; i < num_n; i++, cfg++) {
		grpsm_exit_cfg(cfg);
	}
	free(n);
}

static int grpsm_get_lcas(struct grpsm_ctl *ctl, struct group_cfg **groupcfg)
{
	struct grpsm_fiber *fiber;
	struct grpsm_svc *svc;
	struct group_cfg *grpcfg = NULL, *cfg;
	int i, j, num_groups = 0, rc;

	for (i = 0, fiber = ctl->fibers; i < ctl->num_fibers; i++, fiber++) {
		for (j = 0, svc = fiber->lcas; j < fiber->num_lcas; j++, svc++) {
			rc = grpsm_get_lcas_from_svc(svc, &cfg);
			if (rc > 0) {
				grpsm_append_cfg(&grpcfg, &num_groups, cfg, rc);
			}
			LOGDEBUG("grpsm(get_lcas): fiber[%d]: %d groups.", fiber->fiber, rc);
		}
	}

	if (num_groups > 0) {
		if (groupcfg)
			*groupcfg = grpcfg;
	}

	return num_groups;
}

static int grpsm_remove_olink_from_svc(struct grpsm_fiber *fiber, struct grpsm_svc *svc,
		struct group_channel *channel)
{
	struct grpsm_sqlink *sqlink;
	struct link_info *link;
	int i, j;

	for (i = 0; i < GRPSM_SQ_MAX; i++) {
		sqlink = &svc->sqlinks[i];
		for (j = 0; j < sqlink->num_links; j++) {
			link = sqlink->links[j];
			if ((fiber->fiber == channel->fiber) &&
			    (link->channel == channel->channel)) {
				if (j == sqlink->num_links - 1) {
					sqlink->links[j] = NULL;
					sqlink->num_links--;
				}
				else {
					memmove(sqlink->links + j, sqlink->links + j + 1,
							sizeof(struct link_info *) * (sqlink->num_links - (j + 1)));
					sqlink->num_links--;
				}

				return 1;
			}
		}
	}

	return 0;
}

static void grpsm_remove_olinks_from_svc(struct grpsm_fiber *fiber, struct grpsm_svc *svc,
		struct group_cfg *groupcfg, int num_groups)
{
	struct group_cfg *cfg;
	struct group_channel *channel;
	int i, j;
	int total = 0;

	for (i = 0, cfg = groupcfg; i < num_groups; i++, cfg++) {
		if (cfg->result == 0)
			continue;

		for (j = 0, channel = cfg->chs; j < cfg->num_chs; j++, channel++) {
			total += grpsm_remove_olink_from_svc(fiber, svc, channel);
		}
	}

	LOGDEBUG("grpsm(remove_olinks_from_svc): fiber %d, svc[%d, %d]: %d links removed.",
			fiber->fiber, svc->channel_rate, svc->svc_type, total);
}

static int grpsm_near_mfi(struct link_info *blink, struct link_info *link)
{
	int delta, d1, d2, threshold, mfidist;

	if (blink->vc_order == GRPSM_ORDER_HO) {
		threshold = GRPSM_HO_MFI_DELTA_THRESHOLD;
		mfidist = 4096;
	}
	else {
		threshold = GRPSM_LO_MFI_DELTA_THRESHOLD;
		mfidist = 32;
	}

	if (blink->mfi >= link->mfi) {
		d1 = blink->mfi - link->mfi;
		d2 = mfidist + link->mfi - blink->mfi;
	}
	else {
		d1 = link->mfi - blink->mfi;
		d2 = mfidist + blink->mfi - link->mfi;
	}
	delta = (d1 < d2) ? d1 : d2;

	if (delta <= threshold)
		return 1;

	return 0;
}

static int grpsm_init_sqlinks(struct grpsm_sqlinks *sqlinks, struct grpsm_svc *svc,
		struct link_info *blink, unsigned char sq)
{
	struct grpsm_sqlink *ssq, *dsq;
	struct link_info *link;
	int i, j, num;

	dsq = &sqlinks->sqlinks[sq];
	dsq->links = (struct link_info **)malloc(sizeof(struct link_info *));
	if (dsq->links == NULL) {
		LOGERROR("grpsm(init_sqlinks): failed to allocate links: %d, SQ %d", 1, sq);
		return -1;
	}
	dsq->links[0] = blink;
	dsq->num_links = 1;

	for (i = sq - 1; i >= 0; i--) {
		ssq = &svc->sqlinks[i];
		dsq = &sqlinks->sqlinks[i];

		num = 0;
		for (j = 0; j < ssq->num_links; j++) {
			link = ssq->links[j];
			if (grpsm_near_mfi(blink, link)) {
				LOGDEBUG("grpsm(init_sqlinks): SQ %d: <%d, %d> @ %p",
						i, link->fiber, link->channel, link);
				num++;
			}
		}

		LOGDEBUG("grpsm(init_sqlinks): SQ %d: num %d", i, num);

		if (num == 0) {
			/* No valid links between SQ[0] and SQ(sq) */
			return -1;
		}

		dsq->links = (struct link_info **)malloc(sizeof(struct link_info *) * num);
		if (dsq->links == NULL) {
			LOGERROR("grpsm(init_sqlinks): failed to allocate links: %d, SQ %d", num, i);
			return -1;
		}
		memset(dsq->links, 0, sizeof(struct link_info *) * num);

		for (j = 0; j < ssq->num_links; j++) {
			link = ssq->links[j];
			if (grpsm_near_mfi(blink, link)) {
				dsq->links[dsq->num_links++] = link;
			}
		}
	}

	return 0;
}

static int grpsm_cfg_from_sqlinks(struct grpsm_sqlinks *sqlinks, unsigned char sq,
		struct group_cfg **cfg)
{
	struct group_cfg *grpcfg, *gcfg;
	struct grpsm_sqlink *sqlink;
	struct link_info *link;
	int num_groups, i, j, k, l;
	int cm, num, times, index;

	num_groups = 1;
	for (i = 0; i <= sq; i++) {
		sqlink = &sqlinks->sqlinks[i];
		num_groups *= sqlink->num_links;
	}

	grpcfg = (struct group_cfg *)malloc(sizeof(struct group_cfg) * num_groups);
	if (grpcfg == NULL) {
		LOGERROR("grpsm(cfg_from_sqlinks): failed to allocate group cfg: %d", num_groups);
		return -1;
	}
	memset(grpcfg, 0, sizeof(struct group_cfg) * num_groups);

	for (i = 0, gcfg = grpcfg; i < num_groups; i++, gcfg++) {
		gcfg->chs = (struct group_channel *)malloc(sizeof(struct group_channel) * (sq + 1));
		if (gcfg->chs == NULL) {
			LOGERROR("grpsm(cfg_from_sqlinks): failed to allocate channels: %d, %d", i, sq + 1);
			goto cfg_from_sqlinks_error;
		}
		memset(gcfg->chs, 0, sizeof(struct group_channel) * (sq + 1));
		gcfg->num_chs = sq + 1;
	}

	cm = 1;
	for (i = 0; i <= sq; i++) {
		sqlink = &sqlinks->sqlinks[i];
		num = sqlink->num_links;
		times = num_groups / (cm * num);

		for (j = 0; j < times; j++) {
			for (k = 0; k < num; k++) {
				for (l = 0; l < cm; l++) {
					index = (j * num + k) * cm + l;
					gcfg = &grpcfg[index];
					link = sqlink->links[k];
					gcfg->chs[i].fiber = link->fiber;
					gcfg->chs[i].fiber_rate = link->fiber_rate;
					gcfg->chs[i].channel = link->channel;
					gcfg->chs[i].channel_rate = link->channel_rate;
				}
			}
		}

		cm *= num;
	}

	if (cfg)
		*cfg = grpcfg;

	return num_groups;

cfg_from_sqlinks_error:
	for (i = 0, gcfg = grpcfg; i < num_groups; i++, gcfg++) {
		if (gcfg->chs) {
			free(gcfg->chs);
			gcfg->chs = NULL;
			gcfg->num_chs = 0;
		}
	}
	free(grpcfg);

	return -1;
}

static void grpsm_exit_sqlinks(struct grpsm_sqlinks *sqlinks)
{
	struct grpsm_sqlink *sqlink;
	int i;

	for (i = 0; i < GRPSM_SQ_MAX; i++) {
		sqlink = &sqlinks->sqlinks[i];
		if (sqlink->links) {
			free(sqlink->links);
			sqlink->links = NULL;
			sqlink->num_links = 0;
		}
	}
}

static int grpsm_get_vcs_from_svc_sq(struct grpsm_svc *svc, struct group_cfg **cfg, unsigned char sq)
{
	struct grpsm_sqlinks sqlinks;
	struct grpsm_sqlink *sqlink;
	struct link_info *blink;
	struct group_channel *chs;
	unsigned int num_chs = 0;
	int i, rc;

	struct group_cfg *grpcfg = NULL, *gcfg;
	int num_groups = 0;

	sqlink = &svc->sqlinks[sq];
	if (sq == 0) {
		for (i = 0; i < sqlink->num_links; i++) {
			blink = sqlink->links[i];
			chs = (struct group_channel *)malloc(sizeof(struct group_channel));
			if (chs == NULL) {
				LOGERROR("grpsm(get_vcs_from_svc_sq): failed to allocate channels: %d, SQ 0", i);
				continue;
			}
			chs[0].fiber = blink->fiber;
			chs[0].fiber_rate = blink->fiber_rate;
			chs[0].channel = blink->channel;
			chs[0].channel_rate = blink->channel_rate;
			num_chs = 1;

			grpsm_append_cfg_channels(&grpcfg, &num_groups, chs, num_chs);
		}

		goto vcs_from_svc_sq;
	}

	/* SQ > 0 */
	for (i = 0; i < sqlink->num_links; i++) {
		blink = sqlink->links[i];

		LOGDEBUG("grpsm(get_vcs_from_svc_sq): SQ %d: blink: <%d, %d>",
				sq, blink->fiber, blink->channel);

		memset(&sqlinks, 0, sizeof(sqlinks));
		if (grpsm_init_sqlinks(&sqlinks, svc, blink, sq) == 0) {
			rc = grpsm_cfg_from_sqlinks(&sqlinks, sq, &gcfg);
			if (rc > 0) {
				grpsm_append_cfg(&grpcfg, &num_groups, gcfg, rc);
			}
			grpsm_exit_sqlinks(&sqlinks);
		}
	}

vcs_from_svc_sq:
	if (num_groups > 0) {
		if (cfg)
			*cfg = grpcfg;
	}

	return num_groups;
}

static int grpsm_get_vcs_from_svc(struct grpsm_ctl *ctl, struct grpsm_svc *svc, struct group_cfg **cfg)
{
	struct grpsm_sqlink *sqlink, *psq, *nsq;
	unsigned char sq_max = 0, sq_start, sq, found;
	int i, rc;

	if (svc->sq_status == GRPSM_SQ_STATUS_FINISHED)
		return 0;

	if (ctl->vc_partial == 0) {
		/* Find max SQ */
		for (i = 0; i < GRPSM_SQ_MAX; i++) {
			sqlink = &svc->sqlinks[i];
			if (sqlink->num_links == 0)
				break;
		}
		if (i == 0) {
			/* No link with SQ of [0] */
			svc->sq_status = GRPSM_SQ_STATUS_FINISHED;
			return 0;
		}
		else {
			sq_max = i - 1;
		}

		if (svc->sq_status == GRPSM_SQ_STATUS_GOT) {
			if (svc->sq_pre >= sq_max) {
				svc->sq_status = GRPSM_SQ_STATUS_FINISHED;
				return 0;
			}

			sq_start = svc->sq_pre + 1;
		}
		else {
			sq_start = 0;
		}

		/* Find real SQ */
		if (sq_start == sq_max) {
			sq = sq_start;
		}
		else {
			found = 0;
			for (i = sq_start; i < sq_max; i++) {
				psq = &svc->sqlinks[i];
				nsq = &svc->sqlinks[i + 1];
				if (psq->num_links > nsq->num_links) {
					sq = i;
					found = 1;
					break;
				}
			}
			if (!found) {
				sq = sq_max;
			}
		}
	}
	else {
		sq = svc->sq_pre;

		LOGDEBUG("grpsm(get_vcs_from_svc): svc[%d, %d]: sq %d, partial",
				svc->channel_rate, svc->svc_type, sq);
	}

	LOGDEBUG("grpsm(get_vcs_from_svc): svc[%d, %d]: sq %d",
			svc->channel_rate, svc->svc_type, sq);

	rc = grpsm_get_vcs_from_svc_sq(svc, cfg, sq);
	svc->sq_pre = sq;
	if (svc->sq_status == GRPSM_SQ_STATUS_DEFAULT) {
		svc->sq_status = GRPSM_SQ_STATUS_GOT;
	}

	return rc;
}

static int grpsm_get_vc(struct grpsm_ctl *ctl,
		struct group_cfg *pre_groupcfg, int num_pregroups, struct group_cfg **groupcfg)
{
	struct grpsm_fiber *fiber;
	struct grpsm_svc   *svc;
	struct group_cfg *grpcfg = NULL, *cfg = NULL;
	int i, j, num_groups = 0, rc;

	/* Filter success links */
	for (i = 0, fiber = ctl->fibers; i < ctl->num_fibers; i++, fiber++) {
		for (j = 0, svc = fiber->vcs; j < fiber->num_vcs; j++, svc++) {
			grpsm_remove_olinks_from_svc(fiber, svc, pre_groupcfg, num_pregroups);
		}
	}

	for (i = 0, fiber = ctl->fibers; i < ctl->num_fibers; i++, fiber++) {
		for (j = 0, svc = fiber->vcs; j < fiber->num_vcs; j++, svc++) {
			rc = grpsm_get_vcs_from_svc(ctl, svc, &cfg);
			if (rc > 0) {
				grpsm_append_cfg(&grpcfg, &num_groups, cfg, rc);
			}
		}
	}

	if (num_groups > 0) {
		if (groupcfg)
			*groupcfg = grpcfg;
	}

	return num_groups;
}

/* Parameters:
 *   pre_groupcfg  : Result of previous given groups;
 *   num_pregroups : Number of previous given groups;
 *
 * Return:
 *   groupcfg      : Current given groups;
 *   rc            : Number of current given groups.
 *                   -1 - internal error
 *                    0 - no more groups
 *                   >0 - number of groups
 */
int grpsm_get(void *hd, struct group_cfg *pre_groupcfg, int num_pregroups, int num_trygroups,
		struct group_cfg **groupcfg)
{
	struct grpsm_ctl *ctl = (struct grpsm_ctl *)hd;
	struct group_cfg *cfg;
	int i, rc = 0;

	if (!ctl || !groupcfg) {
		LOGERROR("grpsm(get): invalid parameters.");
		return -1;
	}

	LOGDEBUG("grpsm(get): %p, %d, %d, %p, %p", pre_groupcfg, num_pregroups, num_trygroups, groupcfg, *groupcfg);

	if (pre_groupcfg == NULL) {
		/* First get after push */
		rc = grpsm_get_lcas(ctl, groupcfg);
	}

	if (rc <= 0) {
		grpsm_update_vgroups(ctl, pre_groupcfg, num_pregroups);

		if (ctl->lcas_got == 0) {
			ctl->lcas_got = 1;
			ctl->vc_partial = 0;
			rc = grpsm_get_vc(ctl, NULL, 0, groupcfg);

			LOGDEBUG("VC(first): rc %d", rc);
		}
		else {
			ctl->vc_partial = (num_trygroups < num_pregroups) ? 1 : 0;
			rc = grpsm_get_vc(ctl, pre_groupcfg, num_trygroups, groupcfg);

			LOGDEBUG("VC(more): rc %d", rc);
		}

		/* release previous group configuration */
		for (i = 0, cfg = pre_groupcfg; i < num_pregroups; i++, cfg++) {
			grpsm_exit_cfg(cfg);
		}
		free(pre_groupcfg);
	}

	return rc;
}

int grpsm_stat(void *hd, struct group_stat **groupstat)
{
	struct grpsm_ctl  *ctl = (struct grpsm_ctl *)hd;
	struct group_stat *grpstat;
	struct link_info  *link;
	int i, index = 0;

	if (!ctl || !groupstat) {
		LOGERROR("grpsm(stat): invalid parameters.");
		return -1;
	}

	grpstat = (struct group_stat *)malloc(sizeof(struct group_stat) * ctl->num_links);
	if (grpstat == NULL) {
		LOGERROR("grpsm(stat): failed to allocate group stats.");
		return -1;
	}
	memset(grpstat, 0, sizeof(struct group_stat) * ctl->num_links);

	for (i = 0, link = ctl->links; i < ctl->num_links; i++, link++) {
		memcpy(&(grpstat[i].link), link, sizeof(*link));
		if (is_link_in_group(link, ctl->vgroups, ctl->num_vgroups, &index)) {
			grpstat[i].grouped = 1;
			grpstat[i].groupid = index + 1;
		}
	}

	*groupstat = grpstat;

	return ctl->num_links;
}

void grpsm_close(void *hd)
{
	struct grpsm_ctl *ctl = (struct grpsm_ctl *)hd;

	if (ctl) {
		grpsm_exit(ctl);
		free(ctl);
	}
}

