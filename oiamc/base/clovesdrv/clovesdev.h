/*
 * (C) Copyright 2014
 *  <www.raycores.com>
 *
 */

#ifndef	_CLOVES_DEV_H
#define _CLOVES_DEV_H

#include <linux/cdev.h>

#define GPIO_GREEN_BIT         (1 << 26)   /* gpio5 */
#define GPIO_WD_FEED_BIT       (1 << 21)   /* gpio10 */
#define GPIO_WD_EN_BIT         (1 << 20)   /* gpio11 */
#define GPIO_DIAL_1            (1 << 25)   /* gpio6 */
#define GPIO_DIAL_2            (1 << 24)   /* gpio7 */
#define GPIO_DIAL_3            (1 << 23)   /* gpio8 */
#define GPIO_DIAL_4            (1 << 22)   /* gpio9 */

#define FPGA_MAX_NUM		16
#define FPGA_CFGTYPE_GPIO	0x01
#define FPGA_CFGTYPE_CPLD	0x02

/*
 * General purpose I/O module
 */
typedef struct gpio_reg {
	u32 gpdir;	/* direction register 1: output 0: input */
	u32 gpodr;	/* open drain register */
	u32 gpdat;	/* data register */
	u32 gpier;	/* interrupt event register */
	u32 gpimr;	/* interrupt mask register */
	u32 gpicr;	/* external interrupt control register */
} gpiomap_t;

/*
 * cloves dev struct
 */
struct fpga_info {
	unsigned char	index;

	unsigned char	cfg_type;
	/* pins */
	unsigned char	cfg_cs;
	unsigned char	cfg_rdwr;
	unsigned char	cfg_program;
	unsigned char	cfg_init;
	unsigned char	cfg_done;

	/* The below two items used only in CPLD way */
	unsigned char  *cfg_ctrl_addr;
	unsigned char  *cfg_load_addr;

	unsigned int	mif_phys;
	void	       *mif_virt;
	unsigned int	mif_size;
};

typedef struct cloves {
	int		major;
	struct cdev	cdev;
	struct class   *class;

	void           *cpldp;
	gpiomap_t      *gpiop;

	unsigned int	ledfreq;

	struct fpga_info fpgas[FPGA_MAX_NUM];
	unsigned int	 num_fpgas;
} cloves_dev_t;

extern cloves_dev_t *cloves_devp;

#endif

