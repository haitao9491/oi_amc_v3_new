/*
 * (C) Copyright 2014
 *  <www.raycores.com>
 *
 */

#ifndef	_CLOVES_REG_H
#define _CLOVES_REG_H

#define CLOVES_NAME            "cloves"

#define CLOVES_IOC_MAGIC       'M'
#define CLOVES_CTL_WATCHDOG    _IOWR(CLOVES_IOC_MAGIC, 0x1, void *)
#define CLOVES_CTL_GREEN       _IOWR(CLOVES_IOC_MAGIC, 0x2, void *)
#define CLOVES_GET_DIALCODE    _IOWR(CLOVES_IOC_MAGIC, 0x3, void *)
#define CLOVES_CFG_FPGA_START  _IOWR(CLOVES_IOC_MAGIC, 0x4, void *)
#define CLOVES_CFG_FPGA_STOP   _IOWR(CLOVES_IOC_MAGIC, 0x5, void *)
#define CLOVES_CFG_FPGA_LOAD   _IOWR(CLOVES_IOC_MAGIC, 0x6, void *)
#define CLOVES_CFG_FPGA_DONE   _IOWR(CLOVES_IOC_MAGIC, 0x7, void *)
#define CLOVES_GET_FPGA_REG    _IOWR(CLOVES_IOC_MAGIC, 0x8, void *)
#define CLOVES_SET_FPGA_REG    _IOWR(CLOVES_IOC_MAGIC, 0x9, void *)
#define CLOVES_GET_FPGA_NUM    _IOWR(CLOVES_IOC_MAGIC, 0xA, void *)
#define CLOVES_GET_FPGA_INFO   _IOWR(CLOVES_IOC_MAGIC, 0xB, void *)
#define CLOVES_IOC_NUMBER      0xC


struct fpga_cfg_info {
	unsigned char	index;		/* start @ 0 */
	unsigned char  *data;		/* bitstream */
	unsigned int	len;
};

struct fpga_reg {
	unsigned int	addr;
	unsigned int	value;
};

struct fpga_info_entry {
	unsigned int	phys;
	unsigned int	size;
};

#endif

