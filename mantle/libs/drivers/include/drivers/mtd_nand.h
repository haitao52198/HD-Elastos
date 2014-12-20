/*
 * File      : mtd_nand.h
 * 
 * COPYRIGHT (C) 2006 - 2012, RT-Thread Development Team
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
 * Date           Author       Notes
 * 2011-12-05     Bernard      the first version
 * 2011-04-02     prife        add mark_badblock and check_block
 */

/*
 * COPYRIGHT (C) 2012, Shanghai Real Thread
 */

#ifndef __MTD_NAND_H__
#define __MTD_NAND_H__

#include <rtdevice.h>

struct rt_mtd_nand_driver_ops;
#define RT_MTD_NAND_DEVICE(device)	((struct rt_mtd_nand_device*)(device))

#define RT_MTD_EOK		0	/* NO error */
#define RT_MTD_EECC		1	/* ECC error */
#define RT_MTD_EBUSY		2	/* hardware busy */
#define RT_MTD_EIO		3	/* generic IO issue */
#define RT_MTD_ENOMEM		4	/* out of memory */
#define RT_MTD_ESRC		5	/* source issue */

struct rt_mtd_nand_device
{
	struct rt_device parent;

	UInt16 page_size;			/* The Page size in the flash */
	UInt16 oob_size;			/* Out of bank size */	
	UInt16 oob_free;           /* the free area in oob that flash driver not use */
	UInt16 plane_num; 			/* the number of plane in the NAND Flash */

	UInt32 pages_per_block;    /* The number of page a block */
    UInt16 block_total;

	UInt32 block_start;		/* The start of available block*/
	UInt32 block_end;			/* The end of available block */

	/* operations interface */
	const struct rt_mtd_nand_driver_ops* ops;
};

struct rt_mtd_nand_driver_ops
{
	Int32 (*read_id) (struct rt_mtd_nand_device* device);

	Int32 (*read_page)(struct rt_mtd_nand_device* device,
                          Int32 page,
                          UInt8* data, UInt32 data_len,
                          UInt8 * spare, UInt32 spare_len);

	Int32 (*write_page)(struct rt_mtd_nand_device * device,
                           Int32 page,
                           const UInt8 * data, UInt32 data_len,
                           const UInt8 * spare, UInt32 spare_len);
	Int32 (*move_page) (struct rt_mtd_nand_device *device, Int32 src_page, Int32 dst_page);						   

	Int32 (*erase_block)(struct rt_mtd_nand_device* device, UInt32 block);
	Int32 (*check_block)(struct rt_mtd_nand_device* device, UInt32 block);
	Int32 (*mark_badblock)(struct rt_mtd_nand_device* device, UInt32 block);
};

Int32 rt_mtd_nand_register_device(const char* name, struct rt_mtd_nand_device* device);

rt_inline UInt32 rt_mtd_nand_read_id(struct rt_mtd_nand_device* device)
{
	return device->ops->read_id(device);
}

rt_inline Int32 rt_mtd_nand_read(
	struct rt_mtd_nand_device* device,
	Int32 page,
	UInt8* data, UInt32 data_len,
	UInt8 * spare, UInt32 spare_len)
{
	return device->ops->read_page(device, page, data, data_len, spare, spare_len);
}

rt_inline Int32 rt_mtd_nand_write(
	struct rt_mtd_nand_device* device,
	Int32 page,
	const UInt8* data, UInt32 data_len,
	const UInt8 * spare, UInt32 spare_len)
{
	return device->ops->write_page(device, page, data, data_len, spare, spare_len);
}

rt_inline Int32 rt_mtd_nand_move_page(struct rt_mtd_nand_device* device,
										 Int32 src_page, Int32 dst_page)
{
	return device->ops->move_page(device, src_page, dst_page);
}

rt_inline Int32 rt_mtd_nand_erase_block(struct rt_mtd_nand_device* device, UInt32 block)
{
	return device->ops->erase_block(device, block);
}

rt_inline Int32 rt_mtd_nand_check_block(struct rt_mtd_nand_device* device, UInt32 block)
{
	return device->ops->check_block(device, block);
}

rt_inline Int32 rt_mtd_nand_mark_badblock(struct rt_mtd_nand_device* device, UInt32 block)
{
	return device->ops->mark_badblock(device, block);
}

#endif /* MTD_NAND_H_ */
