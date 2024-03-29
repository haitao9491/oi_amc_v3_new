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
#include "phyadapt.h"
#include "snmpcommon.h"
#include "nmmaptbl.h"
#include "nm_switch.h"
#include "snmpMibModule.h"
#include "boardsInfoTable.h"
#include "switchMibPortInfoStatTable.h"

#define SWITCH_PORT_NAME_LEN     32

    /* Typical data structure for a row entry */
struct switchMibPortInfoStatTable_entry {
    /* Index values */
    in_addr_t switchMibPortInfoStatTablePosition;
    long switchMibPortInfoStatTablePort;

    /* Column values */
    char switchMibPortInfoStatTablePortName[SWITCH_PORT_NAME_LEN];
    size_t switchMibPortInfoStatTablePortName_len;
    long switchMibPortInfoStatTableLinkState;
    long switchMibPortInfoStatTableLinkEnable;
    long old_switchMibPortInfoStatTableLinkEnable;
    long switchMibPortInfoStatTableAutoNeg;
    long old_switchMibPortInfoStatTableAutoNeg;
    long switchMibPortInfoStatTableLinkSpeed;
    long switchMibPortInfoStatTableDuplex;

    /* Illustrate using a simple linked list */
    int   valid;
    struct switchMibPortInfoStatTable_entry *next;
};

struct switchMibPortInfoStatTable_entry  *switchMibPortInfoStatTable_head = NULL;
static int switchMibPortInfoStatTable_count = 0;
static void *lstlock = NULL;

/** Initializes the switchMibPortInfoStatTable module */
void init_switchMibPortInfoStatTable(void)
{
	lstlock = mutex_open(NULL);
	if (lstlock == NULL) {
		LOGERROR("%s: mutex_open failed", __func__);
		return;
	}
  /* here we initialize all the tables we're planning on supporting */
    initialize_table_switchMibPortInfoStatTable();
}

void exit_switchMibPortInfoStatTable(void)
{
	switchMibPortInfoStatTable_clear();

	if (lstlock) 
		mutex_close(lstlock);
}

/** Initialize the switchMibPortInfoStatTable table by defining its contents and how it's structured */
void
initialize_table_switchMibPortInfoStatTable(void)
{
    const oid switchMibPortInfoStatTable_oid[] = {1,3,6,1,4,1,8990,1,5,1};
    const size_t switchMibPortInfoStatTable_oid_len   = OID_LENGTH(switchMibPortInfoStatTable_oid);
    netsnmp_handler_registration    *reg;
    netsnmp_iterator_info           *iinfo;
    netsnmp_table_registration_info *table_info;

    DEBUGMSGTL(("switchMibPortInfoStatTable:init", "initializing table switchMibPortInfoStatTable\n"));

    reg = netsnmp_create_handler_registration(
              "switchMibPortInfoStatTable",     switchMibPortInfoStatTable_handler,
              switchMibPortInfoStatTable_oid, switchMibPortInfoStatTable_oid_len,
              HANDLER_CAN_RWRITE);

    table_info = SNMP_MALLOC_TYPEDEF( netsnmp_table_registration_info );
    netsnmp_table_helper_add_indexes(table_info,
                           ASN_IPADDRESS,  /* index: switchMibPortInfoStatTablePosition */
                           ASN_INTEGER,  /* index: switchMibPortInfoStatTablePort */
                           0);
    table_info->min_column = COLUMN_SWITCHMIBPORTINFOSTATTABLEPOSITION;
    table_info->max_column = COLUMN_SWITCHMIBPORTINFOSTATTABLEDUPLEX;
    
    iinfo = SNMP_MALLOC_TYPEDEF( netsnmp_iterator_info );
    iinfo->get_first_data_point = switchMibPortInfoStatTable_get_first_data_point;
    iinfo->get_next_data_point  = switchMibPortInfoStatTable_get_next_data_point;
    iinfo->table_reginfo        = table_info;
    
    netsnmp_register_table_iterator( reg, iinfo );

    /* Initialise the contents of the table here */
}

/* create a new row in the (unsorted) table */
struct switchMibPortInfoStatTable_entry *
switchMibPortInfoStatTable_createEntry(
                 in_addr_t  switchMibPortInfoStatTablePosition,
                 long switchMibPortInfoStatTablePort) {
    struct switchMibPortInfoStatTable_entry *entry;

    entry = SNMP_MALLOC_TYPEDEF(struct switchMibPortInfoStatTable_entry);
    if (!entry)
        return NULL;

    entry->switchMibPortInfoStatTablePosition = switchMibPortInfoStatTablePosition;
    entry->switchMibPortInfoStatTablePort = switchMibPortInfoStatTablePort;
    entry->next = switchMibPortInfoStatTable_head;
    switchMibPortInfoStatTable_head = entry;
	switchMibPortInfoStatTable_count++;

    return entry;
}

/* remove a row from the table */
void
switchMibPortInfoStatTable_removeEntry( struct switchMibPortInfoStatTable_entry *entry ) {
    struct switchMibPortInfoStatTable_entry *ptr, *prev;

    if (!entry)
        return;    /* Nothing to remove */

    for ( ptr  = switchMibPortInfoStatTable_head, prev = NULL;
          ptr != NULL;
          prev = ptr, ptr = ptr->next ) {
        if ( ptr == entry )
            break;
    }
    if ( !ptr )
        return;    /* Can't find it */

    if ( prev == NULL )
        switchMibPortInfoStatTable_head = ptr->next;
    else
        prev->next = ptr->next;

    SNMP_FREE( entry );   /* XXX - release any other internal resources */
	switchMibPortInfoStatTable_count--;
}


/* Example iterator hook routines - using 'get_next' to do most of the work */
netsnmp_variable_list *
switchMibPortInfoStatTable_get_first_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    *my_loop_context = switchMibPortInfoStatTable_head;
    return switchMibPortInfoStatTable_get_next_data_point(my_loop_context, my_data_context,
                                    put_index_data,  mydata );
}

netsnmp_variable_list *
switchMibPortInfoStatTable_get_next_data_point(void **my_loop_context,
                          void **my_data_context,
                          netsnmp_variable_list *put_index_data,
                          netsnmp_iterator_info *mydata)
{
    struct switchMibPortInfoStatTable_entry *entry = (struct switchMibPortInfoStatTable_entry *)*my_loop_context;
    netsnmp_variable_list *idx = put_index_data;

	mutex_lock(lstlock);

    if ( entry ) {
        snmp_set_var_typed_integer( idx, ASN_IPADDRESS, entry->switchMibPortInfoStatTablePosition );
        idx = idx->next_variable;
        snmp_set_var_typed_integer( idx, ASN_INTEGER, entry->switchMibPortInfoStatTablePort );
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

int switchMibPortInfoStatTable_compare(struct switchMibPortInfoStatTable_entry *entry, in_addr_t po)
{
	if (!entry)
		return -1;

	if (entry->switchMibPortInfoStatTablePosition == po)
		return 0;

	return -1;
}

DEFINE_TABLE_ENTRY_DELETE_FUNC(switchMibPortInfoStatTable) 
DEFINE_TABLE_DELETE_FUNC(switchMibPortInfoStatTable) 

struct switchMibPortInfoStatTable_entry *switchMibPortInfoStatTable_findEntry(in_addr_t po, int port)
{
	struct switchMibPortInfoStatTable_entry *entry = switchMibPortInfoStatTable_head;

	while (entry) {
		if (entry->switchMibPortInfoStatTablePosition == po &&
				entry->switchMibPortInfoStatTablePort == port) {
			return entry;
		}
		entry = entry->next;
	}
	return NULL;
}

static void switchMibPortInfoStatTable_updateEntry(struct switchMibPortInfoStatTable_entry *entry, 
		in_addr_t po, struct sw_port_status *ptr)
{
	entry->switchMibPortInfoStatTablePosition   = po;
	entry->switchMibPortInfoStatTablePort = ptr->port;
	strcpy(entry->switchMibPortInfoStatTablePortName, ptr->portname);
	entry->switchMibPortInfoStatTablePortName_len = strlen(ptr->portname);
	entry->switchMibPortInfoStatTableLinkState  = ptr->lks;
	entry->switchMibPortInfoStatTableLinkEnable = ptr->pwrdn;
	entry->switchMibPortInfoStatTableAutoNeg    = ptr->autoneg;
	entry->switchMibPortInfoStatTableLinkSpeed  = ptr->speed;
	entry->switchMibPortInfoStatTableDuplex     = ptr->duplex;
}

int switchMibPortInfoStatTable_update(struct nm_lkaddr_info po, int cnt, void *data)
{
	struct switchMibPortInfoStatTable_entry *entry = NULL;
	struct sw_port_status *ps = NULL;
	in_addr_t position;	
	int port;
	unsigned int i;

	if (!data) {
		LOGERROR("%s: data is NULL", __func__);
		return -1;
	}
	
	if (switchMibPortInfoStatTable_count >= SWITCHMAXITEMS) 
	{
		LOGERROR("switchMibStat:item over the max row %d", SWITCHMAXITEMS);
		return -1;
	}
	
	mutex_lock(lstlock);
	/* 1: delete board 2: add new data */
	position = (po.rack << 24) | (po.shelf << 16) | (po.slot << 8) | po.subslot;
//	switchMibPortInfoStatTable_delete(position);

	ps = (struct sw_port_status *)data;
	for (i = 0; i < cnt; i++) {
		port = ps->port;
		entry = switchMibPortInfoStatTable_findEntry(position, port);
		if (entry == NULL) {
			entry = switchMibPortInfoStatTable_createEntry(position, port);
			if (!entry) {
				LOGERROR("%s: createEntry failed", __func__);
				continue;
			}
		}
		switchMibPortInfoStatTable_updateEntry(entry, position, ps);
		ps++;
	}

	mutex_unlock(lstlock);
	return 0;
}

DEFINE_TABLE_ENTRY_DELETE_INVALID_FUNC(switchMibPortInfoStatTable) 

void switchMibPortInfoStatTable_check_and_clear(void)
{
	struct switchMibPortInfoStatTable_entry *entry = switchMibPortInfoStatTable_head;
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
	cnt += boardsInfoTable_getboards(BOARD_TYPE_MPCB_V3, &po[cnt]);
	cnt += boardsInfoTable_getboards(BOARD_TYPE_MACB_V2, &po[cnt]);
	for (i = 0; i < cnt; i++) {
		position = (po[i].rack << 24) | (po[i].shelf << 16) | (po[i].slot << 8) | po[i].subslot;
		entry = switchMibPortInfoStatTable_head;
		while (entry) {
			if (entry->switchMibPortInfoStatTablePosition == position) {
				entry->valid = 1;
			}
			entry = entry->next;
		}
	}

	/* clear invalid */
	switchMibPortInfoStatTable_head =
		switchMibPortInfoStatTable_del_invalid_entrys(switchMibPortInfoStatTable_head);

	mutex_unlock(lstlock);
}

void switchMibPortInfoStatTable_clear(void)
{

	mutex_lock(lstlock);

	while (switchMibPortInfoStatTable_head) {
		switchMibPortInfoStatTable_removeEntry(switchMibPortInfoStatTable_head);
	}
	switchMibPortInfoStatTable_head = NULL;
	switchMibPortInfoStatTable_count = 0;

	mutex_unlock(lstlock);
}

/** handles requests for the switchMibPortInfoStatTable table */
int
switchMibPortInfoStatTable_handler(
    netsnmp_mib_handler               *handler,
    netsnmp_handler_registration      *reginfo,
    netsnmp_agent_request_info        *reqinfo,
    netsnmp_request_info              *requests) {

    netsnmp_request_info       *request;
    netsnmp_table_request_info *table_info;
    struct switchMibPortInfoStatTable_entry          *table_entry;
	int ret;

    DEBUGMSGTL(("switchMibPortInfoStatTable:handler", "Processing request (%d)\n", reqinfo->mode));

	mutex_lock(lstlock);

    switch (reqinfo->mode) {
        /*
         * Read-support (also covers GetNext requests)
         */
    case MODE_GET:
        for (request=requests; request; request=request->next) {
            table_entry = (struct switchMibPortInfoStatTable_entry *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
    
            if ( !table_entry ) {
                continue;
            }
            switch (table_info->colnum) {
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEPOSITION:
                snmp_set_var_typed_integer( request->requestvb, ASN_IPADDRESS,
                                            table_entry->switchMibPortInfoStatTablePosition);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEPORT:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                          table_entry->switchMibPortInfoStatTablePort);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEPORTNAME:
                snmp_set_var_typed_value( request->requestvb, ASN_OCTET_STR,
                                          table_entry->switchMibPortInfoStatTablePortName,
                                          table_entry->switchMibPortInfoStatTablePortName_len);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLELINKSTATE:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->switchMibPortInfoStatTableLinkState);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLELINKENABLE:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->switchMibPortInfoStatTableLinkEnable);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEAUTONEG:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->switchMibPortInfoStatTableAutoNeg);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLELINKSPEED:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->switchMibPortInfoStatTableLinkSpeed);
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEDUPLEX:
                snmp_set_var_typed_integer( request->requestvb, ASN_INTEGER,
                                            table_entry->switchMibPortInfoStatTableDuplex);
                break;
            default:
                netsnmp_set_request_error(reqinfo, request,
                                          SNMP_NOSUCHOBJECT);
                break;
            }
        }
        break;

        /*
         * Write-support
         */
    case MODE_SET_RESERVE1:
        for (request=requests; request; request=request->next) {
            table_entry = (struct switchMibPortInfoStatTable_entry *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
    
            switch (table_info->colnum) {
            case COLUMN_SWITCHMIBPORTINFOSTATTABLELINKENABLE:
                /* or possibly 'netsnmp_check_vb_int_range' */
                ret = netsnmp_check_vb_int( request->requestvb );
                if ( ret != SNMP_ERR_NOERROR ) {
                    netsnmp_set_request_error( reqinfo, request, ret );
					mutex_unlock(lstlock);
                    return SNMP_ERR_NOERROR;
                }
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEAUTONEG:
                /* or possibly 'netsnmp_check_vb_int_range' */
                ret = netsnmp_check_vb_int( request->requestvb );
                if ( ret != SNMP_ERR_NOERROR ) {
                    netsnmp_set_request_error( reqinfo, request, ret );
					mutex_unlock(lstlock);
                    return SNMP_ERR_NOERROR;
                }
                break;
            default:
                netsnmp_set_request_error( reqinfo, request,
                                           SNMP_ERR_NOTWRITABLE );
				mutex_unlock(lstlock);
                return SNMP_ERR_NOERROR;
            }
        }
        break;

    case MODE_SET_RESERVE2:
        break;

    case MODE_SET_FREE:
        break;

    case MODE_SET_ACTION:
        for (request=requests; request; request=request->next) {
            table_entry = (struct switchMibPortInfoStatTable_entry *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
    
            switch (table_info->colnum) {
            case COLUMN_SWITCHMIBPORTINFOSTATTABLELINKENABLE:
                table_entry->old_switchMibPortInfoStatTableLinkEnable = table_entry->switchMibPortInfoStatTableLinkEnable;
                table_entry->switchMibPortInfoStatTableLinkEnable     = *request->requestvb->val.integer;
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEAUTONEG:
                table_entry->old_switchMibPortInfoStatTableAutoNeg = table_entry->switchMibPortInfoStatTableAutoNeg;
                table_entry->switchMibPortInfoStatTableAutoNeg     = *request->requestvb->val.integer;
                break;
            }
        }
        break;

    case MODE_SET_UNDO:
        for (request=requests; request; request=request->next) {
            table_entry = (struct switchMibPortInfoStatTable_entry *)
                              netsnmp_extract_iterator_context(request);
            table_info  =     netsnmp_extract_table_info(      request);
    
            switch (table_info->colnum) {
            case COLUMN_SWITCHMIBPORTINFOSTATTABLELINKENABLE:
                table_entry->switchMibPortInfoStatTableLinkEnable     = table_entry->old_switchMibPortInfoStatTableLinkEnable;
                table_entry->old_switchMibPortInfoStatTableLinkEnable = 0;
                break;
            case COLUMN_SWITCHMIBPORTINFOSTATTABLEAUTONEG:
                table_entry->switchMibPortInfoStatTableAutoNeg     = table_entry->old_switchMibPortInfoStatTableAutoNeg;
                table_entry->old_switchMibPortInfoStatTableAutoNeg = 0;
                break;
            }
        }
        break;

    case MODE_SET_COMMIT:
        break;
    }

	mutex_unlock(lstlock);
    return SNMP_ERR_NOERROR;
}
