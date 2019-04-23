/*
 * (C) Copyright 2015
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * fpgaload.h - A description goes here.
 *
 */

#ifndef _HEAD_FPGALOAD_72440E65_43051CA4_18411ADE_H
#define _HEAD_FPGALOAD_72440E65_43051CA4_18411ADE_H

#if defined(EIPB_V2) || defined(MUCB_V1)
#include "mpc8315.h"

#define DEV_NAME		MPC8315_NAME

#define DEV_CFG_FPGA_START	MPC8315_CFG_FPGA_START
#define DEV_CFG_FPGA_STOP	MPC8315_CFG_FPGA_STOP
#define DEV_CFG_FPGA_LOAD	MPC8315_CFG_FPGA_LOAD
#define DEV_CFG_FPGA_DONE	MPC8315_CFG_FPGA_DONE

#elif defined(OI_AMC_V3) || defined(EI_AMC_V1) || defined(OIPC_V1) \
	|| defined(EIPC_V1) || defined(LBB_V1) || defined(OI_AMC_V4) \
    || defined(EIPC_V2) || defined(XSCB_V2) || defined(OTAP_V1) \
    || defined(ETAP_V1) || defined(XPPP_V2) || defined(LCEA_V1)
#include "cloves.h"

#define DEV_NAME		CLOVES_NAME

#define DEV_CFG_FPGA_START	CLOVES_CFG_FPGA_START
#define DEV_CFG_FPGA_STOP	CLOVES_CFG_FPGA_STOP
#define DEV_CFG_FPGA_LOAD	CLOVES_CFG_FPGA_LOAD
#define DEV_CFG_FPGA_DONE	CLOVES_CFG_FPGA_DONE

#endif

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__cplusplus)
}
#endif

#endif /* #ifndef _HEAD_FPGALOAD_72440E65_43051CA4_18411ADE_H */
