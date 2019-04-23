/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * reg.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include "reg.h"


#define REG_READ            0
#define REG_WRITE           1

#define FPGA_NUM_MAX        4
#define CPLD_NUM_MAX        1
#define E1PHY_NUM_MAX       16
#define ONE_E1PHY_SIZE      0x2000

#define REG_ACCESS          1 
#define REG_NO_ACCESS       2

typedef struct device_info {
    int access;
    int num;
    unsigned int phys;
    unsigned int size;
    int readcmd;
    int writecmd;
} device_info_t ;

device_info_t fpgas[FPGA_NUM_MAX];
device_info_t cplds[CPLD_NUM_MAX];
device_info_t e1phys[E1PHY_NUM_MAX];

void usage(char *program)
{
	printf("Usage: %s [[r addr [num]] | [w addr val]]\n", basename(program));
	printf("    val: 0x00 - 0xff\n");
}
#if defined(EIPB_V2) || defined(MUCB_V1) \
    || defined(OI_AMC_V3) || defined(EI_AMC_V1) || defined(OIPC_V1) \
	|| defined(EIPC_V1) || defined(LBB_V1) || defined(OI_AMC_V4)  \
    || defined(EIPC_V2) || defined(XSCB_V2) || defined(OTAP_V1) \
    || defined(ETAP_V1) || defined(XPPP_V2) || defined(LCEA_V1)

static int get_fpga_info(int fd)
{
    struct fpga_info_entry *fpga_entries = NULL;
    int num, i;

    num = 0;
    if (ioctl(fd, DEV_GET_FPGA_NUM, &num) != 0) {
        printf("failed to get fpga number.\n");
        return -1;
    }

    if (num == 0) {
        printf("No FPGA defined.\n");
        return 0;
    }

    fpga_entries = (struct fpga_info_entry *)malloc(num * sizeof(struct fpga_info_entry));
    if (fpga_entries == NULL) {
        printf("failed to allocate %d entries.\n", num);
        return -1;
    }

    memset(fpga_entries , 0, num * sizeof(struct fpga_info_entry));
    if (ioctl(fd, DEV_GET_FPGA_INFO, fpga_entries ) != 0) {
        printf("failed to get fpga info.\n");
        free(fpga_entries);
        return -1;
    }

    for (i = 0; i < num; i++) {
        fpgas[i].access = REG_ACCESS;
        fpgas[i].num = num;
        fpgas[i].phys = fpga_entries[i].phys;
        fpgas[i].size = fpga_entries[i].size;
        fpgas[i].readcmd = DEV_GET_FPGA_REG;
        fpgas[i].writecmd = DEV_SET_FPGA_REG;
    }

    free(fpga_entries);
    return 0;
}
#endif

#if defined(MACB_V3)
static int get_cpld_info(int fd)
{
    struct cpld_info_entry cpld_entries;

    memset(&cpld_entries, 0, sizeof(struct cpld_info_entry));
	if (ioctl(fd, DEV_GET_CPLD_INFO, &cpld_entries) != 0) {
        printf("failed to get cpld info.\n");
        return -1;
    }

    cplds[0].access = REG_ACCESS;
    cplds[0].num = 1;
    cplds[0].phys = cpld_entries.phys;
    cplds[0].size = cpld_entries.size;
    cplds[0].readcmd = DEV_GET_CPLD_REG;
    cplds[0].writecmd = DEV_SET_CPLD_REG;

    return 0;
}
#endif

#if defined(EI_AMC_V1) || defined(LCEA_V1)
static int get_e1phy_info(int fd)
{
    struct e1phy_info_entry e1phy_entries;
    int i, num;

    memset(&e1phy_entries, 0, sizeof(struct e1phy_info_entry));
	if (ioctl(fd, DEV_GET_E1PHY_INFO, &e1phy_entries) != 0) {
        printf("failed to get e1phy info.\n");
        return -1;
    }

    num = (e1phy_entries.size / ONE_E1PHY_SIZE) <= E1PHY_NUM_MAX ? (e1phy_entries.size / ONE_E1PHY_SIZE) : E1PHY_NUM_MAX;
    for (i = 0; i < num; i++) {
        e1phys[i].access = REG_ACCESS;
        e1phys[i].num = num;
        e1phys[i].readcmd = 0;
        e1phys[i].writecmd = 0;
        e1phys[i].phys = e1phy_entries.phys + (i * ONE_E1PHY_SIZE);
        e1phys[i].size = ONE_E1PHY_SIZE;
    }

    return 0;
}
#endif

static int reg_init(int fd)
{
    memset(&fpgas, 0, sizeof(device_info_t) * FPGA_NUM_MAX);
    memset(&cplds, 0, sizeof(device_info_t) * CPLD_NUM_MAX);
    memset(&e1phys, 0, sizeof(device_info_t) * E1PHY_NUM_MAX);
   
#if defined(EIPB_V2) || defined(MUCB_V1) \
    || defined(OI_AMC_V3) || defined(EI_AMC_V1) || defined(OIPC_V1) \
	|| defined(EIPC_V1) || defined(LBB_V1) || defined(OI_AMC_V4)  \
    || defined(EIPC_V2) || defined(XSCB_V2) || defined(OTAP_V1) \
    || defined(ETAP_V1) || defined(XPPP_V2) || defined(LCEA_V1)
    get_fpga_info(fd);
#endif

#if defined(MACB_V3)
    get_cpld_info(fd);
#endif

#if defined(EI_AMC_V1) || defined(LCEA_V1)
    get_e1phy_info(fd);
#endif

    return 0;
}

static void reg_prt_info(void)
{
    int i;

    for (i = 0; i < FPGA_NUM_MAX; i++) {
        if (fpgas[i].access == REG_ACCESS)
            printf("FPGA[%d]: 0x%08x 0x%08x\n", i, fpgas[i].phys, fpgas[i].size);
    }

    for (i = 0; i < CPLD_NUM_MAX; i++) {
        if (cplds[i].access == REG_ACCESS)
            printf("CPLD[%d]: 0x%08x 0x%08x\n", i, cplds[i].phys, cplds[i].size);
    }

    for (i = 0; i < E1PHY_NUM_MAX; i++) {
        if (e1phys[i].access == REG_ACCESS)
            printf("E1PHY[%d]: 0x%08x 0x%08x\n", i, e1phys[i].phys, e1phys[i].size);
    }
}

static int do_reg(int fd, unsigned int addr, int rw, int value)
{
#if defined(EIPB_V2) || defined(MUCB_V1) \
    || defined(OI_AMC_V3) || defined(EI_AMC_V1) || defined(OIPC_V1) \
	|| defined(EIPC_V1) || defined(LBB_V1) || defined(OI_AMC_V4)  \
    || defined(EIPC_V2) || defined(XSCB_V2) || defined(OTAP_V1) \
    || defined(ETAP_V1) || defined(XPPP_V2) || defined(LCEA_V1)
    struct fpga_reg reg;
#endif
#if defined(MACB_V3)
    struct cpld_reg reg;
#endif

    int i;
    int cmd = 0;

    if (!addr)
        return -1;


    for (i = 0; i < FPGA_NUM_MAX; i++) {

        if (fpgas[i].access != REG_ACCESS)
            continue;

//        if ((fpgas[i].phys <= addr) && (addr < (fpgas[i].phys + fpgas[i].size))) {

            if (rw == REG_READ)
                cmd = fpgas[i].readcmd;
            else
                cmd = fpgas[i].writecmd;

            goto ioctl_reg;
//          }
    }

    for (i = 0; i < CPLD_NUM_MAX; i++) {

        if (cplds[i].access != REG_ACCESS)
            continue;

        if ((cplds[i].phys <= addr) && (addr < (cplds[i].phys + cplds[i].size))) {

            if (rw == REG_READ)
                cmd = cplds[i].readcmd;
            else
                cmd = cplds[i].writecmd;

            goto ioctl_reg;
        }
    }


ioctl_reg:

    if (rw == REG_READ) {
        for (i = 0; i < value; i++) {
            memset(&reg, 0, sizeof(reg));
            reg.addr = addr + i;

            if (ioctl(fd, cmd, &reg) == 0) {
                printf("READ: [%08x]=0x%08x\n", reg.addr, reg.value);
            } else {
                printf("READ: [%08x] error!\n", reg.addr);
            }
        }

	} else {
        memset(&reg, 0, sizeof(&reg));
		reg.addr = addr;
		reg.value = value;

		if (ioctl(fd, cmd, &reg) == 0) {
			printf("WRITE: [%08x]=0x%08x ok\n", addr, value);
		} else {
			printf("WRITE: [%08x]=0x%08x error!\n", addr, value);
		}
	}

    return 0;
}


int main(int argc, char *argv[])
{
	int fd;
	unsigned int addr;
	unsigned int num = 0;
	unsigned int val = 0;
	char fname[32];

	memset(fname, 0, sizeof(fname));
	sprintf(fname, "/dev/%s", DEV_NAME);

	fd = open(fname, O_RDWR);
	if (fd < 0) {
		printf("open %s device err!\n", fname);
		return -1;
	}

    reg_init(fd);
	if (argc == 1) {
		reg_prt_info();
        close(fd);
        return 0;
	}

	if (strcmp(argv[1], "r") == 0) {
		if (argc < 3) {
			usage(argv[0]);
            goto err_exit;
		}
		addr = strtoul(argv[2], NULL, 0);
		num = 1;

		if (argc > 3) {
			num = strtoul(argv[3], NULL, 0);
		}

       do_reg(fd, addr, REG_READ, num);

	} else if (strcmp(argv[1], "w") == 0) {
		if (argc < 4) {
			usage(argv[0]);
            goto err_exit;
		}
		addr = strtoul(argv[2], NULL, 0);
		val = strtoul(argv[3], NULL, 0);
		if (val > 0xff) {
			usage(argv[0]);
            goto err_exit;
		}

        do_reg(fd, addr, REG_WRITE, val);
	} else {
		usage(argv[0]);
        goto err_exit;
	}

	close(fd);
	return 0;

err_exit:
    close(fd);
    return -1;
}

