/*
 * File      : mtd_core.c
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
 */

/*
 * COPYRIGHT (C) 2012, Shanghai Real Thread
 */

#include <drivers/mtd_nand.h>

#ifdef RT_USING_MTD_NAND

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

Int32 rt_mtd_nand_register_device(const char                *name,
                                     struct rt_mtd_nand_device *device)
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

#if defined(RT_MTD_NAND_DEBUG) && defined(RT_USING_FINSH)
#include <finsh.h>
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

static void mtd_dump_hex(const UInt8 *ptr, UInt32 buflen)
{
	unsigned char *buf = (unsigned char*)ptr;
	int i, j;
	for (i=0; i<buflen; i+=16) 
	{
		printf("%06x: ", i);
		for (j=0; j<16; j++)
			if (i+j < buflen)
				printf("%02x ", buf[i+j]);
			else
				printf("   ");
		printf(" ");
		for (j=0; j<16; j++)
			if (i+j < buflen)
				printf("%c", __is_print(buf[i+j]) ? buf[i+j] : '.');
		printf("\n");
	}
}

int mtd_nandid(const char* name)
{
	struct rt_mtd_nand_device *nand;
	nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
	if (nand == NULL)
	{
		printf("no nand device found!\n");
		return -RT_ERROR;
	}

	return rt_mtd_nand_read_id(nand);
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nandid, nand_id, read ID - nandid(name));

int mtd_nand_read(const char* name, int block, int page)
{
	Int32 result;
	UInt8 *page_ptr;
	UInt8 *oob_ptr;
	struct rt_mtd_nand_device *nand;

	nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
	if (nand == NULL)
	{
		printf("no nand device found!\n");
		return -RT_ERROR;
	}

	page_ptr = malloc(nand->page_size + nand->oob_size);
	if (page_ptr == NULL)
	{
		printf("out of memory!\n");
		return -RT_ENOMEM;
	}

	oob_ptr = page_ptr + nand->page_size;
	memset(page_ptr, 0xff, nand->page_size + nand->oob_size);

	/* calculate the page number */
	page = block * nand->pages_per_block + page;
	result = rt_mtd_nand_read(nand, page, page_ptr, nand->page_size,
		oob_ptr, nand->oob_size);

	printf("read page, rc=%d\n", result);
	mtd_dump_hex(page_ptr, nand->page_size);
	mtd_dump_hex(oob_ptr, nand->oob_size);

	rt_free(page_ptr);
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_read, nand_read, read page in nand - nand_read(name, block, page));

int mtd_nand_readoob(const char* name, int block, int page)
{
	struct rt_mtd_nand_device *nand;
	UInt8 *oob_ptr;

	nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
	if (nand == NULL)
	{
		printf("no nand device found!\n");
		return -RT_ERROR;
	}

	oob_ptr = malloc(nand->oob_size);
	if (oob_ptr == NULL)
	{
		printf("out of memory!\n");
		return -RT_ENOMEM;
	}

	/* calculate the page number */
	page = block * nand->pages_per_block + page;
	rt_mtd_nand_read(nand, page, NULL, nand->page_size,
		oob_ptr, nand->oob_size);
	mtd_dump_hex(oob_ptr, nand->oob_size);

	rt_free(oob_ptr);
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_readoob, nand_readoob, read spare data in nand - nand_readoob(name, block, page));

int mtd_nand_write(const char* name, int block, int page)
{
	Int32 result;
	UInt8 *page_ptr;
	UInt8 *oob_ptr;
	UInt32 index;
	struct rt_mtd_nand_device *nand;

	nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
	if (nand == NULL)
	{
		printf("no nand device found!\n");
		return -RT_ERROR;
	}

	page_ptr = malloc(nand->page_size + nand->oob_size);
	if (page_ptr == NULL)
	{
		printf("out of memory!\n");
		return -RT_ENOMEM;
	}

	oob_ptr = page_ptr + nand->page_size;
	/* prepare page data */
	for (index = 0; index < nand->page_size; index ++)
	{
		page_ptr[index] = index & 0xff;
	}
	/* prepare oob data */
	for (index = 0; index < nand->oob_size; index ++)
	{
		oob_ptr[index] = index & 0xff;
	}

	/* calculate the page number */
	page = block * nand->pages_per_block + page;
	result = rt_mtd_nand_write(nand, page, page_ptr, nand->page_size,
		oob_ptr, nand->oob_size);
	if (result != RT_MTD_EOK)
	{
		printf("write page failed!, rc=%d\n", result);
	}

	rt_free(page_ptr);
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_write, nand_write, write dump data to nand - nand_write(name, block, page));

int mtd_nand_erase(const char* name, int block)
{
	struct rt_mtd_nand_device *nand;
	nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
	if (nand == NULL)
	{
		printf("no nand device found!\n");
		return -RT_ERROR;
	}

	return rt_mtd_nand_erase_block(nand, block);
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_erase, nand_erase, nand_erase(name, block));

int mtd_nand_erase_all(const char* name)
{
	UInt32 index = 0;
	struct rt_mtd_nand_device *nand;
	
	nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
	if (nand == NULL)
	{
		printf("no nand device found!\n");
		return -RT_ERROR;
	}

	for (index = 0; index < (nand->block_end - nand->block_start); index ++)
	{
		rt_mtd_nand_erase_block(nand, index);
	}
	
	return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_erase_all, nand_erase_all, erase all of nand device - nand_erase_all(name, block));
#endif

#endif
