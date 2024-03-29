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
#include "sdhPhyMibPortFlowStatTable.h"

    /* Typical data structure for a row entry */
struct sdhPhyMibPortFlowStatTable_entry {
    /* Index values */
    in_addr_t sdhPhyMibPortFlowStatPosition;
    long sdhPhyMibPortFlowStatPort;

    /* Column values */
    u_long sdhPhyMibPortFlowStatValue;

    /* Illustrate using a simple linked list */
    int   valid;
    struct sdhPhyMibPortFlowStatTable_entry *next;
};

struct sdhPhyMibPortFlowStatTable_entry  *sdhPhyMibPortFlowStatTable_head = NULL;
static int sdhPhyMibPortFlowStatTable_count = 0;
static void *lstlock = NULL;

/** Initializes the sdhPhyMibPortFlowStatTable module */
void
init_sdhPhyMibPortFlowStatTable(void)
{
	lstlock = mutex_open(NULL);
	if (lstlock == NULL) {
		LOGERROR("%s: mutex_open failed", __func__);
		return;
	}
  /* here we initialize all the tables we're planning on supporting */
    initialize_table_sdhPhyMibPortFlowStatTable();
}

void exit_sdhPhyMibPortFlowStatTable(void)
{
	sdhPhyMibPortFlowStatTable_clear();

	if (lstlock) 
		mutex_close(lstlock);
}

/** Initialize the sdhPhyMibPortFlowStatTable table by defining its contents and how it's structured */
void
initialize_table_sdhPhyMibPortFlowStatTable(void)
{
    const oid sdhPhyMibPortFlowStatTable_oid[] = {1,3,6,1,4,1,8990,1,4,2};
    const size_t sdhPhyMibPortFlowStatTable_oid_len   = OID_LENGTH(sdhPhyMibPortFlowStatTable_oid);
    netsnmp_handler_registration    *reg;
    netsnmp_iterator_info           *iinfo;
    netsnmp_table_registration_info *table_info;

    DEBUGMSGTL(("sdhPhyMibPortFlowStatTable:init", "initializing table sdhPhyMibPortFlowStatTable\n"));

    reg = netsnmp_create_handler_registration(
              "sdhPhyMibPortFlowStatTable",     sdhPhyMibPortFlowStatTable_handler,
              sdhPhyMibPortFlowStatTable_oid, sdhPhyMibPortFlowStatTable_oid_len,
              HANDLER_CAN_RONLY);

    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_IPADDRESS,  /* index: sdhPhyMibPortFlowStatPosition */
                           ASN_INTEGER,  /* index: sdhPhyMibPortFlowStatPort */
                           0);
    table_info->min_column = COLUMN_SDHPHYMIBPORTFLOWSTATPOSITION;
    table_info->max_column = COLUMN_SDHPHYMIBPORTFLOWSTATVALUE;
    
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->get_first_data_point = sdhPhyMibPortFlowStatTable_get_first_data_point;
    iinfo->get_next_data_point  = sdhPhyMibPortFlowStatTable_get_next_data_point;
    iinfo->table_reginfo        = table_info;
    
    netsnmp_register_table_iterator( reg, iinfo );

    /* Initialise the contents of the table here */
}

/* create a new row in the (unsorted) table */
struct sdhPhyMibPortFlowStatTable_entry *
sdhPhyMibPortFlowStatTable_createEntry(in_addr_t  sdhPhyMibPortFlowStatPosition,
                 long  sdhPhyMibPortFlowStatPort) {
    struct sdhPhyMibPortFlowStatTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(struct sdhPhyMibPortFlowStatTable_entry);
    if (!entry)
        return NULL;

    entry->sdhPhyMibPortFlowStatPosition = sdhPhyMibPortFlowStatPosition;
    entry->sdhPhyMibPortFlowStatPort = sdhPhyMibPortFlowStatPort;
    entry->next = sdhPhyMibPortFlowStatTable_head;
    sdhPhyMibPortFlowStatTable_head = entry;
	sdhPhyMibPortFlowStatTable_count++;

    return entry;
}

/* remove a row from the table */
void
sdhPhyMibPortFlowStatTable_removeEntry( struct sdhPhyMibPortFlowStatTable_entry *entry ) {
    struct sdhPhyMibPortFlowStatTable_entry *ptr, *prev;

    if (!entry)
        return;    /* Nothing to remove */

    for ( ptr  = sdhPhyMibPortFlowStatTable_head, prev = NULL;
          ptr != NULL;
          prev = ptr, ptr = ptr->next ) {
        if ( ptr == entry )
            break;
    }
    if ( !ptr )
        return;    /* Can't find it */

    if ( prev == NULL )
        sdhPhyMibPortFlowStatTable_head = ptr->next;
    else
        prev->next = ptr->next;

    SNMP_FREE( entry );   /* XXX - release any other internal resources */
	sdhPhyMibPortFlowStatTable_count--;
}


/* Example iterator hook routines - using 'get_next' to do most of the work */
netsnmp_variable_list *
sdhPhyMibPortFlowStatTable_get_first_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    *my_loop_context = sdhPhyMibPortFlowStatTable_head;
    return sdhPhyMibPortFlowStatTable_get_next_data_point(my_loop_context, my_data_context,
                                    put_index_data,  mydata );
}

netsnmp_variable_list *
sdhPhyMibPortFlowStatTable_get_next_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    struct sdhPhyMibPortFlowStatTable_entry *entry = (struct sdhPhyMibPortFlowStatTable_entry *)*my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

	mutex_lock(lstlock);

    if ( entry ) {
        snmp_set_var_typed_integer( idx, ASN_IPADDRESS, entry->sdhPhyMibPortFlowStatPosition );
        idx = idx->next_variable;
        snmp_set_var_typed_integer( idx, ASN_INTEGER, entry->sdhPhyMibPortFlowStatPort );
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

int sdhPhyMibPortFlowStatTable_compare(struct sdhPhyMibPortFlowStatTable_entry *entry, in_addr_t po)
{
	if (!entry)
		return -1;

	if (entry->sdhPhyMibPortFlowStatPosition == po)
		return 0;

	return -1;
}

DEFINE_TABLE_ENTRY_DELETE_FUNC(sdhPhyMibPortFlowStatTable) 
DEFINE_TABLE_DELETE_FUNC(sdhPhyMibPortFlowStatTable) 

void sdhPhyMibPortFlowStatTable_updateEntry(struct sdhPhyMibPortFlowStatTable_entry *entry, 
		in_addr_t po, int port, unsigned int value)
{
	
	entry->sdhPhyMibPortFlowStatPosition = po;
	entry->sdhPhyMibPortFlowStatPort     = port;

	entry->sdhPhyMibPortFlowStatValue    = value;
}

int sdhPhyMibPortFlowStatTable_update(struct nm_lkaddr_info po, int bdtype, void *data)
{
	struct sdhPhyMibPortFlowStatTable_entry *entry = sdhPhyMibPortFlowStatTable_head;
	struct fpga_fstat *flow;
	in_addr_t position;	
	int port;
	unsigned int i;
	
	if (sdhPhyMibPortFlowStatTable_count >= SDHPHYMAXITEMS) 
	{
		LOGERROR("sdhphyflowstat:item over the max row %d", SDHPHYMAXITEMS);
		return -1;
	}
	
	mutex_lock(lstlock);
	/* 1: delete board sdhphyinfo 2: add new data */
	position = (po.rack << 24) | (po.shelf << 16) | (po.slot << 8) | po.subslot;
	sdhPhyMibPortFlowStatTable_delete(position);

	if (bdtype == BOARD_TYPE_OI_AMC_V3) {
		flow = (struct fpga_fstat *)data;
		for (i = 0; i < flow->pcnt; i++) {
			port = i + 1;
			entry = sdhPhyMibPortFlowStatTable_createEntry(position, port);
			if (!entry) {
				LOGERROR("%s: createEntry failed", __func__);
				continue;
			}
			sdhPhyMibPortFlowStatTable_updateEntry(entry, position, port, flow->fsta[i].flow);
		}
	}

	mutex_unlock(lstlock);
	return 0;
}

DEFINE_TABLE_ENTRY_DELETE_INVALID_FUNC(sdhPhyMibPortFlowStatTable) 

void sdhPhyMibPortFlowStatTable_check_and_clear(void)
{
	struct sdhPhyMibPortFlowStatTable_entry *entry = sdhPhyMibPortFlowStatTable_head;
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
	cnt = boardsInfoTable_getboards(BOARD_TYPE_OI_AMC_V3, po);
	for (i = 0; i < cnt; i++) {
		position = (po[i].rack << 24) | (po[i].shelf << 16) | (po[i].slot << 8) | po[i].subslot;
		entry = sdhPhyMibPortFlowStatTable_head;
		while (entry) {
			if (entry->sdhPhyMibPortFlowStatPosition == position) {
				entry->valid = 1;
			}
			entry = entry->next;
		}
	}

	/* clear invalid */
	sdhPhyMibPortFlowStatTable_head =
		sdhPhyMibPortFlowStatTable_del_invalid_entrys(sdhPhyMibPortFlowStatTable_head);

	mutex_unlock(lstlock);
}

void sdhPhyMibPortFlowStatTable_clear(void)
{

	mutex_lock(lstlock);

	while (sdhPhyMibPortFlowStatTable_head) {
		sdhPhyMibPortFlowStatTable_removeEntry(sdhPhyMibPortFlowStatTable_head);
	}

	sdhPhyMibPortFlowStatTable_head = NULL;
	sdhPhyMibPortFlowStatTable_count = 0;

	mutex_unlock(lstlock);
}


/** handles requests for the sdhPhyMibPortFlowStatTable table */
int sdhPhyMibPortFlowStatTable_handler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests) {

    netsnmp_request_info       *request;
    netsnmp_table_request_info *table_info;
    struct sdhPhyMibPortFlowStatTable_entry          *table_entry;

    DEBUGMSGTL(("sdhPhyMibPortFlowStatTable:handler", "Processing request (%d)\n", reqinfo->mode));

	mutex_lock(lstlock);

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request=requests; request; request=request->next) {
            table_entry = (struct sdhPhyMibPortFlowStatTable_entry *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
    
			if ( !table_entry ) {
				continue;
			}

            switch (table_info->colnum) {
            case COLUMN_SDHPHYMIBPORTFLOWSTATPOSITION:
                snmp_set_var_typed_integer( request->requestvb, ASN_IPADDRESS,
                                            table_entry->sdhPhyMibPortFlowStatPosition);
                break;
            case COLUMN_SDHPHYMIBPORTFLOWSTATPORT:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->sdhPhyMibPortFlowStatPort);
                break;
            case COLUMN_SDHPHYMIBPORTFLOWSTATVALUE:
                snmp_set_var_typed_integer( request->requestvb, ASN_COUNTER,
                                            table_entry->sdhPhyMibPortFlowStatValue);
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
