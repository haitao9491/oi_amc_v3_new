/*
 * (C) Copyright 2015
 * <xiumei.wang@raycores.com>
 *
 * nm_typdef.h - A description goes here.
 *
 */

#ifndef _HEAD_NMTYPEDEF_H
#define _HEAD_NMTYPEDEF_H

/* some misc define */
#define NMSERVER_LPORT		28000
#define NMSERVER_SPORT		28002

#define NMSERVER_LTYPE      0xc1
#define NMSERVER_STYPE      0xc2
#define IP_BUF_LEN          32
#define PKT_LEN             sizeof(pkt_hdr)	
#define NMPKT_LEN           sizeof(nmpkt_hdr)

/* component define */
#define NM_COMPONENT_E1PHY        0x00000001
#define NM_COMPONENT_SDHPHY       0x00000002
#define NM_COMPONENT_POSPHY       0x00000004
#define NM_COMPONENT_FPGA         0x00000008
#define NM_COMPONENT_SWITCH       0x00000010
#define NM_COMPONENT_TILE         0x00000020

/* module type define */
#define NM_MODULE_GLB_INFO         0x00     /* global info */
#define NM_MODULE_E1PHY            0x01     /* e1 phy, ex: ds26528 */ 
#define NM_MODULE_SDHPHY           0x02     /* sdh phy, ex: pm8310 */
#define NM_MODULE_POSPHY           0x03     /* pos phy, ex: pm5354 */
#define NM_MODULE_FPGA             0x04     /* fpga */
#define NM_MODULE_SWITCH           0x05
#define NM_MODULE_TILE             0x06     /* tile */
#define NM_MODULE_UNKNOWN          0xff

/* global module cmd */
#define NM_CMD_GET_BOARD_INFO      0x01
#define NM_CMD_GET_NTP_TIME        0x02

/* phy module layer cmd */
#define NM_CMD_GET_E1PHY_STATUS    0x01
#define NM_CMD_TRAP_E1PHY_LSCHG    0x02     /* trap link status change */

/* fpga module cmd */
#define NM_CMD_GET_FPGA_INFO       0x01
#define NM_CMD_GET_FPGA_STATUS     0x02
#define NM_CMD_GET_FPGA_STAT       0x03
#define NM_CMD_GET_FPGA_FLOW_STAT  0x04     /* flow statistics */

/* switch module cmd */
#define NM_CMD_GET_SW_PORT_STATUS  0x02     /* get switch port status info */
#define NM_CMD_GET_SW_PORT_STAT    0x03     /* get switch port statistics info */
#define NM_CMD_TRAP_SW_PORT_LSCHG  0x04     /* trap switch port status change */
#define NM_CMD_SET_SW_PORT_AUTONEG 0x05     /* set switch port autoneg */
#define NM_CMD_SET_SW_PORT_PWRDN   0x06     /* set switch port power down */
#define NM_CMD_UNKNOWN             0xff

/* nm cmd response return micro define */
#define NM_RET_OK                  0x00
#define NM_RET_UNKNOWN_ERR         0x01
#define NM_RET_UNSUPPORT_ERR       0x02
#define NM_RET_PKT_FORMAT_ERR      0x03
#define NM_RET_TIMEOUT             0x04

/* board active state */
#define NM_BOARD_ACTIVE     1
#define NM_BOARD_INACTIVE   0

/* shelf type define */
#define NM_SHELF_3U_2SLOT          0x01
#define NM_SHELF_4U_4SLOT          0x02
#define NM_SHELF_6U_6SLOT          0x03
#define NM_SHELF_14U_14SLOT        0x04
#define NM_SHELF_15U_14SLOT        0x05
#define NM_SHELF_5U_6SLOT          0x06
#define NM_SHELF_16U_14SLOT        0x07

#endif

