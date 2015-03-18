/*
 * File      : spi_dev.c
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
 */

#include <hdElastosMantle.h>
#include <drivers/spi.h>

/* SPI bus device interface, compatible with RT-Thread 0.3.x/1.0.x */
static UInt32 _spi_bus_device_read(rt_device_t dev,
                                      Int32    pos,
                                      void       *buffer,
                                      UInt32   size)
{
    struct rt_spi_bus *bus;

    bus = (struct rt_spi_bus *)dev;
    assert(bus != NULL);
    assert(bus->owner != NULL);

    return rt_spi_transfer(bus->owner, NULL, buffer, size);
}

static UInt32 _spi_bus_device_write(rt_device_t dev,
                                       Int32    pos,
                                       const void *buffer,
                                       UInt32   size)
{
    struct rt_spi_bus *bus;

    bus = (struct rt_spi_bus *)dev;
    assert(bus != NULL);
    assert(bus->owner != NULL);

    return rt_spi_transfer(bus->owner, buffer, NULL, size);
}

static Int32 _spi_bus_device_control(rt_device_t dev,
                                        UInt8  cmd,
                                        void       *args)
{
    /* TODO: add control command handle */
    switch (cmd)
    {
    case 0: /* set device */
        break;
    case 1:
        break;
    }

    return RT_EOK;
}

Int32 rt_spi_bus_device_init(struct rt_spi_bus *bus, const char *name)
{
    struct rt_device *device;
    assert(bus != NULL);

    device = &bus->parent;

    /* set device type */
    device->type    = RT_Device_Class_SPIBUS;
    /* initialize device interface */
    device->init    = NULL;
    device->open    = NULL;
    device->close   = NULL;
    device->read    = _spi_bus_device_read;
    device->write   = _spi_bus_device_write;
    device->control = _spi_bus_device_control;

    /* register to device manager */
    return rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);
}

/* SPI Dev device interface, compatible with RT-Thread 0.3.x/1.0.x */
static UInt32 _spidev_device_read(rt_device_t dev,
                                     Int32    pos,
                                     void       *buffer,
                                     UInt32   size)
{
    struct rt_spi_device *device;

    device = (struct rt_spi_device *)dev;
    assert(device != NULL);
    assert(device->bus != NULL);

    return rt_spi_transfer(device, NULL, buffer, size);
}

static UInt32 _spidev_device_write(rt_device_t dev,
                                      Int32    pos,
                                      const void *buffer,
                                      UInt32   size)
{
    struct rt_spi_device *device;

    device = (struct rt_spi_device *)dev;
    assert(device != NULL);
    assert(device->bus != NULL);

    return rt_spi_transfer(device, buffer, NULL, size);
}

static Int32 _spidev_device_control(rt_device_t dev,
                                       UInt8  cmd,
                                       void       *args)
{
    switch (cmd)
    {
    case 0: /* set device */
        break;
    case 1:
        break;
    }

    return RT_EOK;
}

Int32 rt_spidev_device_init(struct rt_spi_device *dev, const char *name)
{
    struct rt_device *device;
    assert(dev != NULL);

    device = &(dev->parent);

    /* set device type */
    device->type    = RT_Device_Class_SPIDevice;
    device->init    = NULL;
    device->open    = NULL;
    device->close   = NULL;
    device->read    = _spidev_device_read;
    device->write   = _spidev_device_write;
    device->control = _spidev_device_control;

    /* register to device manager */
    return rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);
}
