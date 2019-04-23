/*
 * Note: this file originally auto-generated by mib2c using
 *  $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "os.h"
#include "aplog.h"
#include "bdtype.h"
#include "mutex.h"
#include "nm_typdef.h"
#include "nmpkt.h"
#include "nm_glb.h"
#include "snmpcommon.h"
#include "phyadapt.h"
#include "nm_fpga.h"
#include "snmpMibModule.h"
#include "boardsInfoTable.h"
#include "e1PhyMibPortInfoStatTable.h"

    /* Typical data structure for a row entry */
struct e1PhyMibPortInfoStatTable_entry {
    /* Index values */
    in_addr_t e1PhyMibPortInfoStatTablePosition;
    long e1PhyMibPortInfoStatTablePortGroup;

    /* Column values */
    u_long e1PhyMibPortInfoStatTableSIG64KCHCNT;
    u_long e1PhyMibPortInfoStatTableSIG64KFRCNT;
    u_long e1PhyMibPortInfoStatTableSIG2MCHCNT;
    u_long e1PhyMibPortInfoStatTableSIG2MFRCNT;

    /* Illustrate using a simple linked list */
    int   valid;
    struct e1PhyMibPortInfoStatTable_entry *next;
};

struct e1PhyMibPortInfoStatTable_entry  *e1PhyMibPortInfoStatTable_head = NULL;
static int e1PhyMibPortInfoStatTable_count = 0;
static void *lstlock = NULL;

/** Initializes the e1PhyMibPortInfoStatTable module */
void
init_e1PhyMibPortInfoStatTable(void)
{
	lstlock = mutex_open(NULL);
	if (lstlock == NULL) {
		LOGERROR("%s: mutex_open failed", __func__);
		return;
	}
  /* here we initialize all the tables we're planning on supporting */
    initialize_table_e1PhyMibPortInfoStatTable();
}

void exit_e1PhyMibPortInfoStatTable(void)
{
	e1PhyMibPortInfoStatTable_clear();

	if (lstlock) 
		mutex_close(lstlock);
}

/** Initialize the e1PhyMibPortInfoStatTable table by defining its contents and how it's structured */
void
initialize_table_e1PhyMibPortInfoStatTable(void)
{
    const oid e1PhyMibPortInfoStatTable_oid[] = {1,3,6,1,4,1,8990,1,3,2};
    const size_t e1PhyMibPortInfoStatTable_oid_len   = OID_LENGTH(e1PhyMibPortInfoStatTable_oid);
    netsnmp_handler_registration    *reg;
    netsnmp_iterator_info           *iinfo;
    netsnmp_table_registration_info *table_info;

    DEBUGMSGTL(("e1PhyMibPortInfoStatTable:init", "initializing table e1PhyMibPortInfoStatTable\n"));

    reg = netsnmp_create_handler_registration(
              "e1PhyMibPortInfoStatTable",     e1PhyMibPortInfoStatTable_handler,
              e1PhyMibPortInfoStatTable_oid, e1PhyMibPortInfoStatTable_oid_len,
              HANDLER_CAN_RONLY);

    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_IPADDRESS,  /* index: e1PhyMibPortInfoStatTablePosition */
                           ASN_INTEGER,  /* index: e1PhyMibPortInfoStatTablePortGroup */
                           0);
    table_info->min_column = COLUMN_E1PHYMIBPORTINFOSTATTABLEPOSITION;
    table_info->max_column = COLUMN_E1PHYMIBPORTINFOSTATTABLESIG2MFRCNT;
    
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->get_first_data_point = e1PhyMibPortInfoStatTable_get_first_data_point;
    iinfo->get_next_data_point  = e1PhyMibPortInfoStatTable_get_next_data_point;
    iinfo->table_reginfo        = table_info;
    
    netsnmp_register_table_iterator( reg, iinfo );

    /* Initialise the contents of the table here */
}

/* create a new row in the (unsorted) table */
struct e1PhyMibPortInfoStatTable_entry *
e1PhyMibPortInfoStatTable_createEntry(in_addr_t  e1PhyMibPortInfoStatTablePosition,
                 long  e1PhyMibPortInfoStatTablePortGroup) {
    struct e1PhyMibPortInfoStatTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(struct e1PhyMibPortInfoStatTable_entry);
    if (!entry)
        return NULL;

    entry->e1PhyMibPortInfoStatTablePosition = e1PhyMibPortInfoStatTablePosition;
    entry->e1PhyMibPortInfoStatTablePortGroup = e1PhyMibPortInfoStatTablePortGroup;
    entry->next = e1PhyMibPortInfoStatTable_head;
    e1PhyMibPortInfoStatTable_head = entry;
	e1PhyMibPortInfoStatTable_count++;

    return entry;
}

/* remove a row from the table */
void
e1PhyMibPortInfoStatTable_removeEntry( struct e1PhyMibPortInfoStatTable_entry *entry ) {
    struct e1PhyMibPortInfoStatTable_entry *ptr, *prev;

    if (!entry)
        return;    /* Nothing to remove */

    for ( ptr  = e1PhyMibPortInfoStatTable_head, prev = NULL;
          ptr != NULL;
          prev = ptr, ptr = ptr->next ) {
        if ( ptr == entry )
            break;
    }
    if ( !ptr )
        return;    /* Can't find it */

    if ( prev == NULL )
        e1PhyMibPortInfoStatTable_head = ptr->next;
    else
        prev->next = ptr->next;

    SNMP_FREE( entry );   /* XXX - release any other internal resources */
	e1PhyMibPortInfoStatTable_count--;
}


/* Example iterator hook routines - using 'get_next' to do most of the work */
netsnmp_variable_list *
e1PhyMibPortInfoStatTable_get_first_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    *my_loop_context = e1PhyMibPortInfoStatTable_head;
    return e1PhyMibPortInfoStatTable_get_next_data_point(my_loop_context, my_data_context,
                                    put_index_data,  mydata );
}

netsnmp_variable_list *
e1PhyMibPortInfoStatTable_get_next_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    struct e1PhyMibPortInfoStatTable_entry *entry = (struct e1PhyMibPortInfoStatTable_entry *)*my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

	mutex_lock(lstlock);

    if ( entry ) {
        snmp_set_var_typed_integer( idx, ASN_IPADDRESS, entry->e1PhyMibPortInfoStatTablePosition );
        idx = idx->next_variable;
        snmp_set_var_typed_integer( idx, ASN_INTEGER, entry->e1PhyMibPortInfoStatTablePortGroup );
        idx = idx->next_variable;
        *my_data_context = (void *)entry;
        *my_loop_context = (void *)entry->next;
		mutex_unlock(lstlock);

        return put_index_data;
    } else {
		mutex_unlock(lstlock);
        return NULL;
    }
}

int e1PhyMibPortInfoStatTable_compare(struct e1PhyMibPortInfoStatTable_entry *entry, in_addr_t po)
{
	if (!entry)
		return -1;

	if (entry->e1PhyMibPortInfoStatTablePosition == po)
		return 0;

	return -1;
}

DEFINE_TABLE_ENTRY_DELETE_FUNC(e1PhyMibPortInfoStatTable) 
DEFINE_TABLE_DELETE_FUNC(e1PhyMibPortInfoStatTable) 

void e1PhyMibPortInfoStatTable_updateEntry(struct e1PhyMibPortInfoStatTable_entry *entry, 
		in_addr_t po, int port, struct fpga_port_stat *ptr)
{
	
	entry->e1PhyMibPortInfoStatTablePosition    = po;
	entry->e1PhyMibPortInfoStatTablePortGroup   = port;

	entry->e1PhyMibPortInfoStatTableSIG64KCHCNT = ptr->sig64kchcnt;
	entry->e1PhyMibPortInfoStatTableSIG64KFRCNT = ptr->sig64kchcnt;
	entry->e1PhyMibPortInfoStatTableSIG2MCHCNT  = ptr->sig2mchcnt;
	entry->e1PhyMibPortInfoStatTableSIG2MFRCNT  = ptr->sig2mfrcnt;
}

int e1PhyMibPortInfoStatTable_update(struct nm_lkaddr_info po, int bdtype, void *data)
{
	struct e1PhyMibPortInfoStatTable_entry *entry = e1PhyMibPortInfoStatTable_head;
	struct fpga_stat *fs;
	in_addr_t position;	
	int port;
	unsigned int i;
	
	if (e1PhyMibPortInfoStatTable_count >= E1PHYMAXITEMS) 
	{
		LOGERROR("e1PhyInfoStat: item over the max row %d", E1PHYMAXITEMS);
		return -1;
	}
	
	mutex_lock(lstlock);
	/* 1: delete board sdhphyinfo 2: add new data */
	position = (po.rack << 24) | (po.shelf << 16) | (po.slot << 8) | po.subslot;
	e1PhyMibPortInfoStatTable_delete(position);

	if (bdtype == BOARD_TYPE_EIPB_V2) {
		fs = (struct fpga_stat *)data;
		for (i = 0; i < fs->pcnt; i++) {
			port = i + 1;
			entry = e1PhyMibPortInfoStatTable_createEntry(position, port);
			if (!entry) {
				LOGERROR("%s: createEntry failed", __func__);
				continue;
			}
			e1PhyMibPortInfoStatTable_updateEntry(entry, position, port, &fs->sta[i]);
		}
	}

	mutex_unlock(lstlock);
	return 0;
}

DEFINE_TABLE_ENTRY_DELETE_INVALID_FUNC(e1PhyMibPortInfoStatTable) 

void e1PhyMibPortInfoStatTable_check_and_clear(void)
{
	struct e1PhyMibPortInfoStatTable_entry *entry = e1PhyMibPortInfoStatTable_head;
	struct nm_lkaddr_info po[SNMP_MAX_BOARD_CNT];
	int cnt = 0;
	in_addr_t position;
	int i;

	if (!entry)
		return;

	mutex_lock(lstlock);

	/* reset valid */
	while (entry) {
		entry->valid = 0;
		entry = entry->next;
	}

	/* get boards, and set valid */
	memset(po, 0, sizeof(po));
	cnt = boardsInfoTable_getboards(BOARD_TYPE_EIPB_V2, po);
	for (i = 0; i < cnt; i++) {
		position = (po[i].rack << 24) | (po[i].shelf << 16) | (po[i].slot << 8) | po[i].subslot;
		entry = e1PhyMibPortInfoStatTable_head;
		while (entry) {
			if (entry->e1PhyMibPortInfoStatTablePosition == position) {
				entry->valid = 1;
			}
			entry = entry->next;
		}
	}

	/* clear invalid */
	e1PhyMibPortInfoStatTable_head =
		e1PhyMibPortInfoStatTable_del_invalid_entrys(e1PhyMibPortInfoStatTable_head);

	mutex_unlock(lstlock);
}

void e1PhyMibPortInfoStatTable_clear(void)
{

	mutex_lock(lstlock);

	while (e1PhyMibPortInfoStatTable_head) {
		e1PhyMibPortInfoStatTable_removeEntry(e1PhyMibPortInfoStatTable_head);
	}

	e1PhyMibPortInfoStatTable_head = NULL;
	e1PhyMibPortInfoStatTable_count = 0;

	mutex_unlock(lstlock);
}

/** handles requests for the e1PhyMibPortInfoStatTable table */
int
e1PhyMibPortInfoStatTable_handler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests) {

    netsnmp_request_info       *request;
    netsnmp_table_request_info *table_info;
    struct e1PhyMibPortInfoStatTable_entry          *table_entry;

    DEBUGMSGTL(("e1PhyMibPortInfoStatTable:handler", "Processing request (%d)\n", reqinfo->mode));

	mutex_lock(lstlock);

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request=requests; request; request=request->next) {
            table_entry = (struct e1PhyMibPortInfoStatTable_entry *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
    
			if ( !table_entry ) {
				continue;
			}

            switch (table_info->colnum) {
            case COLUMN_E1PHYMIBPORTINFOSTATTABLEPOSITION:
                snmp_set_var_typed_integer( request->requestvb, ASN_IPADDRESS,
                                            table_entry->e1PhyMibPortInfoStatTablePosition);
                break;
            case COLUMN_E1PHYMIBPORTINFOSTATTABLEPORTGROUP:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->e1PhyMibPortInfoStatTablePortGroup);
                break;
            case COLUMN_E1PHYMIBPORTINFOSTATTABLESIG64KCHCNT:
                snmp_set_var_typed_integer( request->requestvb, ASN_COUNTER,
                                            table_entry->e1PhyMibPortInfoStatTableSIG64KCHCNT);
                break;
            case COLUMN_E1PHYMIBPORTINFOSTATTABLESIG64KFRCNT:
                snmp_set_var_typed_integer( request->requestvb, ASN_COUNTER,
                                            table_entry->e1PhyMibPortInfoStatTableSIG64KFRCNT);
                break;
            case COLUMN_E1PHYMIBPORTINFOSTATTABLESIG2MCHCNT:
                snmp_set_var_typed_integer( request->requestvb, ASN_COUNTER,
                                            table_entry->e1PhyMibPortInfoStatTableSIG2MCHCNT);
                break;
            case COLUMN_E1PHYMIBPORTINFOSTATTABLESIG2MFRCNT:
                snmp_set_var_typed_integer( request->requestvb, ASN_COUNTER,
                                            table_entry->e1PhyMibPortInfoStatTableSIG2MFRCNT);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;

    }

	mutex_unlock(lstlock);
    return SNMP_ERR_NOERROR;
}
