/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * psagent.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "apfrm.h"
#include "os.h"
#include "aplog.h"
#include "cconfig.h"
#include "coding.h"
#include "list.h"
#include "adapter.h"
#include "adapter_cs.h"
#include "mutex.h"
#include "psp.h"
#include "psagent.h"

#define PSAGENTVERSION "2.0"

#define PSAGENT_OPS_REGISTER                 register_msg 
#define PSAGENT_OPS_REGISTER_ACK             register_ack_msg 
#define PSAGENT_OPS_SCAN_CLEAR               scan_clear
#define PSAGENT_OPS_NOTIFY_ANM               notify_anm
#define PSAGENT_OPS_NOTIFY_REL               notify_rel
#define PSAGENT_OPS_PHY_STAT                 get_phy_stat
#define PSAGENT_OPS_CHANNEL_STAT             get_channel_stat
#define PSAGENT_OPS_CLEAR_INDEX              clear_index 
#define PSAGENT_OPS_VCH_START                vchannel_start
#define PSAGENT_OPS_VCH_STOP                 vchannel_stop
#define PSAGENT_OPS_CLEAR_ALL                clear_all
#define PSAGENT_OPS_CHECK_PHY_STATUS         check_phy 
#define PSAGENT_OPS_RESET_POINTER            reset_pointer 
#define PSAGENT_OPS_TRAN_ENABLE              tran_enable 

#define PSAGENT_PROCESS(_ops)    \
	do { \
		if (pops && pops->_ops) { \
			if ((pops->_ops)(adap, mif, msg) < 0) { \
				LOGERROR("Psagent process failed."); \
			} \
		} \
	} while (0)

#define TIMER_INTERVAL    1

static void *adap = NULL;
static void *mif = NULL;
static int register_flag = 0;
struct psp_ops *pops = NULL;
char *cfgfile = NULL;

extern struct psp_ops *psagent_ops_register(void);

void psagent_show_usage(char *progname)
{
	printf("    --cfg : Configuration file.\n");
	printf("    --lmsimple : lm simple version\n");
}

void psagent_show_version(char *progname)
{
	printf("psagent - %s\n", PSAGENTVERSION);
}

int psagent_parse_args(int argc, char **argv)
{
	int i = 0;

	while (i < argc) {
		if (strcmp(argv[i], "--cfg") == 0) {
			if (((i + 1) >= argc) || (argv[i + 1][0] == '-'))
				return -1;

            i++;
            cfgfile = argv[i];
		} else if (strcmp(argv[i], "--lmsimple") == 0) {
			psglb.lm_version = LM_VERSION_SIMPLE;
        } else {
            fprintf(stderr, "Unknown option: %s \n", argv[i]);
            return -1;
        }
        i++;
    }

    printf("psagent_parse_args");
    return 0;
}

void psagent_sigalrm_handle()
{
}

void psagent_sigusr1_handle()
{
}

void psagent_sigusr2_handle()
{
}

static void psagent_exit_env()
{
	if (adap) {
		adapter_close(adap);
		adap = NULL;
	}
	if (mif) {
		pops->close(mif);
		mif = NULL;
	}
}

static void psagent_conncb(char *lip, int lport, char *rip, int rport, void *arg)
{
    struct psp_msg *msg = NULL;

	LOG("PSAgent: Connection established.");
#if 0
	unsigned char buf[64], *p;
	int len;
	char s, su;

	len = sizeof(pkt_hdr) + 4;
	memset(buf, 0, sizeof(buf));
	pkthdr_set_sync((pkt_hdr *)buf);
	pkthdr_set_plen((pkt_hdr *)buf, len);
	pkthdr_set_type((pkt_hdr *)buf, (PSP_MSG_REGISTER >> 8) & 0xff);
	pkthdr_set_subtype((pkt_hdr *)buf, (PSP_MSG_REGISTER >> 0) & 0xff);

	s = slot;
	su = subslot;

	p = buf + sizeof(pkt_hdr);
	CODING_8(p, probetype);
	CODING_8(p, cardtype);
	CODING_8(p, s);
	CODING_8(p, su);

	if (adapter_write(adap, buf, len) <= 0) {
		LOGERROR("Failed to send REGISTER message.");
	}
#else 
    pkt_stat.register_cnt++;
    PSAGENT_PROCESS(PSAGENT_OPS_REGISTER);
    /* set fpga reset_pointer */
    PSAGENT_PROCESS(PSAGENT_OPS_RESET_POINTER);
#endif

}

static void psagent_disccb(char *lip, int lport, char *rip, int rport, void *arg)
{
    struct psp_msg  p_msg;
    struct psp_msg *msg = NULL;

	LOG("PSAgent: Connection lost.");
    memset(&p_msg,0,sizeof(struct psp_msg));
    p_msg.tran_en.enable = 0;
    msg = &p_msg;
    /* set tran disable */
    PSAGENT_PROCESS(PSAGENT_OPS_TRAN_ENABLE);
    PSAGENT_PROCESS(PSAGENT_OPS_CLEAR_ALL);
	register_flag = 0;
}

static int psagent_init_parameters(unsigned long cfghd)
{
    char value[128];

    memset(value, 0, 128);
    if (CfgGetValue(cfghd, "Psagent", "probetype", value, 1, 1) < 0) {
        LOGERROR("Failed to load probetype.");
        return -1;
    }

    if (strcmp(value, "E1") == 0)
        psglb.probetype = PSP_PROBETYPE_E1;
    else if (strcmp(value, "155M_CPOS") == 0)
        psglb.probetype = PSP_PROBETYPE_155M_CPOS;
    else if (strcmp(value, "155M_ATM") == 0)
        psglb.probetype = PSP_PROBETYPE_155M_ATM;
    else {
        fprintf(stderr, "Invalid probetype; %s\n", value);
        return -1;
    }

    memset(value, 0, 128);
    if (CfgGetValue(cfghd, "Psagent", "cardtype", value, 1, 1) < 0) {
        LOGERROR("Failed to load cardtype.");
        return -1;
    }
    if (strcmp(value, "AMC") == 0) {
        psglb.cardtype = PSP_CARDTYPE_AMC;
		psglb.ps_port_num = 4;
	}
    else if (strcmp(value, "ATCA") == 0) {
        psglb.cardtype = PSP_CARDTYPE_ATCA;
	}
    else if (strcmp(value, "OIPC") == 0) {
        psglb.cardtype = PSP_CARDTYPE_OIPC;
		if (psglb.lm_version == LM_VERSION_SIMPLE) {
			psglb.ps_port_num = 16;
		}
		else {
			psglb.ps_port_num = 32;
		}
	}
    else {
        fprintf(stderr, "Invalid cardtype: %s\n", value);
        return -1;
    }

    memset(value, 0, 128);
    if (CfgGetValue(cfghd, "Psagent", "slot", value, 1, 1) < 0) {
        LOGERROR("Failed to load slot.");
        return -1;
    }
    psglb.slot = atoi(value);

    memset(value, 0, 128);
    if (CfgGetValue(cfghd, "Psagent", "subslot", value, 1, 1) < 0) {
        LOGERROR("Failed to load subslot.");
        return -1;
    }
    psglb.subslot = atoi(value);

    memset(value, 0, 128);
    if (CfgGetValue(cfghd, "Psagent", "check_phy_status", value, 1, 1) < 0) {
        LOGERROR("Failed to load check_phy_status.");
        return -1;
    }
    if (!strcmp(value, "on")) {
        psglb.check_phy_status = 1;
		memset(value, 0, 128);
		if (CfgGetValue(cfghd, "Psagent", "check_interval", value, 1, 1) < 0) {
			LOGERROR("Failed to load check_interval.");
			return -1;
		}
		psglb.check_interval = atoi(value);
        LOG("check phy status on. interval: %d.", psglb.check_interval); 
    } else if (!strcmp(value, "off")) {
        psglb.check_phy_status = 0;
        LOG("check phy status off !!!"); 
    } else {
        LOGERROR("Unkown check_phy_status %s.", value);
    }

    return 0;
}

static void check_phy_status(void)
{
    struct psp_msg *msg = NULL;

    PSAGENT_PROCESS(PSAGENT_OPS_CHECK_PHY_STATUS);
}

static void printf_log_message(void)
{
	LOG("Send Packets: TOL %llu,REGISTER %llu,GET_PHY_STAT_ACK %llu,GET_CHANNEL_STAT_ACK %llu CLEAR_INDEX_ACK %llu.CHECK_PHY %llu Recv Packets: TOL %llu,REGISTER_ACK %llu,SCAN_CLEAR %llu,ANM %llu,REL %llu,GET_PHY_STAT %llu,GET_CHANNEL_STAT %llu,MISS %llu,CLEAR_INDEX %llu,CLEAR_ALL_INDEX %llu,RESET_POINTER %llu TRAN_ENABLE %llu VCHANNEL_START %llu VCHANNEL_STOP %llu", \
            pkt_stat.spkts, pkt_stat.register_cnt, pkt_stat.get_phy_stat_ack, pkt_stat.get_channel_stat_ack, pkt_stat.clear_index_ack, pkt_stat.check_phy_cnt, pkt_stat.rpkts, pkt_stat.register_ack_cnt, pkt_stat.scan_clear, pkt_stat.notify_anm, pkt_stat.notify_rel, \
            pkt_stat.get_phy_stat, pkt_stat.get_channel_stat, pkt_stat.miss, pkt_stat.clear_index, pkt_stat.clear_all_index, pkt_stat.reset_pointer, pkt_stat.tran_en, pkt_stat.vch_start, pkt_stat.vch_stop);
}

static int psagent_init_env()
{
    unsigned long cfghd;

    printf("psagent_init_env start.");

	pops = psagent_ops_register();
	if (pops == NULL) {
		LOGERROR("Failed to register psagent operations.");
		return -1;
	}

    if ((cfghd = CfgInitialize(cfgfile)) == 0ul) {
        LOGERROR1("Parsing configuration file [%s] failed.", cfgfile);
		return -1;
    }

    if (psagent_init_parameters(cfghd) < 0) {
        LOGERROR("Failed psagent_init_parameters .");
		goto init_env_error_free;
    }

	memset(&pkt_stat, 0, sizeof(pkt_stat));

	mif = pops->open();
	if (mif == NULL) {
		LOGERROR("Failed to open fpgamif.");
		goto init_env_error_free;
	}

    /*get svrip, svrport, txbufsize, rxbufsize from psag.cfg */
	adap = adapter_register_cs(cfghd, "Adapter.PSClient", ap_is_running);
	if (adap == NULL) {
		LOGERROR("Failed to register cs adapter.");
		goto init_env_error_free;
	}
	adapter_cs_setconncb(adap, psagent_conncb, NULL);
	adapter_cs_setdisccb(adap, psagent_disccb, NULL);

	adapter_open(adap);

	return 0;

init_env_error_free:
	psagent_exit_env();

    CfgInvalidate(cfghd);

	return -1;
}

static int psagent_process(struct psp_msg *msg)
{
	int ret = 0;

	switch (msg->id) {
        case PSP_MSG_REGISTER:
            {
                pkt_stat.register_cnt++;
				PSAGENT_PROCESS(PSAGENT_OPS_REGISTER);
            }
            break;
        case PSP_MSG_REGISTER_ACK:
            {
                pkt_stat.register_ack_cnt++;
                if (((pops->register_ack_msg)(adap, mif, msg)) < 0) {
                    ap_stop_running();
                    ret = -1;
                    LOGERROR("register_ack failed. ap stop running.");
                }
            }
            break;
		case PSP_MSG_SCAN_CLEAR:
			{
				pkt_stat.scan_clear++;
				PSAGENT_PROCESS(PSAGENT_OPS_SCAN_CLEAR);
			}
			break;
		case PSP_MSG_NOTIFY_ANM:
			{
				pkt_stat.notify_anm++;
				PSAGENT_PROCESS(PSAGENT_OPS_NOTIFY_ANM);
			}
			break;
		case PSP_MSG_NOTIFY_REL:
			{
				pkt_stat.notify_rel++;
				PSAGENT_PROCESS(PSAGENT_OPS_NOTIFY_REL);
			}
			break;
		case PSP_MSG_PHY_STAT:
			{
				pkt_stat.get_phy_stat++;
				PSAGENT_PROCESS(PSAGENT_OPS_PHY_STAT);
			}
			break;
		case PSP_MSG_CHANNEL_STAT:
            {
				pkt_stat.get_channel_stat++;
				PSAGENT_PROCESS(PSAGENT_OPS_CHANNEL_STAT);
            }
            break;
		case PSP_MSG_CLEAR_INDEX:
            {
				pkt_stat.clear_index++;
				PSAGENT_PROCESS(PSAGENT_OPS_CLEAR_INDEX);
            }
            break;
		case PSP_MSG_VCHANNEL_START:
            {
				pkt_stat.vch_stop++;
				PSAGENT_PROCESS(PSAGENT_OPS_VCH_START);
            }
            break;
		case PSP_MSG_VCHANNEL_STOP:
            {
				pkt_stat.vch_start++;
				PSAGENT_PROCESS(PSAGENT_OPS_VCH_STOP);
            }
            break;
        case PSP_MSG_CLEAR_ALL_INDEX:
            {
				pkt_stat.clear_all_index++;
				PSAGENT_PROCESS(PSAGENT_OPS_CLEAR_ALL);
            }
            break;
        case PSP_MSG_RESET_POINTER:
            {
				pkt_stat.reset_pointer++;
				PSAGENT_PROCESS(PSAGENT_OPS_RESET_POINTER);
            }
            break;
        case PSP_MSG_TRAN_ENABLE:
            {
				pkt_stat.tran_en++;
				PSAGENT_PROCESS(PSAGENT_OPS_TRAN_ENABLE);
            }
            break;
		default:
			{
				pkt_stat.miss++;
				LOGERROR("Bad psp msg id:%d.", msg->id);
			}
			break;
	}

	return ret;
}

#define CONTINUE 2
static int check_register_ack(pkt_hdr *ph)
{
	struct psp_msg msg;

    SLEEP_MS(1);
    //times++;

    if (ph != NULL) {
        LGWRDEBUG(ph, pkthdr_get_plen(ph), "Packet:");
        pkt_stat.rpkts++;
        memset(&msg, 0, sizeof(struct psp_msg));
        if (psp_decode_msg(ph, &msg) == 0) {
            if (msg.id != PSP_MSG_REGISTER_ACK) {
                LOGERROR("PSP_MSG_REGISTER_ACK: should recv 0x%x, but recv 0x%x", PSP_MSG_REGISTER_ACK, msg.id);
                if (msg.id == (PSP_MSG_REGISTER | 0x0400)) {
                    /*receive  unack: do send register_cmd again*/
                    msg.id = PSP_MSG_REGISTER;
                    psagent_process(&msg);
    //                times = 0;
                }
            } else {
                LOG("PSP_MSG_REGISTER_ACK sucess.");
                register_flag = 1;
                psagent_process(&msg);
                return 0;
            }
        }
    }

#if 0
    if (times != 0 && times % 3000 == 0) {
        /*outtime 3s: do send register_cmd again*/ 
        LOGNOTICE("waiting 3s for REGISTER_CMD_ACK, send REGISTER_CMD again.");
        times = 0;
        msg.id = PSP_MSG_REGISTER;
        psagent_process(&msg);
    }
#endif

    return 0;
}

static int check_device(pkt_hdr *ph)
{
    unsigned short device = 0;
    unsigned short local_device = 0;
    unsigned int id = 0;

    if (ph != NULL) {
        device = pkthdr_get_device(ph);
        id = (pkthdr_get_type(ph) << 8) | pkthdr_get_subtype(ph);
		if (psglb.cardtype == PSP_CARDTYPE_AMC) {
			local_device = ((psglb.slot & 0x0f) << 4) | ((psglb.subslot & 0x0f) << 0);
		}
		else if (psglb.cardtype == PSP_CARDTYPE_OIPC) {
			local_device = psglb.slot;
		}
        if (local_device  != (device & 0xff)) {
            LOGERROR("adapter_read msg->id: 0x%x slot: %d is diff from local_device : %d", id, device, local_device);
            return -1; 
        }
    }

    return 0;
}

int psagent_run(long instance, unsigned long data)
{
	pkt_hdr *ph;
	struct psp_msg msg;
	struct timeval t;
	unsigned int pre = 0, pre1 = 0, cur = 0;

	if (psagent_init_env() < 0) {
        LOGERROR("failed psagent_init_env.");
		return -1;
    }
    
	while (ap_is_running()) {

		gettimeofday(&t, NULL);
		cur = (unsigned int)t.tv_sec;
		/* print log */
		if ((cur != pre) && (cur - pre) >= 600) {
			printf_log_message();
			pre = cur;
		}
		/* check phy status and report link change */
		if (psglb.check_phy_status) {
			if ((cur != pre1) && (cur - pre1) >= psglb.check_interval) {
				check_phy_status();
				pre1 = cur;
			}
		}

		ph = (pkt_hdr *)adapter_read(adap);

        if (check_device(ph) != 0) {
            free(ph);
            continue;
        }

        if (register_flag == 0) {
            check_register_ack(ph);
            free(ph);
            continue;
        }

        if (ph != NULL) {
            LGWRDEBUG(ph, pkthdr_get_plen(ph), "Packet:");
            pkt_stat.rpkts++;
            memset(&msg, 0, sizeof(&msg));
            if (psp_decode_msg(ph, &msg) != 0) {
                LOGERROR("psp_decode_msg failed.");
                free(ph);
                continue;
            }
            /*psagent linkmap process*/
            psagent_process(&msg);
            free(ph);
        } else {
			usleep(100);
        }
    }
	printf_log_message();

	psagent_exit_env();

	return 0;
}

static struct ap_framework psagent_app = {
	NULL,
	psagent_run,
	0,
	psagent_sigalrm_handle,
	psagent_sigusr1_handle,
	psagent_sigusr2_handle,
	psagent_show_usage,
	psagent_show_version,
	psagent_parse_args
};

#if defined(__cplusplus)
extern "C" {
#endif

struct ap_framework *register_ap(void)
{
	return &psagent_app;
}

#if defined(__cplusplus)
}
#endif

