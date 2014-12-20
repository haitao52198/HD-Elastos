/*
 * File      : mtd_nor.c
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

#include <drivers/mtd_nor.h>

#ifdef RT_USING_MTD_NOR

/**
 * RT-Thread Generic Device Interface
 */
static Int32 _mtd_init(rt_device_t dev)
{
    return RT_EOK;
}

static Int32 _mtd_open(rt_device_t dev, UInt16 oflag)
{
    return RT_EOK;
}

static Int32 _mtd_close(rt_device_t dev)
{
    return RT_EOK;
}

static UInt32 _mtd_read(rt_device_t dev,
                           Int32    pos,
                           void       *buffer,
                           UInt32   size)
{
    return size;
}

static UInt32 _mtd_write(rt_device_t dev,
                            Int32    pos,
                            const void *buffer,
                            UInt32   size)
{
    return size;
}

static Int32 _mtd_control(rt_device_t dev, UInt8 cmd, void *args)
{
    return RT_EOK;
}

Int32 rt_mtd_nor_register_device(const char               *name,
                                    struct rt_mtd_nor_device *device)
{
    rt_device_t dev;

    dev = RT_DEVICE(device);
    assert(dev != NULL);

    /* set device class and generic device interface */
    dev->type        = RT_Device_Class_MTD;
    dev->init        = _mtd_init;
    dev->open        = _mtd_open;
    dev->read        = _mtd_read;
    dev->write       = _mtd_write;
    dev->close       = _mtd_close;
    dev->control     = _mtd_control;

    dev->rx_indicate = NULL;
    dev->tx_complete = NULL;

    /* register to RT-Thread device system */
    return rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
}

#endif
