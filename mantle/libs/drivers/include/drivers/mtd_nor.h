/*
 * File      : mtd_nor.h
 * 
 * COPYRIGHT (C) 2012, Shanghai Real-Thread Technology Co., Ltd
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
 * 2012-5-30     Bernard      the first version
 */

#ifndef __MTD_NOR_H__
#define __MTD_NOR_H__

#include <rtdevice.h>

struct rt_mtd_nor_driver_ops;
#define RT_MTD_NOR_DEVICE(device)	((struct rt_mtd_nor_device*)(device))

struct rt_mtd_nor_device
{
	struct rt_device parent;

	UInt32 block_size;			/* The Block size in the flash */
	UInt32 block_start;		/* The start of available block*/
	UInt32 block_end;			/* The end of available block */

	/* operations interface */
	const struct rt_mtd_nor_driver_ops* ops;
};

struct rt_mtd_nor_driver_ops
{
	Int32 (*read_id) (struct rt_mtd_nor_device* device);

	UInt32 (*read)    (struct rt_mtd_nor_device* device, Int32 offset, UInt8* data, UInt32 length);
	UInt32 (*write)   (struct rt_mtd_nor_device* device, Int32 offset, const UInt8* data, UInt32 length);

	Int32 (*erase_block)(struct rt_mtd_nor_device* device, Int32 offset, UInt32 length);
};

Int32 rt_mtd_nor_register_device(const char* name, struct rt_mtd_nor_device* device);

rt_inline UInt32 rt_mtd_nor_read_id(struct rt_mtd_nor_device* device)
{
	return device->ops->read_id(device);
}

rt_inline UInt32 rt_mtd_nor_read(
	struct rt_mtd_nor_device* device,
	Int32 offset, UInt8* data, UInt32 length)
{
	return device->ops->read(device, offset, data, length);
}

rt_inline UInt32 rt_mtd_nor_write(
	struct rt_mtd_nor_device* device,
	Int32 offset, const UInt8* data, UInt32 length)
{
	return device->ops->write(device, offset, data, length);
}

rt_inline Int32 rt_mtd_nor_erase_block(struct rt_mtd_nor_device* device, Int32 offset, UInt32 length)
{
	return device->ops->erase_block(device, offset, length);
}

#endif
