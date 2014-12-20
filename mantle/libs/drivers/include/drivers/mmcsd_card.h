/*
 * File      : mmcsd_card.h
 * 
 * COPYRIGHT (C) 2006, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author		Notes
 * 2011-07-25     weety		first version
 */

#ifndef __MMCSD_CARD_H__
#define __MMCSD_CARD_H__

#include <drivers/mmcsd_host.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_SCR_BUS_WIDTH_1	(1 << 0)
#define SD_SCR_BUS_WIDTH_4	(1 << 2)

struct rt_mmcsd_cid {
	UInt8  mid;       /* ManufacturerID */
	UInt8  prv;       /* Product Revision */
	UInt16 oid;       /* OEM/Application ID */
	UInt32 psn;       /* Product Serial Number */
	UInt8  pnm[5];    /* Product Name */
	UInt8  reserved1;/* reserved */
	UInt16 mdt;       /* Manufacturing Date */
	UInt8  crc;       /* CID CRC */
	UInt8  reserved2;/* not used, always 1 */
};

struct rt_mmcsd_csd {
	UInt8		csd_structure;	/* CSD register version */
	UInt8		taac;
	UInt8		nsac;
	UInt8		tran_speed;	/* max data transfer rate */
	UInt16		card_cmd_class;	/* card command classes */
	UInt8		rd_blk_len;	/* max read data block length */
	UInt8		rd_blk_part;
	UInt8		wr_blk_misalign;
	UInt8		rd_blk_misalign;
	UInt8		dsr_imp;	/* DSR implemented */
	UInt8		c_size_mult;	/* CSD 1.0 , device size multiplier */
	UInt32		c_size;		/* device size */
	UInt8		r2w_factor;
	UInt8		wr_blk_len;	/* max wtire data block length */
	UInt8		wr_blk_partial;
	UInt8		csd_crc;
	
};

struct rt_sd_scr {
	UInt8		sd_version;
	UInt8		sd_bus_widths;
};

struct rt_sdio_cccr {
	UInt8		sdio_version;
	UInt8		sd_version;
	UInt8		direct_cmd:1,     /*  Card Supports Direct Commands during data transfer
	                                               only SD mode, not used for SPI mode */
				multi_block:1,    /*  Card Supports Multi-Block */
				read_wait:1,      /*  Card Supports Read Wait
				                       only SD mode, not used for SPI mode */
				suspend_resume:1, /*  Card supports Suspend/Resume
				                       only SD mode, not used for SPI mode */
				s4mi:1,            /* generate interrupts during a 4-bit 
				                      multi-block data transfer */
				e4mi:1,            /*  Enable the multi-block IRQ during 
				                       4-bit transfer for the SDIO card */
				low_speed:1,      /*  Card  is  a  Low-Speed  card */
				low_speed_4:1;    /*  4-bit support for Low-Speed cards */

	UInt8		bus_width:1,     /* Support SDIO bus width, 1:4bit, 0:1bit */
				cd_disable:1,    /*  Connect[0]/Disconnect[1] the 10K-90K ohm pull-up 
				                     resistor on CD/DAT[3] (pin 1) of the card */
				power_ctrl:1,    /* Support Master Power Control */
				high_speed:1;    /* Support High-Speed  */
				
				
};

struct rt_sdio_cis {
	UInt16		manufacturer;
	UInt16		product;
	UInt16		func0_blk_size;
	UInt32		max_tran_speed;
};

/*
 * SDIO function CIS tuple (unknown to the core)
 */
struct rt_sdio_function_tuple {
	struct rt_sdio_function_tuple *next;
	UInt8 code;
	UInt8 size;
	UInt8 *data;
};

struct rt_sdio_function;
typedef void (rt_sdio_irq_handler_t)(struct rt_sdio_function *);

/*
 * SDIO function devices
 */
struct rt_sdio_function {
	struct rt_mmcsd_card		*card;		/* the card this device belongs to */
	rt_sdio_irq_handler_t	*irq_handler;	/* IRQ callback */
	UInt8		num;		/* function number */

	UInt8		func_code;   /*  Standard SDIO Function interface code  */
	UInt16		manufacturer;		/* manufacturer id */
	UInt16		product;		/* product id */

	UInt32		max_blk_size;	/* maximum block size */
	UInt32		cur_blk_size;	/* current block size */

	UInt32		enable_timeout_val; /* max enable timeout in msec */

	struct rt_sdio_function_tuple *tuples;
};

#define SDIO_MAX_FUNCTIONS		7



struct rt_mmcsd_card {
	struct rt_mmcsd_host *host;
	UInt32	rca;		/* card addr */
	UInt32	resp_cid[4];	/* card CID register */
	UInt32	resp_csd[4];	/* card CSD register */
	UInt32	resp_scr[2];	/* card SCR register */

	UInt16	tacc_clks;	/* data access time by ns */
	UInt32	tacc_ns;	/* data access time by clk cycles */
	UInt32	max_data_rate;	/* max data transfer rate */
	UInt32	card_capacity;	/* card capacity, unit:KB */
	UInt32	card_blksize;	/* card block size */
	UInt16	card_type;
#define CARD_TYPE_MMC                   0 /* MMC card */
#define CARD_TYPE_SD                    1 /* SD card */
#define CARD_TYPE_SDIO                  2 /* SDIO card */
#define CARD_TYPE_SDIO_COMBO            3 /* SD combo (IO+mem) card */

	UInt16 flags;
#define CARD_FLAG_HIGHSPEED  (1 << 0)   /* SDIO bus speed 50MHz */
#define CARD_FLAG_SDHC       (1 << 1)   /* SDHC card */
#define CARD_FLAG_SDXC       (1 << 2)   /* SDXC card */

	struct rt_sd_scr	scr;
	struct rt_mmcsd_csd	csd;
	UInt32     hs_max_data_rate;  /* max data transfer rate in high speed mode */

	UInt8      sdio_function_num;	/* totol number of SDIO functions */
	struct rt_sdio_cccr    cccr;  /* common card info */
	struct rt_sdio_cis     cis;  /* common tuple info */
	struct rt_sdio_function	*sdio_function[SDIO_MAX_FUNCTIONS + 1]; /* SDIO functions (devices) */

};

#ifdef __cplusplus
}
#endif

#endif
