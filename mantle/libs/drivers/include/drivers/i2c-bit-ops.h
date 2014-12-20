/*
 * File      : i2c-bit-ops.h
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

#ifndef __I2C_BIT_OPS_H__
#define __I2C_BIT_OPS_H__

#ifdef __cplusplus
extern "C" {
#endif

struct rt_i2c_bit_ops
{
    void *data;            /* private data for lowlevel routines */
    void (*set_sda)(void *data, Int32 state);
    void (*set_scl)(void *data, Int32 state);
    Int32 (*get_sda)(void *data);
    Int32 (*get_scl)(void *data);

    void (*udelay)(UInt32 us);

    UInt32 delay_us;  /* scl and sda line delay */
    UInt32 timeout;   /* in tick */
};

Int32 rt_i2c_bit_add_bus(struct rt_i2c_bus_device *bus,
                            const char               *bus_name);

#ifdef __cplusplus
}
#endif

#endif
