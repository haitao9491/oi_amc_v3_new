/*
 * Note: this file originally auto-generated by mib2c using
 *  $
 */
#ifndef E1PHYMIBPORTINFOSTATTABLE_H
#define E1PHYMIBPORTINFOSTATTABLE_H

#include "nm_glb.h"

/* function declarations */
void init_e1PhyMibPortInfoStatTable(void);
void exit_e1PhyMibPortInfoStatTable(void);
void initialize_table_e1PhyMibPortInfoStatTable(void);
int e1PhyMibPortInfoStatTable_update(struct nm_lkaddr_info po, int bdtype, void *data);
int e1PhyMibPortInfoStatTable_delete(in_addr_t po);
void e1PhyMibPortInfoStatTable_check_and_clear(void);
void e1PhyMibPortInfoStatTable_clear(void);
Netsnmp_Node_Handler e1PhyMibPortInfoStatTable_handler;
Netsnmp_First_Data_Point  e1PhyMibPortInfoStatTable_get_first_data_point;
Netsnmp_Next_Data_Point   e1PhyMibPortInfoStatTable_get_next_data_point;

/* column number definitions for table e1PhyMibPortInfoStatTable */
       #define COLUMN_E1PHYMIBPORTINFOSTATTABLEPOSITION		1
       #define COLUMN_E1PHYMIBPORTINFOSTATTABLEPORTGROUP		2
       #define COLUMN_E1PHYMIBPORTINFOSTATTABLESIG64KCHCNT		3
       #define COLUMN_E1PHYMIBPORTINFOSTATTABLESIG64KFRCNT		4
       #define COLUMN_E1PHYMIBPORTINFOSTATTABLESIG2MCHCNT		5
       #define COLUMN_E1PHYMIBPORTINFOSTATTABLESIG2MFRCNT		6
#endif /* E1PHYMIBPORTINFOSTATTABLE_H */
