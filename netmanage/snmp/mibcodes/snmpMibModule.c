/*
 *
 * cstGroomerMibModule.c - A brief description goes here.
 *
 */

#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "os.h"
#include "aplog.h"
#include "platform_info.h"
#include "boardsInfoTable.h"
#include "e1PhyMibPortInfoAlarmTable.h"
#include "e1PhyMibPortInfoStatTable.h"
#include "e1PhyMibPortFlowStatTable.h"
#include "sdhPhyMibPortInfoAlarmTable.h"
#include "sdhPhyMibPortFlowStatTable.h"
#include "switchMibPortInfoStatTable.h"
#include "socketAdapt.h"
#include "phyadapt.h"

DECLARE_LOGHANDLE;

void init_snmpMibModule(void)
{
	LGWROPEN("/tmp/snmp.log", LGWRLEVELERROR, 1024 * 1024);

	init_platform_info();
	init_boardsInfoTable();
	init_e1PhyMibPortInfoAlarmTable();
#if 0
	init_e1PhyMibPortInfoStatTable();
	init_e1PhyMibPortFlowStatTable();
	init_sdhPhyMibPortInfoAlarmTable();
	init_sdhPhyMibPortFlowStatTable();
#endif
	init_switchMibPortInfoStatTable();
	init_socketAdapt();
	sleep(1);
}

void exit_snmpMibModule(void)
{
	exit_boardsInfoTable();
	exit_e1PhyMibPortInfoAlarmTable();
#if 0
	exit_e1PhyMibPortInfoStatTable();
	exit_e1PhyMibPortFlowStatTable();
	exit_sdhPhyMibPortInfoAlarmTable();
	exit_sdhPhyMibPortFlowStatTable();
#endif
	exit_switchMibPortInfoStatTable();
	exit_socketAdapt();
	LGWRCLOSE();
}
