/*
 * (C) Copyright 2015
 *  <www.raycores.com>
 *
 */

#ifndef	_ESN_IOCTL_COMMAND_H_
#define _ESN_IOCTL_COMMAND_H_

#define ESNAPP_DEV_NAME            "fpgadrv_esn"

#define ESNAPP_IOC_MAGIC           'N'
/* version info */
#define ESNAPP_RD_VERSION          _IOWR(ESNAPP_IOC_MAGIC, 0x01, void *)
/* board start opt */
#define ESNAPP_RD_DDR_INIT_STAT    _IOWR(ESNAPP_IOC_MAGIC, 0x02, void *)
#define ESNAPP_RD_BD_START_OPT     _IOWR(ESNAPP_IOC_MAGIC, 0x03, void *)
#define ESNAPP_WR_BD_START_OPT     _IOWR(ESNAPP_IOC_MAGIC, 0x04, void *)
/* board cfg info */
#define ESNAPP_RD_BD_CFG_INFO      _IOWR(ESNAPP_IOC_MAGIC, 0x05, void *)
#define ESNAPP_WR_BD_CFG_INFO      _IOWR(ESNAPP_IOC_MAGIC, 0x06, void *)
/* logic and physical channel check */
#define ESNAPP_RD_VOICE_CFG        _IOWR(ESNAPP_IOC_MAGIC, 0x07, void *)  /* not used */
#define ESNAPP_WR_VOICE_CFG        _IOWR(ESNAPP_IOC_MAGIC, 0x08, void *)
#define ESNAPP_RD_PL_SCAN_RST      _IOWR(ESNAPP_IOC_MAGIC, 0x09, void *)  /* not used */
#define ESNAPP_WR_PL_SCAN_RST      _IOWR(ESNAPP_IOC_MAGIC, 0x0a, void *)
#define ESNAPP_RD_CFG_RESULT       _IOWR(ESNAPP_IOC_MAGIC, 0x0b, void *)
/* board run info */
#define ESNAPP_RD_BD_RUN_INFO      _IOWR(ESNAPP_IOC_MAGIC, 0x0c, void *)
/* 64K channel transfer */
#define ESNAPP_RD_64K_CH_TRAN      _IOWR(ESNAPP_IOC_MAGIC, 0x0d, void *)  /* not used */
#define ESNAPP_WR_64K_CH_TRAN      _IOWR(ESNAPP_IOC_MAGIC, 0x0e, void *)
#define ESNAPP_RD_2M_CH_TRAN       _IOWR(ESNAPP_IOC_MAGIC, 0x0f, void *)  /* not used */
#define ESNAPP_WR_2M_CH_TRAN       _IOWR(ESNAPP_IOC_MAGIC, 0x10, void *)
/* e1phy port config */
#define ESNAPP_RD_E1PHY_STAT       _IOWR(ESNAPP_IOC_MAGIC, 0x11, void *)
#define ESNAPP_WR_E1PHY_STAT       _IOWR(ESNAPP_IOC_MAGIC, 0x12, void *)
#define ESNAPP_START_UP			   _IOWR(ESNAPP_IOC_MAGIC, 0x13, void *)

#define ESNAPP_IOC_NUMBER          0x14

#define ESNAPP_TIMESLOT_MASK       0x1f   /* 5bit */
#define ESNAPP_PCMID_MASK          0x7ff  /* 11bit */
#endif
