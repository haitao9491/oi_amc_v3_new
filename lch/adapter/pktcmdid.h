/*
 *
 * pktcmdid.h - A brief description goes here.
 *
 */

#ifndef _HEAD_PKTCMDID_56F0F29F_2142A804_6DD53C47_H
#define _HEAD_PKTCMDID_56F0F29F_2142A804_6DD53C47_H

#define CMD_GET_HW_MODEL             0x2001
#define CMD_GET_HW_VERSION           0x2002
#define CMD_GET_SW_VERSION           0x2003
#define CMD_GET_HW_SERIAL_NUM        0x2004
#define CMD_GET_HW_DESCRIPTION       0x2005
#define CMD_SET_HW_DESCRIPTION       0x2006
#define CMD_GET_ETHER_INFO           0x2011
#define CMD_SET_ETHER_INFO           0x2012
#define CMD_GET_MAC                  0x2013
#define CMD_SET_MAC                  0x2014
#define CMD_GET_SERVER_IP            0x2015
#define CMD_SET_SERVER_IP            0x2016
#define CMD_GET_GATEWAY_IP           0x2017
#define CMD_SET_GATEWAY_IP           0x2018
#define CMD_GET_SYS_TIME             0x2021
#define CMD_SET_SYS_TIME             0x2022
#define CMD_GET_NTP_MODE             0x2023
#define CMD_SET_NTP_MODE             0x2024
#define CMD_GET_CLK_TYPE             0x2031
#define CMD_SET_CLK_TYPE             0x2032
#define CMD_GET_PORTS                0x2041
#define CMD_GET_GLOBAL_PORT_CFG      0x2042
#define CMD_SET_GLOBAL_PORT_CFG      0x2043
#define CMD_GET_PORT_CFG             0x2044
#define CMD_SET_PORT_CFG             0x2045
#define CMD_DEL_PORT_CFG             0x2046
#define CMD_DOWNLOAD_PORT_CFGFILE    0x2047
#define CMD_UPLOAD_PORT_CFGFILE      0x2048
#define CMD_DOWNLOAD_CH_CFGFILE      0x2051
#define CMD_UPLOAD_CH_CFGFILE        0x2052
#define CMD_GET_DEV_ID               0x2061
#define CMD_SET_DEV_ID               0x2062
#define CMD_GET_PACK_TYPE            0x2063
#define CMD_SET_PACK_TYPE            0x2064
#define CMD_GET_DEV_MODE             0x2065
#define CMD_SET_DEV_MODE             0x2066
#define CMD_GET_PORT_SDH_MATRIX_CFG  0x2071
#define CMD_SET_PORT_SDH_MATRIX_CFG  0x2072
#define CMD_GET_PORT_MATRIX_CFG      0x2073
#define CMD_SET_PORT_MATRIX_CFG      0x2074
#define CMD_DOWNLOAD_MATRIX_CFGFILE  0x2075
#define CMD_UPLOAD_MATRIX_CFGFILE    0x2076
#define CMD_GAIN_SCAN                0x2101
#define CMD_CHAN_SCAN                0x2102
#define CMD_GET_GLOBAL_BERT_CFG      0x2111
#define CMD_SET_GLOBAL_BERT_CFG      0x2112
#define CMD_GET_BERT_CFG             0x2113
#define CMD_SET_BERT_CFG             0x2114
#define CMD_DEL_BERT_CFG             0x2115
#define CMD_START_PRBS               0x2116
#define CMD_STOP_PRBS                0x2117
#define CMD_INSERT_ERROR             0x2118
#define CMD_SYS_REBOOT               0x2121
#define CMD_SYS_START                0x2123
#define CMD_SYS_STOP                 0x2124
#define CMD_SYS_FIRMWARE_UPGRADE     0x2122
#define CMD_GET_SYS_RES_REP          0x2131
#define CMD_GET_LINE_STAT_INFO       0x2141
#define CMD_CLR_LINE_STAT_INFO       0x2142
#define CMD_GET_PORT_STAT_INFO       0x2151
#define CMD_CLR_PORT_STAT_INFO       0x2152
#define CMD_START_PFM_TEST           0x2153
#define CMD_GET_PORT_PFM_RESULT      0x2154
#define CMD_STOP_PFM_TEST            0x2155
#define CMD_GET_CH_CFG               0x2161
#define CMD_GET_CH_STAT_INFO         0x2162
#define CMD_CLR_CH_STAT_INFO         0x2163
#define CMD_GET_RAW_DATA             0x2300
#define CMD_GET_RT_VOICE             0x2301
#define CMD_GET_CALL_TRACE           0x2302

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_PKTCMDID_56F0F29F_2142A804_6DD53C47_H */
