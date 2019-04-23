/*
 * (C) Copyright 2019
 * liye <ye.li@raycores.com>
 *
 * gfpapp.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "aplog.h"
#include "cconfig.h"
#include "gfplib.h"

void *__lgwr__handle = NULL;
void *gfphd = NULL;
uint8_t fpgaid = 0;

void usage(void)
{
    printf("gfpapp show <fpga0|fpga1|fpga2|fpga3>: \n");
    printf("                                    au4-status <1-32>       : show au4 status.\n");
    printf("                                    linkinfo <1-32>         : show link info of au4.\n");
    printf("                                    linkinfo <1-32> <1-63>  : show link info of channel.\n");
    printf("                                    srcmap <1-32> <1-63>    : show trace to the source.\n");
    printf("gfpapp cfg <fpga0|fpga1|fpga2|fpga3>: \n");
    printf("                                    try [cfgfile]           : cfg trial groups.\n");
    printf("                                    set [cfgfile]           : cfg set groups of gfp result.\n");
    return;
}

int gfpapp_show_au4_status(int au4)
{
    struct gfp_au4status status;

    memset(&status, 0, sizeof(&status));

    if (gfplib_get_au4status(gfphd, au4, &status) != 0) {
        printf("gfplib_get_au4status failed \n");
        return -1;
    }

    printf("au4 s-chassisid s-slot s-port s-au4 los lof \n");
    printf("%-2d  %-2d          %-2d     %-2d     %-2d    %-2d  %-2d\n", \
           au4, status.chassisid, status.slot, status.port, status.au4, status.los, status.lof);

    return 0;
}

int gfpapp_show_linkinfo(int au4, int e1)
{
    struct gfp_linkinfo info;
    int rv = 0;


    memset(&info, 0, sizeof(&info));
        
    if (gfplib_get_linkinfo(gfphd, au4, e1, &info) != 0) {
        rv = -1;
    } else {

        printf("%-2d  %-2d %-2d          %-2d     %-2d     %-2d    %-2d   %-2d     %-2d   %-2d    %-2d      %-2d        %-2d             %-5d %-2d %-10d %-10d\n", au4, e1, info.chassisid, info.slot, info.port, info.au4, info.e1, info.e1_rate, \
               info.svc_type, info.vc_valid, info.is_lcas, info.is_member, info.is_last_member,\
               info.mfi, info.sq, info.pre_gid, info.cur_gid);
    }

    return rv;
}

int gfpapp_show_srcmap(int au4, int e1)
{
    struct gfp_linkinfo map;

    memset(&map, 0, sizeof(&map));
    if (gfplib_channel_local2global(gfphd, au4, e1, &map) != 0) {
        return -1;
    }

    printf("au4 e1 s-chassisid s-slot s-port s-au4 s-e1 \n");
    printf("%-2d  %-2d %-2d          %-2d     %-2d      %-2d    %-2d\n", \
          au4, e1, map.chassisid, map.slot, map.port, map.au4, map.e1);

    return 0;
}

int gfpapp_show(int argc, char *argv[])
{
    int rv = 0;
    int i = 0;

    if (!strcmp(argv[0], "au4-status")) {
        printf("argv[0] %s argv[1] %s\n", argv[0], argv[1]);
        if (argc != 2) {
            printf("argc %d err. argv[1]: %s \n", argc, argv[1]);
            rv = -1;
        }
        if (gfpapp_show_au4_status(atoi(argv[1])) != 0) {
            rv = -1;
        }

    } else if (!strcmp(argv[0], "linkinfo")) {
        if (argc < 2) {
            printf("argc %d err. \n", argc);
            rv = -1;
        }

        printf ("au4 e1 s-chassisid s-slot s-port s-au4 s-e1 e1rate type valid is-lcas is-member is-last-member mfi   sq pre-gid    cur-gid\n");

        gfplib_get_linkinfo_begin(gfphd);
        if (argc == 3) {

            if (gfpapp_show_linkinfo(atoi(argv[1]), atoi(argv[2])) != 0)
                rv = -1;

        } else {
            for (i = 1; i <= 63; i++) {

                if (gfpapp_show_linkinfo(atoi(argv[1]), i) != 0)
                    rv = -1;

            }
        }
        gfplib_get_linkinfo_end(gfphd);

    } else if (!strcmp(argv[0], "srcmap")) {
        if (argc < 2) {
            printf("argc %d err. argv[1] : %s \n", argc, argv[1]);
            rv = -1;
        }

        if (argc == 3) {
            if (gfpapp_show_srcmap(atoi(argv[1]), atoi(argv[2])) != 0) {
                rv = -1;
            }

        } else {
            for (i = 1; i <= 63; i++) {
                if (gfpapp_show_srcmap(atoi(argv[1]), i) != 0) {
                    rv = -1;
                }
            }

        }

    } else {
        printf("Unkown argv %s \n", argv[0]);
        rv = -1;
    }

    return rv;
}

int get_from_cfgfile(char *cfgfile, struct gfp_groups *groups)
{
	unsigned long cfghd = 0;
    char *token = NULL;
    char value[128];
    int i, j;
    
    cfghd = CfgInitialize(cfgfile);
    if (cfghd == 0ul) {
        printf("open cfghd error. \n");
        return -1;
    }
    groups->groups_num = CfgGetCount(cfghd, (char *)"Group", NULL, 0);

    for (i = 0; i < CfgGetCount(cfghd, (char *)"Group", NULL, 0); i++) {
        groups->groups[i].id = i;
        groups->groups[i].links_num = CfgGetCount(cfghd, (char *)"Group", (char *)"slink", (i + 1));
        printf("groups[%d] links num %d \n", (i+1), groups->groups[i].links_num);

        for (j = 0; j < groups->groups[i].links_num; j++) {
            if (CfgGetValue(cfghd, (char *)"Group", (char *)"slink", value, (j+1), (i+1)) == -1) {
                printf("CfgGetValue section[%d] link[%d] error.\n", (i+1), (j+1));
                continue;
            }

            memset(&groups->groups[i].links[j], 0, sizeof(struct gfp_link));
            token = strtok(value, ",");
            if (token != NULL) {
                groups->groups[i].links[j].au4 = atoi(token);
            }

            token = strtok(NULL, ",");
            if (token != NULL) {
                groups->groups[i].links[j].e1 = atoi(token);
            }
            printf("group[%d] %d-%d \n", (i+1), groups->groups[i].links[j].au4, groups->groups[i].links[j].e1);
        }
    }
    CfgInvalidate(cfghd);

    return 0;
}

int gfpapp_cfg_trial_groups(char *cfgfile)
{
    int i = 0;
    int size = 0;
    struct gfp_groups *trial_groups = NULL;
    struct gfp_trial_results *results = NULL;
    int cnt = 0;

    size = sizeof(struct gfp_groups);
    printf("gfp_groups size %d gfp_group size %d gfp_link size %d\n", \
          sizeof(struct gfp_groups), sizeof(struct gfp_group), sizeof(struct gfp_link));

    trial_groups = (struct gfp_groups *)malloc(size);
    if (trial_groups == NULL) {
        printf("malloc failed. \n");
        return -1;
    }
    memset(trial_groups, 0, size);

    results = (struct gfp_trial_results *)malloc(sizeof(struct gfp_trial_results));
    if (results == NULL) {
        printf("malloc failed. \n");
        return -1;
    }
    memset(results, 0, sizeof(struct gfp_trial_results));

    if (get_from_cfgfile(cfgfile, trial_groups) != 0) {
        free(trial_groups);
        return -1;
    }

    printf("cfg trial groups cfgfile %s \n", cfgfile);
    
    printf("Groups num %d \n", trial_groups->groups_num);

    if (gfplib_set_trial_groups(gfphd, trial_groups, results) != 0) {
        free(trial_groups);
        free(results);
        return -1;
    }

    printf("groupid count-id success error \n");
    for (i = 0; i < trial_groups->groups_num; i++) {
        printf("%-2d     %-2d        %-2d       %2d   \n",\
               (results->results[i].id), cnt, results->results[i].succs_pkts, results->results[i].error_pkts);
    }

    free(trial_groups);
    free(results);
    return 0;
}

int gfpapp_cfg_set_group(char *cfgfile)
{
    int i, j;
    int size = 0;
    struct gfp_groups *result = NULL;

    size = sizeof(struct gfp_groups);
    printf("gfp_groups size %d gfp_group size %d gfp_link size %d\n", \
          sizeof(struct gfp_groups), sizeof(struct gfp_group), sizeof(struct gfp_link));

    result = (struct gfp_groups *)malloc(size);
    memset(result, 0, size);

    if (get_from_cfgfile(cfgfile, result) != 0) {
        free(result);
        return -1;
    }

    printf("cfg trial groups cfgfile %s \n", cfgfile);
    
    printf("Groups num %d \n", result->groups_num);

    if (gfplib_set_groups(gfphd, result) < 0) {
        free(result);
        return -1;
    }

    printf("groupid linkid au4 e1\n");
    for (i = 0; i < result->groups_num; i++) {
        for (j = 0; j < result->groups[i].links_num; j++) {
            printf("%-2d     %-2d        %-2d       %2d   \n",\
                   (i+1), (j+1), result->groups[i].links[j].au4, result->groups[i].links[j].e1);
        }
    }
    printf("set groups complete !\n");

    free(result);
    return 0;
}

int gfpapp_cfg(int argc, char *argv[])
{
    char *cfgfile = NULL;

    if (!strcmp(argv[0], "try")) {
        if (argc != 2) {
            printf("gfpapp try . argc %d err.\n", argc); 
            return -1;
        }

        cfgfile = argv[1];
        printf("gfpapp_cfg try argv[0] %s argv[1] %s %s\n", argv[0], argv[1], cfgfile);
        if (gfpapp_cfg_trial_groups(cfgfile) != 0) {
            printf("failed to gfpapp_cfg_trial_groups\n");
            return -1;
        }

    } else if (!strcmp(argv[0], "set")) {
        if (argc != 2) {
            printf("gfpapp set. argc %d err.\n", argc); 
            return -1;
        }

        cfgfile = argv[1];
        printf("gfpapp_cfg set argv[0] %s argv[1] %s %s\n", argv[0], argv[1], cfgfile);
        if (gfpapp_cfg_set_group(cfgfile) != 0) {
            printf("failed to gfpapp_cfg_set_groups\n");
            return -1;
        }

    } else {
        return -1;
    }

    return 0;
}

int gfpapp_prase_argv(int argc, char *argv[])
{
    int flag = 0;

    if (!strcmp(argv[0], "show")) {
        flag = 1;
    } else if (!strcmp(argv[0], "cfg")) {
        flag = 2;
    } else {
        printf("Unkown argv %s \n", argv[0]);
        return -1;
    }

    if (!strcmp(argv[1], "fpga0")) {
        fpgaid = 1;
        gfphd = gfplib_open(1);
    } else if (!strcmp(argv[1], "fpga1")) {
        fpgaid = 2;
        gfphd = gfplib_open(2);
    } else if (!strcmp(argv[1], "fpga2")) {
        fpgaid = 3;
        gfphd = gfplib_open(3);
    } else if (!strcmp(argv[1], "fpga3")) {
        fpgaid = 4;
        gfphd = gfplib_open(4);
    } else {
        printf("Unkown fpga %s \n", argv[1]);
        return -1;
    }

    if (gfphd == NULL) {
        printf("%s gfp open failed.\n", argv[1]);
        return -1;
    }

    if (flag == 1) {
        if (gfpapp_show((argc - 2), &argv[2]) != 0) {
            return -1;
        }
    } else {
        if (gfpapp_cfg((argc - 2), &argv[2]) != 0) {
            return -1;
        }
    }

    return 0;
}

void gfpapp_exit(void)
{
    gfplib_close(gfphd);
}


int main(int argc, char *argv[])
{
    int i = 0;

    for (i = 0; i < argc; i++) {
        printf("argv[%d]: %s \n", i, argv[i]);
    }

    if (gfpapp_prase_argv((argc - 1), (&argv[1])) != 0) {
        usage();
        gfpapp_exit();
        return -1;
    }

    gfpapp_exit();
    return 0;
}
