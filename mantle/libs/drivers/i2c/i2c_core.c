/*
 * File      : i2c_core.c
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

#include <rtdevice.h>

Int32 rt_i2c_bus_device_register(struct rt_i2c_bus_device *bus,
                                    const char               *bus_name)
{
    Int32 res = RT_EOK;

    rt_mutex_init(&bus->lock, "i2c_bus_lock", RT_IPC_FLAG_FIFO);

    if (bus->timeout == 0) bus->timeout = RT_TICK_PER_SECOND;

    res = rt_i2c_bus_device_device_init(bus, bus_name);

    i2c_dbg("I2C bus [%s] registered\n", bus_name);

    return res;
}

struct rt_i2c_bus_device *rt_i2c_bus_device_find(const char *bus_name)
{
    struct rt_i2c_bus_device *bus;
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == NULL || dev->type != RT_Device_Class_I2CBUS)
    {
        i2c_dbg("I2C bus %s not exist\n", bus_name);

        return NULL;
    }

    bus = (struct rt_i2c_bus_device *)dev->user_data;

    return bus;
}

UInt32 rt_i2c_transfer(struct rt_i2c_bus_device *bus,
                          struct rt_i2c_msg         msgs[],
                          UInt32               num)
{
    UInt32 ret;

    if (bus->ops->master_xfer)
    {
#ifdef RT_I2C_DEBUG
        for (ret = 0; ret < num; ret++)
        {
            i2c_dbg("msgs[%d] %c, addr=0x%02x, len=%d%s\n", ret,
                    (msgs[ret].flags & RT_I2C_RD) ? 'R' : 'W',
                    msgs[ret].addr, msgs[ret].len);
        }
#endif

        rt_mutex_take(&bus->lock, RT_WAITING_FOREVER);
        ret = bus->ops->master_xfer(bus, msgs, num);
        rt_mutex_release(&bus->lock);

        return ret;
    }
    else
    {
        i2c_dbg("I2C bus operation not supported\n");

        return 0;
    }
}

UInt32 rt_i2c_master_send(struct rt_i2c_bus_device *bus,
                             UInt16               addr,
                             UInt16               flags,
                             const UInt8         *buf,
                             UInt32               count)
{
    UInt32 ret;
    struct rt_i2c_msg msg;

    msg.addr  = addr;
    msg.flags = flags & RT_I2C_ADDR_10BIT;
    msg.len   = count;
    msg.buf   = (UInt8 *)buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret > 0) ? count : ret;
}

UInt32 rt_i2c_master_recv(struct rt_i2c_bus_device *bus,
                             UInt16               addr,
                             UInt16               flags,
                             UInt8               *buf,
                             UInt32               count)
{
    UInt32 ret;
    struct rt_i2c_msg msg;
    assert(bus != NULL);

    msg.addr   = addr;
    msg.flags  = flags & RT_I2C_ADDR_10BIT;
    msg.flags |= RT_I2C_RD;
    msg.len    = count;
    msg.buf    = buf;

    ret = rt_i2c_transfer(bus, &msg, 1);

    return (ret > 0) ? count : ret;
}

int rt_i2c_core_init(void)
{
    return 0;
}
INIT_COMPONENT_EXPORT(rt_i2c_core_init);
