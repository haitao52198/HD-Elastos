/*
 * File      : i2c.h
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
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 */

#ifndef __I2C_H__
#define __I2C_H__

#include <hdElastosMantle.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RT_I2C_WR                0x0000
#define RT_I2C_RD               (1u << 0)
#define RT_I2C_ADDR_10BIT       (1u << 2)  /* this is a ten bit chip address */
#define RT_I2C_NO_START         (1u << 4)
#define RT_I2C_IGNORE_NACK      (1u << 5)
#define RT_I2C_NO_READ_ACK      (1u << 6)  /* when I2C reading, we do not ACK */

struct rt_i2c_msg
{
    UInt16 addr;
    UInt16 flags;
    UInt16 len;
    UInt8  *buf;
};

struct rt_i2c_bus_device;

struct rt_i2c_bus_device_ops
{
    UInt32 (*master_xfer)(struct rt_i2c_bus_device *bus,
                             struct rt_i2c_msg msgs[],
                             UInt32 num);
    UInt32 (*slave_xfer)(struct rt_i2c_bus_device *bus,
                            struct rt_i2c_msg msgs[],
                            UInt32 num);
    Int32 (*i2c_bus_control)(struct rt_i2c_bus_device *bus,
                                UInt32,
                                UInt32);
};

/*for i2c bus driver*/
struct rt_i2c_bus_device
{
    struct rt_device parent;
    const struct rt_i2c_bus_device_ops *ops;
    UInt16  flags;
    UInt16  addr;
    struct rt_mutex lock;
    UInt32  timeout;
    UInt32  retries;
    void *priv;
};

#ifdef RT_I2C_DEBUG
#define i2c_dbg(fmt, ...)   printf(fmt, ##__VA_ARGS__)
#else
#define i2c_dbg(fmt, ...)
#endif

Int32 rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name);
struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name);
UInt32 rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          UInt32               num);
UInt32 rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             UInt16               addr,
                             UInt16               flags,
                             const UInt8         *buf,
                             UInt32               count);
UInt32 rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             UInt16               addr,
                             UInt16               flags,
                             UInt8               *buf,
                             UInt32               count);
int rt_i2c_core_init(void);

#ifdef __cplusplus
}
#endif

#endif
