/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * reg.h - A description goes here.
 *
 */

#ifndef _HEAD_REG_70E60E2D_283E2290_46E45CC5_H
#define _HEAD_REG_70E60E2D_283E2290_46E45CC5_H

#if defined(EIPB_V2) || defined(MUCB_V1)
#include "mpc8315.h"

#define DEV_NAME		MPC8315_NAME

#define DEV_GET_FPGA_REG	MPC8315_GET_FPGA_REG
#define DEV_SET_FPGA_REG	MPC8315_SET_FPGA_REG
#define DEV_GET_FPGA_NUM	MPC8315_GET_FPGA_NUM
#define DEV_GET_FPGA_INFO	MPC8315_GET_FPGA_INFO

#elif defined(OI_AMC_V3) || defined(EI_AMC_V1) || defined(OIPC_V1) \
	|| defined(EIPC_V1) || defined(LBB_V1) || defined(OI_AMC_V4)  \
    || defined(EIPC_V2) || defined(XSCB_V2) || defined(OTAP_V1) \
    || defined(ETAP_V1) || defined(XPPP_V2) || defined(LCEA_V1)
#include "cloves.h"

#define DEV_NAME		CLOVES_NAME

#define DEV_GET_FPGA_REG	CLOVES_GET_FPGA_REG
#define DEV_SET_FPGA_REG	CLOVES_SET_FPGA_REG
#define DEV_GET_FPGA_NUM	CLOVES_GET_FPGA_NUM
#define DEV_GET_FPGA_INFO	CLOVES_GET_FPGA_INFO

#elif defined(MACB_V3)

#include "cloves.h"

#define DEV_NAME		CLOVES_NAME

#define DEV_GET_CPLD_REG    CLOVES_GET_CPLD_REG
#define DEV_SET_CPLD_REG    CLOVES_SET_CPLD_REG
#define DEV_GET_CPLD_INFO   CLOVES_GET_CPLD_INFO

#endif

#if defined(EI_AMC_V1) || defined(LCEA_V1)
#define DEV_GET_E1PHY_INFO CLOVES_GET_E1PHY_INFO
#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_REG_70E60E2D_283E2290_46E45CC5_H */
