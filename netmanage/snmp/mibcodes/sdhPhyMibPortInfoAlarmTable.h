/*
 * Note: this file originally auto-generated by mib2c using
 *  $
 */
#ifndef SDHPHYMIBPORTINFOALARMTABLE_H
#define SDHPHYMIBPORTINFOALARMTABLE_H

#include "nm_glb.h"

/* function declarations */
void init_sdhPhyMibPortInfoAlarmTable(void);
void exit_sdhPhyMibPortInfoAlarmTable(void);
void initialize_table_sdhPhyMibPortInfoAlarmTable(void);
int sdhPhyMibPortInfoAlarmTable_update(struct nm_lkaddr_info po, int bdtype, void *data);
int sdhPhyMibPortInfoAlarmTable_delete(in_addr_t po);
void sdhPhyMibPortInfoAlarmTable_check_and_clear(void);
void sdhPhyMibPortInfoAlarmTable_clear(void);
Netsnmp_Node_Handler sdhPhyMibPortInfoAlarmTable_handler;
Netsnmp_First_Data_Point  sdhPhyMibPortInfoAlarmTable_get_first_data_point;
Netsnmp_Next_Data_Point   sdhPhyMibPortInfoAlarmTable_get_next_data_point;

/* column number definitions for table sdhPhyMibPortInfoAlarmTable */
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLEPOSITION		1
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLEPORT		2
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLELOS		3
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLELOF		4
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLESTM1		5
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLEE1CNT		6
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLESIG64KCHCNT		7
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLESIG64KFRCNT		8
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLESIG2MCHCNT		9
       #define COLUMN_SDHPHYMIBPORTINFOALARMTABLESIG2MFRCNT		10
#endif /* SDHPHYMIBPORTINFOALARMTABLE_H */