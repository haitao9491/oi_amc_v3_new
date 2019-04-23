/*
 * Note: this file originally auto-generated by mib2c using
 *  $
 */
#ifndef E1PHYMIBPORTFLOWSTATTABLE_H
#define E1PHYMIBPORTFLOWSTATTABLE_H

#include "nm_glb.h"

/* function declarations */
void init_e1PhyMibPortFlowStatTable(void);
void exit_e1PhyMibPortFlowStatTable(void);
void initialize_table_e1PhyMibPortFlowStatTable(void);
int e1PhyMibPortFlowStatTable_update(struct nm_lkaddr_info po, int bdtype, void *data);
int e1PhyMibPortFlowStatTable_delete(in_addr_t po);
void e1PhyMibPortFlowStatTable_check_and_clear(void);
void e1PhyMibPortFlowStatTable_clear(void);
Netsnmp_Node_Handler e1PhyMibPortFlowStatTable_handler;
Netsnmp_First_Data_Point  e1PhyMibPortFlowStatTable_get_first_data_point;
Netsnmp_Next_Data_Point   e1PhyMibPortFlowStatTable_get_next_data_point;

/* column number definitions for table e1PhyMibPortFlowStatTable */
       #define COLUMN_E1PHYMIBPORTFLOWSTATPOSITION		1
       #define COLUMN_E1PHYMIBPORTFLOWSTATPORT		2
       #define COLUMN_E1PHYMIBPORTFLOWSTATVALUE		3
#endif /* E1PHYMIBPORTFLOWSTATTABLE_H */