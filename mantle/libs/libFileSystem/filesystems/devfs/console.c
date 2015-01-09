/*
 * File      : console.c
 * 
 * COPYRIGHT (C) 2004-2011, RT-Thread Development Team
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

#include <hdElastos.h>

struct console_device
{
	struct rt_device parent;

	rt_device_t device; /* the actual device */
};
struct console_device _console;

/* common device interface */
static Int32 console_init(rt_device_t dev)
{
	return RT_EOK;
}

static Int32 console_open(rt_device_t dev, UInt16 oflag)
{
	return RT_EOK;
}

static Int32 console_close(rt_device_t dev)
{
	return RT_EOK;
}

static UInt32 console_read(rt_device_t dev, Int32 pos, void* buffer, UInt32 size)
{
	struct console_device* device;

	device = (struct console_device*)dev;
	assert(device != NULL);

	return rt_device_read(device->device, pos, buffer, size);
}

static UInt32 console_write(rt_device_t dev, Int32 pos, const void* buffer, UInt32 size)
{
	struct console_device* device;

	device = (struct console_device*)dev;
	assert(device != NULL);

	return rt_device_write(device->device, pos, buffer, size);
}

static Int32 console_control(rt_device_t dev, UInt8 cmd, void *args)
{
	return rt_device_control(_console.device, cmd, args);
}

void rt_console_init(const char* device_name)
{
	rt_device_t device;
	/* register to device framework */

	device = rt_device_find(device_name);
	if (device != NULL)
	{
		struct console_device* console;
		/* get console device */
		console = &_console;
		memset(console, 0, sizeof(_console));

		/* device initialization */
		console->parent.type = RT_Device_Class_Char;
		/* set device interface */
		console->parent.init 	= console_init;
		console->parent.open 	= console_open;
		console->parent.close   = console_close;
		console->parent.read 	= console_read;
		console->parent.write   = console_write;
		console->parent.control	= console_control;
		console->parent.user_data = NULL;

		console->device = device;

		rt_device_register(&console->parent, "console", RT_DEVICE_FLAG_RDWR);
	}
}

