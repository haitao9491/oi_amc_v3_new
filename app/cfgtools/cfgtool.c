/*
 * (C) Copyright 2016
 * zhangqizhi <qizhi.zhang@raycores.com>
 *
 * cfgtool.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <fcntl.h>
#include "cfgtool.h"
#include "cconfig.h"
#include "apfrm.h"
#include "os.h"
#include "aplog.h"

#define CFGTOOL_ERR (-1)
#define CFGTOOL_OK  (0)
#define CFGTOOL_EXIST (0x10)
#define CFGTOOL_DEF_INDEX (0xffffffff)
#define CFGTOOL_VALUE_BUF_SIZE (128)

struct stData {
	char *progname;
};

struct stData gdata;

char value_buf[CFGTOOL_VALUE_BUF_SIZE] = { 0 };

char *gpath = NULL;
char *gsection = NULL;
char *gkey = NULL;
char *gvalue = NULL;
unsigned int gop_flag = 0;
unsigned int gcheck_flag = 0;
char gaction = 0;
int gindex = 0;
#ifdef OIPC_V1
char *lockfile = "/application/etc/web.cfg.lock";
#elif MACB_V3 
char *lockfile = "/myself/etc/cpld.cfg.lock";
#else
char *lockfile = NULL;
#endif
int lfd = 0;
int lockfile_unexist = 0;

static int Cfglock(void)
{
    if (lockfile_unexist == 1)
        return 0;

    lfd = open(lockfile, O_RDWR);
    if (lfd == -1) {
        LOGERROR("Cfglock: open %s failed.", lockfile);
        return CFGTOOL_ERR;
    }

    if (flock(lfd, LOCK_EX) != 0) {
        LOGERROR("Cfglock: %s lock failed.", lockfile);
        close(lfd);
        return CFGTOOL_ERR;
    }

   return 0;
}

static int Cfgunlock(void)
{
    if (lockfile_unexist == 1)
        return 0;

    if (flock(lfd, LOCK_UN) != 0)
        LOGERROR("Cfgunlock: %s unlock failed.", lockfile);

    close(lfd);

    return 0;
}

int CfgtoolShow(const char *path)
{
	int ret = 0;
	char buf[128] = { 0 };

	sprintf(buf, "cat %s", path);
	ret = system(buf);

	return ret;
}

int CfgtoolAddSection(unsigned long h, const char *section)
{
	unsigned int section_count = 0;
	
	if (0 == h || NULL == section) {
		return CFGTOOL_ERR;
	}

	section_count = CfgGetCount(h, section, NULL, 0);
	if (0 != section_count) {
		return CFGTOOL_EXIST;
	}
	if (CfgAddToken(h, section, -1, NULL, NULL, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolDelSection(unsigned long h, const char *section)
{
	if (0 == h || NULL == section) {
		return CFGTOOL_ERR;
	}

	if (CfgDelSection(h, section, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolGetSection(unsigned long h, const char *section, char *buf, unsigned buflen)
{
	if (0 == h || NULL == section || NULL == buf) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolAddKey(unsigned long h, const char *section, const char *key, const char *val)
{

	int index = 0;

	if (0 == h || NULL == section || NULL == key || NULL == val) {
		return CFGTOOL_ERR;
	}

	index = CfgGetIndex(h, section, key, 0, 1);
	while (-1 != index) {
		memset(value_buf, 0, CFGTOOL_VALUE_BUF_SIZE);
		if (CfgGetValue(h, section, key, value_buf, index, 1) != 0) {
			break;
		}
		if (strcmp(value_buf, val) == 0) {
			return CFGTOOL_OK;
		}
		index = CfgGetIndex(h, section, key, index, 1);
	}
	if (CfgAddToken(h, section, -1, key, val, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolDelKey(unsigned long h, const char *section, const char *key, int index)
{
	int key_index = 0;

	if (0 == h || NULL == section || NULL == key) {
		return CFGTOOL_ERR;
	}

	(CFGTOOL_DEF_INDEX == index) ? (key_index = 1):(key_index = index);
	if (CfgDelToken(h, section, key, key_index, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolSetKeyVal(unsigned long h, const char *section, const char *key, const char *val, int index)
{
	int key_index = 0;

	if (0 == h || NULL == section || NULL == key || NULL == val) {
		return CFGTOOL_ERR;
	}

	(CFGTOOL_DEF_INDEX == index) ? (key_index = 1):(key_index = index);
	if (CfgSetValue(h, section, key, val, key_index , 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolGetKeyVal(unsigned long h, const char *section, const char *key, char *val, int index)
{
	int key_index = 0;

	if (0 == h || NULL == section || NULL == key || NULL == val) {
		return CFGTOOL_ERR;
	}

	(CFGTOOL_DEF_INDEX == index) ? (key_index = 1):(key_index = index);
	if (CfgGetValue(h, section, key, val, key_index, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolAddVector(unsigned long h, const char *section, const char *value)
{
	if (0 == h || NULL == section || NULL == value) {
		return CFGTOOL_ERR;
	}

	CfgtoolAddSection(h, section);
	if (CfgAddToken(h, section, -1, NULL, value, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolDelVector(unsigned long h, const char *section, int index)
{
	if (0 == h || NULL == section) {
		return CFGTOOL_ERR;
	}

	if (CfgDelToken(h, section, NULL, index, 1) != 0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

int CfgtoolGetVectorCount(unsigned long h, const char *section, int *count)
{
	int ct = 0;

	if (0 == h || NULL == section || NULL == count) {
		return CFGTOOL_ERR;
	}

	ct = CfgGetCount(h, section, NULL, 1);
	*count = ct;

	return CFGTOOL_OK;
}

int CfgtoolGetVectorByIndex(unsigned long h, const char *section, char *buf, int index)
{
	if (0 == h || NULL == section || NULL == buf) {
		return CFGTOOL_ERR;
	}

	if (CfgGetValue(h, section, NULL, buf, index, 1) !=0) {
		return CFGTOOL_ERR;
	}

	return CFGTOOL_OK;
}

void CfgtoolShowUsage(char *name)
{
	printf("        --file <file>: config file\n");
	printf("        --lockfile <file>: the file for lock\n");
	printf("        --action|-A <add|del|set|get>:\n");
	printf("        --section|-S <section>:\n");
	printf("        --key|-K <key>:\n");
	printf("        --value <value>:\n");
	printf("        --vector:\n");
	printf("        --index|-I: the index of element in vector\n");
	printf("        --create|-C: if file not exist, create it\n");
}

void CfgtoolShowUsageLocal(char *name)
{
	printf("Usage: %s [application specific options]", name);
	CfgtoolShowUsage(name);
}

void CfgtoolShowVersion(char *name)
{

}

int CfgtoolArgsCheckWhenRun(void)
{
	struct stat buf = { 0 };

	stat(gpath, &buf);
	
	if (S_ISDIR(buf.st_mode)) {
        LOGERROR("Failed %s is dir.", gpath);
		return -1;
	}
	if ((gcheck_flag & 0x1) == 0x0) {
		/*check file exist or not, if not exist return directly*/
		if (!(S_ISREG(buf.st_mode))) {
            LOGERROR("Failed %s is not exist.", gpath);
			return -1;
		}
	}

	return 0;
}

int CfgtoolArgsCheck(void)
{
	if ((gop_flag & 0x01) != 0x01) {
        LOGERROR("CfgtoolArgsCheck err.");
		return -1;
	}
	
	return 0;
}

int CfgtoolParseArgs(int argc, char **argv)
{
	int i = 0;
    int rc = 0;

    LOG("CfgtoolParseArgs.....");
	gop_flag = 0;
	gaction = 0;
	for (i = 0;i < argc;i++) {
		if (strcmp(argv[i], "--file") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--file: %s err.", argv[i + 1]);
				return -1;
			}
			i++;
			gpath = argv[i];
			gop_flag |= (1 << 0);
		}
		else if (strcmp(argv[i], "--lockfile") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--lockfile: %s err.", argv[i + 1]);
				return -1;
            }
            i++;
            lockfile = argv[i];
			gop_flag |= (1 << 6);
        }
		else if ((strcmp(argv[i], "--action") == 0) || (strcmp(argv[i], "-A") == 0)) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--action: %s err.", argv[i + 1]);
				return -1;
			}
			i++;
			if (strcmp(argv[i], "add") == 0) {
				gaction = 0x1;
			}
			else if (strcmp(argv[i], "del") == 0) {
				gaction = 0x2;
			}
			else if (strcmp(argv[i], "get") == 0) {
				gaction = 0x4;
			}
			else if (strcmp(argv[i], "set") == 0) {
				gaction = 0x8;
			}
			else {
				gaction = 0;
				return -1;
			}
		}
		else if ((strcmp(argv[i], "-S") == 0) || (strcmp(argv[i], "--section") == 0)) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--section: %s err.", argv[i + 1]);
				return -1;
			}
			i++;
			gsection = argv[i];
			gop_flag |= (1 << 1);
		}
		else if ((strcmp(argv[i], "-K") == 0) || (strcmp(argv[i], "--key") == 0)) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--key: %s err.", argv[i + 1]);
				return -1;
			}
			i++;
			gkey = argv[i];
			gop_flag |= (1 << 2);
		}
		else if (strcmp(argv[i], "--value") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--value: %s err.", argv[i + 1]);
				return -1;
			}
			i++;
			gvalue = argv[i];
			gop_flag |= (1 << 3);
		}
		else if (strcmp(argv[i], "--vector") == 0) {
			gop_flag |= (1 << 4);
		}
		else if ((strcmp(argv[i], "--index") == 0) || (strcmp(argv[i], "-I") == 0)) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-')) {
                LOGERROR("--index: %s err.", argv[i + 1]);
				return -1;
			}
			i++;
			gvalue = argv[i];
			gindex = atoi(gvalue);
			gop_flag |= (1 << 5);
		}
		else if ((strcmp(argv[i], "--create") == 0) || (strcmp(argv[i], "-C") == 0)) {
			gcheck_flag |= (1 << 0);
		}
	}

	rc = CfgtoolArgsCheck();
    if (rc != 0)
        LOGERROR("CfgtoolArgsCheck err.");

    return rc;
}

int cfgtool_env_init()
{
	struct stat buf = { 0 };

	stat(lockfile, &buf);
	
	if (S_ISDIR(buf.st_mode)) {
        LOGERROR("Failed %s is dir.", lockfile);
		return -1;
	}
    /*check file exist or not, if not exist return directly*/
    if (!(S_ISREG(buf.st_mode))) {
        if (gop_flag & 0x40) {
            LOGERROR("Failed %s is not exist.", lockfile);
            return -1;
        } else {
            lockfile_unexist = 1;
            return 0;
        }
    }

    return 0;
}

/*
*gop_flag: bits
* 0      1         2    3     4		5      6
* |      |         |    |     |		|      |
*file section add key value vector index  lockfile
*
*gcheck_flag: bits
*  0
*  |
*create
*
*gaction: bits
*add - 0x1
*del - 0x2
*get - 0x4
*set - 0x8
*/

int CfgtoolRun(long instance, unsigned long data)
{
	unsigned long h = 0;
	int ret = -1;
	char tmpbuf[128] = { 0 };
    unsigned int pflag = 0;

    if (cfgtool_env_init() != 0)
        return -1;

    if (Cfglock() != 0)
        return -1;

	if (CfgtoolArgsCheckWhenRun() != 0) {
        Cfgunlock();
		return -1;
	}

	h = CfgInitialize(gpath);
	if (0 == h) {
        LOGERROR("Failed: CfgInitialize.");
        Cfgunlock();
		return -1;
	}

    pflag = gop_flag & 0x3f;
	switch (gaction) {
		case 0x1:/*add*/
			if ((pflag & 0x3f) == 0xf) {
				ret = CfgtoolAddKey(h, gsection, gkey, gvalue);
			}
			else if (pflag == 0x3) {
				ret = CfgtoolAddSection(h, gsection);
			}
			else if (pflag == 0x1b) {
				ret = CfgtoolAddVector(h, gsection, gvalue);
			}
			break;
		case 0x2:/*del*/
			if (pflag == 0x7) {
				ret = CfgtoolDelKey(h, gsection, gkey, CFGTOOL_DEF_INDEX);
			}
			else if (pflag == 0x3) {
				ret = CfgtoolDelSection(h, gsection);
			}
			else if (pflag == 0x33) {
				ret = CfgtoolDelVector(h, gsection, gindex);
			}
			break;
		case 0x4:/*get*/
			if (pflag == 0x7) {
				memset(tmpbuf, 0, 128);
				if (CfgtoolGetKeyVal(h, gsection, gkey, tmpbuf, CFGTOOL_DEF_INDEX) != 0) {
					printf("NULL");
                    LOGERROR("CfgtoolGetKeyVal err.");
					ret = -1;
				}
				else {
					printf("%s", tmpbuf);
					ret = 0;
				}
			}
			else if (pflag == 0x3) {
                LOGERROR("pflag==0x03 err.");
				ret = -1;
			}
			else if (pflag == 0x13) {
				gindex = 0;
				CfgtoolGetVectorCount(h, gsection, &gindex);
				printf("%d", gindex);
				ret = 0;
			}
			else if (pflag == 0x33) {
				memset(tmpbuf, 0, 128);
				if (CfgtoolGetVectorByIndex(h, gsection, tmpbuf, gindex) != CFGTOOL_OK) {
					printf("NULL");
                    LOGERROR("CfgtoolGetVectorByIndex err.");
					ret = -1;
				}
				else {
					printf("%s", tmpbuf);
					ret = 0;
				}
			} else {
                LOGERROR("--get: unkown pflag 0x%x.", pflag);
            }

			break;
		case 0x8:/*set*/
			if (pflag == 0xf) {
				ret = CfgtoolSetKeyVal(h, gsection, gkey, gvalue, CFGTOOL_DEF_INDEX);
                if (ret != 0)
                    LOGERROR("CfgtoolSetKeyVal err.");
			}
			break;
		default:
			ret = -1;
	}

	CfgInvalidate(h);

    Cfgunlock();
	return ret;
}

static struct ap_framework cfgtool_app = {
	NULL,
	CfgtoolRun,
	0,
	NULL,
	NULL,
	NULL,
	CfgtoolShowUsage,
    NULL,
	CfgtoolParseArgs
};

#if defined(__cplusplus)
extern "C" {
#endif

struct ap_framework *register_ap(void)
{
	ap_set_foreground();
	return &cfgtool_app;
}

#if defined(__cplusplus)
}
#endif


