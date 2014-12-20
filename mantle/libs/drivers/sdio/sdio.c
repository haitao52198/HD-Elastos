/*
 * File      : sdio.c
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
 * Date           Author        Notes
 * 2012-01-13     weety         first version
 */

#include <drivers/mmcsd_core.h>
#include <drivers/sdio.h>

#ifndef RT_SDIO_STACK_SIZE
#define RT_SDIO_STACK_SIZE 512
#endif
#ifndef RT_SDIO_THREAD_PREORITY
#define RT_SDIO_THREAD_PREORITY  0x40
#endif

static rt_list_t sdio_cards;
static rt_list_t sdio_drivers;

struct sdio_card
{
    struct rt_mmcsd_card *card;
    rt_list_t  list;
};

struct sdio_driver
{
    struct rt_sdio_driver *drv;
    rt_list_t  list;
};

#define MIN(a, b) (a < b ? a : b)

static const UInt8 speed_value[16] =
{
    0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};

static const UInt32 speed_unit[8] =
{
    10000, 100000, 1000000, 10000000, 0, 0, 0, 0
};

rt_inline Int32 sdio_match_card(struct rt_mmcsd_card           *card,
                                     const struct rt_sdio_device_id *id);


Int32 sdio_io_send_op_cond(struct rt_mmcsd_host *host,
                                UInt32           ocr,
                                UInt32          *cmd5_resp)
{
    struct rt_mmcsd_cmd cmd;
    Int32 i, err = 0;

    assert(host != NULL);

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_IO_SEND_OP_COND;
    cmd.arg = ocr;
    cmd.flags = RESP_SPI_R4 | RESP_R4 | CMD_BCR;

    for (i = 100; i; i--) 
    {
        err = mmcsd_send_cmd(host, &cmd, 0);
        if (err)
            break;

        /* if we're just probing, do a single pass */
        if (ocr == 0)
            break;

        /* otherwise wait until reset completes */
        if (controller_is_spi(host)) 
        {
            /*
             * Both R1_SPI_IDLE and MMC_CARD_BUSY indicate
             * an initialized card under SPI, but some cards
             * (Marvell's) only behave when looking at this
             * one.
             */
            if (cmd.resp[1] & CARD_BUSY)
                break;
        } 
        else 
        {
            if (cmd.resp[0] & CARD_BUSY)
                break;
        }

        err = -RT_ETIMEOUT;

        mmcsd_delay_ms(10);
    }

    if (cmd5_resp)
        *cmd5_resp = cmd.resp[controller_is_spi(host) ? 1 : 0];

    return err;
}

Int32 sdio_io_rw_direct(struct rt_mmcsd_card *card,
                             Int32            rw,
                             UInt32           fn,
                             UInt32           reg_addr,
                             UInt8           *pdata,
                             UInt8            raw)
{
    struct rt_mmcsd_cmd cmd;
    Int32 err;

    assert(card != NULL);
    assert(fn <= SDIO_MAX_FUNCTIONS);

    if (reg_addr & ~SDIO_ARG_CMD53_REG_MASK)
        return -RT_ERROR;

    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));

    cmd.cmd_code = SD_IO_RW_DIRECT;
    cmd.arg = rw ? SDIO_ARG_CMD52_WRITE : SDIO_ARG_CMD52_READ;
    cmd.arg |= fn << SDIO_ARG_CMD52_FUNC_SHIFT;
    cmd.arg |= raw ? SDIO_ARG_CMD52_RAW_FLAG : 0x00000000;
    cmd.arg |= reg_addr << SDIO_ARG_CMD52_REG_SHIFT;
    cmd.arg |= *pdata;
    cmd.flags = RESP_SPI_R5 | RESP_R5 | CMD_AC;

    err = mmcsd_send_cmd(card->host, &cmd, 0);
    if (err)
        return err;

    if (!controller_is_spi(card->host)) 
    {
        if (cmd.resp[0] & R5_ERROR)
            return -RT_EIO;
        if (cmd.resp[0] & R5_FUNCTION_NUMBER)
            return -RT_ERROR;
        if (cmd.resp[0] & R5_OUT_OF_RANGE)
            return -RT_ERROR;
    }

    if (!rw || raw) 
    {
        if (controller_is_spi(card->host))
            *pdata = (cmd.resp[0] >> 8) & 0xFF;
        else
            *pdata = cmd.resp[0] & 0xFF;
    }

    return 0;
}

Int32 sdio_io_rw_extended(struct rt_mmcsd_card *card,
                               Int32            rw,
                               UInt32           fn,
                               UInt32           addr,
                               Int32            op_code,
                               UInt8           *buf,
                               UInt32           blocks,
                               UInt32           blksize)
{
    struct rt_mmcsd_req req;
    struct rt_mmcsd_cmd cmd;
    struct rt_mmcsd_data data;

    assert(card != NULL);
    assert(fn <= SDIO_MAX_FUNCTIONS);
    assert(blocks != 1 || blksize <= 512);
    assert(blocks != 0);
    assert(blksize != 0);

    if (addr & ~SDIO_ARG_CMD53_REG_MASK)
        return -RT_ERROR;

    memset(&req, 0, sizeof(struct rt_mmcsd_req));
    memset(&cmd, 0, sizeof(struct rt_mmcsd_cmd));
    memset(&data, 0, sizeof(struct rt_mmcsd_data));

    req.cmd = &cmd;
    req.data = &data;

    cmd.cmd_code = SD_IO_RW_EXTENDED;
    cmd.arg = rw ? SDIO_ARG_CMD53_WRITE : SDIO_ARG_CMD53_READ;
    cmd.arg |= fn << SDIO_ARG_CMD53_FUNC_SHIFT;
    cmd.arg |= op_code ? SDIO_ARG_CMD53_INCREMENT : 0x00000000;
    cmd.arg |= addr << SDIO_ARG_CMD53_REG_SHIFT;
    if (blocks == 1 && blksize <= 512)
        cmd.arg |= (blksize == 512) ? 0 : blksize;      /* byte mode */
    else
        cmd.arg |= SDIO_ARG_CMD53_BLOCK_MODE | blocks;  /* block mode */
    cmd.flags = RESP_SPI_R5 | RESP_R5 | CMD_ADTC;

    data.blksize = blksize;
    data.blks = blocks;
    data.flags = rw ? DATA_DIR_WRITE : DATA_DIR_READ;
    data.buf = (UInt32 *)buf;

    mmcsd_set_data_timeout(&data, card);

    mmcsd_send_request(card->host, &req);

    if (cmd.err)
        return cmd.err;
    if (data.err)
        return data.err;

    if (!controller_is_spi(card->host)) 
    {
        if (cmd.resp[0] & R5_ERROR)
            return -RT_EIO;
        if (cmd.resp[0] & R5_FUNCTION_NUMBER)
            return -RT_ERROR;
        if (cmd.resp[0] & R5_OUT_OF_RANGE)
            return -RT_ERROR;
    }

    return 0;
}

rt_inline UInt32 sdio_max_block_size(struct rt_sdio_function *func)
{
    UInt32 size = MIN(func->card->host->max_seg_size,
                           func->card->host->max_blk_size);
    size = MIN(size, func->max_blk_size);

    return MIN(size, 512u); /* maximum size for byte mode */
}

static Int32 sdio_io_rw_extended_block(struct rt_sdio_function *func,
                                            Int32               rw,
                                            UInt32              addr,
                                            Int32               op_code,
                                            UInt8              *buf,
                                            UInt32              len)
{
    Int32  ret;
    UInt32 left_size;
    UInt32 max_blks, blks;
    
    left_size = len;

    /* Do the bulk of the transfer using block mode (if supported). */
    if (func->card->cccr.multi_block && (len > sdio_max_block_size(func)))
    {
        max_blks = MIN(func->card->host->max_blk_count,
                       func->card->host->max_seg_size / func->cur_blk_size);
        max_blks = MIN(max_blks, 511u);

        while (left_size > func->cur_blk_size)
        {
            blks = left_size / func->cur_blk_size;
            if (blks > max_blks)
                blks = max_blks;
            len = blks * func->cur_blk_size;

            ret = sdio_io_rw_extended(func->card, rw, func->num, 
                  addr, op_code, buf, blks, func->cur_blk_size);
            if (ret)
                return ret;

            left_size -= len;
            buf += len;
            if (op_code)
                addr += len;
        }
    }

    while (left_size > 0)
    {
        len = MIN(left_size, sdio_max_block_size(func));

        ret = sdio_io_rw_extended(func->card, rw, func->num, 
                  addr, op_code, buf, 1, len);
        if (ret)
            return ret;

        left_size -= len;
        buf += len;
        if (op_code)
            addr += len;
    }

    return 0;
}

UInt8 sdio_io_readb(struct rt_sdio_function *func, 
                         UInt32              reg,
                         Int32              *err)
{
    UInt8 data;
    Int32 ret;

    ret = sdio_io_rw_direct(func->card, 0, func->num, reg, &data, 0);

    if (err)
    {
        *err = ret;
    }

    return data;
}

Int32 sdio_io_writeb(struct rt_sdio_function *func, 
                          UInt32              reg,
                          UInt8               data)
{
    return sdio_io_rw_direct(func->card, 1, func->num, reg, &data, 0);
}

UInt16 sdio_io_readw(struct rt_sdio_function *func,
                          UInt32              addr,
                          Int32              *err)
{
    Int32 ret;
    UInt32 dmabuf;

    if (err)
        *err = 0;

    ret = sdio_io_rw_extended_block(func, 0, addr, 1, (UInt8 *)&dmabuf, 2);
    if (ret) 
    {
        if (err)
            *err = ret;
    }

    return (UInt16)dmabuf;
}

Int32 sdio_io_writew(struct rt_sdio_function *func,
                          UInt16              data,
                          UInt32              addr)
{
    UInt32 dmabuf = data;

    return sdio_io_rw_extended_block(func, 1, addr, 1, (UInt8 *)&dmabuf, 2);
}

UInt32 sdio_io_readl(struct rt_sdio_function *func,
                          UInt32              addr,
                          Int32              *err)
{
    Int32 ret;
    UInt32 dmabuf;

    if (err)
        *err = 0;

    ret = sdio_io_rw_extended_block(func, 0, addr, 1, (UInt8 *)&dmabuf, 4);
    if (ret) 
    {
        if (err)
            *err = ret;
    }

    return dmabuf;
}

Int32 sdio_io_writel(struct rt_sdio_function *func,
                          UInt32              data,
                          UInt32              addr)
{
    UInt32 dmabuf = data;

    return sdio_io_rw_extended_block(func, 1, addr, 1, (UInt8 *)&dmabuf, 4);
}

Int32 sdio_io_read_multi_fifo_b(struct rt_sdio_function *func, 
                                     UInt32              addr,
                                     UInt8              *buf,
                                     UInt32              len)
{
    return sdio_io_rw_extended_block(func, 0, addr, 0, buf, len);
}

Int32 sdio_io_write_multi_fifo_b(struct rt_sdio_function *func, 
                                      UInt32              addr,
                                      UInt8              *buf,
                                      UInt32              len)
{
    return sdio_io_rw_extended_block(func, 1, addr, 0, buf, len);
}

Int32 sdio_io_read_multi_incr_b(struct rt_sdio_function *func, 
                                     UInt32              addr,
                                     UInt8              *buf,
                                     UInt32              len)
{
    return sdio_io_rw_extended_block(func, 0, addr, 1, buf, len);
}

Int32 sdio_io_write_multi_incr_b(struct rt_sdio_function *func, 
                                      UInt32              addr,
                                      UInt8              *buf,
                                      UInt32              len)
{
    return sdio_io_rw_extended_block(func, 1, addr, 1, buf, len);
}

static Int32 sdio_read_cccr(struct rt_mmcsd_card *card)
{
    Int32 ret;
    Int32 cccr_version;
    UInt8 data;

    memset(&card->cccr, 0, sizeof(struct rt_sdio_cccr));

    data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_CCCR_REV, &ret);
    if (ret)
        goto out;

    cccr_version = data & 0x0f;

    if (cccr_version > SDIO_CCCR_REV_1_20) 
    {
        printf("unrecognised CCCR structure version %d\n", cccr_version);

        return -RT_ERROR;
    }

    card->cccr.sdio_version = (data & 0xf0) >> 4;

    data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_CARD_CAPS, &ret);
    if (ret)
        goto out;

    if (data & SDIO_CCCR_CAP_SMB)
        card->cccr.multi_block = 1;
    if (data & SDIO_CCCR_CAP_LSC)
        card->cccr.low_speed = 1;
    if (data & SDIO_CCCR_CAP_4BLS)
        card->cccr.low_speed_4 = 1;
    if (data & SDIO_CCCR_CAP_4BLS)
        card->cccr.bus_width = 1;

    if (cccr_version >= SDIO_CCCR_REV_1_10) 
    {
        data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_POWER_CTRL, &ret);
        if (ret)
            goto out;

        if (data & SDIO_POWER_SMPC)
            card->cccr.power_ctrl = 1;
    }

    if (cccr_version >= SDIO_CCCR_REV_1_20) 
    {
        data = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_SPEED, &ret);
        if (ret)
            goto out;

        if (data & SDIO_SPEED_SHS)
            card->cccr.high_speed = 1;
    }

out:
    return ret;
}

static Int32 cistpl_funce_func0(struct rt_mmcsd_card *card,
                                     const UInt8     *buf,
                                     UInt32           size)
{
    if (size < 0x04 || buf[0] != 0)
        return -RT_ERROR;

    /* TPLFE_FN0_BLK_SIZE */
    card->cis.func0_blk_size = buf[1] | (buf[2] << 8);

    /* TPLFE_MAX_TRAN_SPEED */
    card->cis.max_tran_speed = speed_value[(buf[3] >> 3) & 15] *
                speed_unit[buf[3] & 7];

    return 0;
}

static Int32 cistpl_funce_func(struct rt_sdio_function *func,
                                    const UInt8        *buf,
                                    UInt32              size)
{
    UInt32 version;
    UInt32 min_size;

    version = func->card->cccr.sdio_version;
    min_size = (version == SDIO_SDIO_REV_1_00) ? 28 : 42;

    if (size < min_size || buf[0] != 1)
        return -RT_ERROR;

    /* TPLFE_MAX_BLK_SIZE */
    func->max_blk_size = buf[12] | (buf[13] << 8);

    /* TPLFE_ENABLE_TIMEOUT_VAL, present in ver 1.1 and above */
    if (version > SDIO_SDIO_REV_1_00)
        func->enable_timeout_val = (buf[28] | (buf[29] << 8)) * 10;
    else
        func->enable_timeout_val = 1000; /* 1000ms */

    return 0;
}

static Int32 sdio_read_cis(struct rt_sdio_function *func)
{
    Int32 ret;
    struct rt_sdio_function_tuple *curr, **prev;
    UInt32 i, cisptr = 0;
    UInt8 data;
    UInt8 tpl_code, tpl_link;

    struct rt_mmcsd_card *card = func->card;
    struct rt_sdio_function *func0 = card->sdio_function[0];

    assert(func0 != NULL);

    for (i = 0; i < 3; i++)
    {
        data = sdio_io_readb(func0, 
            SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_CIS + i, &ret);
        if (ret)
            return ret;
        cisptr |= data << (i * 8);
    }

    prev = &func->tuples;

    do {
        tpl_code = sdio_io_readb(func0, cisptr++, &ret);
        if (ret)
            break;
        tpl_link = sdio_io_readb(func0, cisptr++, &ret);
        if (ret)
            break;

        if ((tpl_code == CISTPL_END) || (tpl_link == 0xff))
            break;

        if (tpl_code == CISTPL_NULL)
            continue;


        curr = malloc(sizeof(struct rt_sdio_function_tuple) + tpl_link);
        if (!curr)
            return -RT_ENOMEM;
        curr->data = (UInt8 *)curr + sizeof(struct rt_sdio_function_tuple);

        for (i = 0; i < tpl_link; i++) 
        {
            curr->data[i] = sdio_io_readb(func0, cisptr + i, &ret);
            if (ret)
                break;
        }
        if (ret) 
        {
            rt_free(curr);
            break;
        }

        switch (tpl_code)
        {
        case CISTPL_MANFID:
            if (tpl_link < 4)
            {
                printf("bad CISTPL_MANFID length\n");
                break;
            }
            if (func->num != 0)
            {
                func->manufacturer = curr->data[0];
                func->manufacturer |= curr->data[1] << 8;
                func->product = curr->data[2];
                func->product |= curr->data[3] << 8;
            }
            else
            {
                card->cis.manufacturer = curr->data[0];
                card->cis.manufacturer |= curr->data[1] << 8;
                card->cis.product = curr->data[2];
                card->cis.product |= curr->data[3] << 8;
            }
            break;
        case CISTPL_FUNCE:
            if (func->num != 0)
                ret = cistpl_funce_func(func, curr->data, tpl_link);
            else
                ret = cistpl_funce_func0(card, curr->data, tpl_link);

            if (ret)
            {
                printf("bad CISTPL_FUNCE size %u "
                       "type %u\n", tpl_link, curr->data[0]);
            }

            break;
        case CISTPL_VERS_1:
            if (tpl_link < 2)
            {
                printf("CISTPL_VERS_1 too short\n");
            }
            break;
        default: 
            /* this tuple is unknown to the core */
            curr->next = NULL;
            curr->code = tpl_code;
            curr->size = tpl_link;
            *prev = curr;
            prev = &curr->next;
            printf( "function %d, CIS tuple code %#x, length %d\n",
                func->num, tpl_code, tpl_link);
            break;
        }

        cisptr += tpl_link;
    } while (1);

    /*
     * Link in all unknown tuples found in the common CIS so that
     * drivers don't have to go digging in two places.
     */
    if (func->num != 0)
        *prev = func0->tuples;

    return ret;
}


void sdio_free_cis(struct rt_sdio_function *func)
{
    struct rt_sdio_function_tuple *tuple, *tmp;
    struct rt_mmcsd_card *card = func->card;

    tuple = func->tuples;

    while (tuple && ((tuple != card->sdio_function[0]->tuples) || (!func->num))) 
    {
        tmp = tuple;
        tuple = tuple->next;
        rt_free(tmp);
    }

    func->tuples = NULL;
}

static Int32 sdio_read_fbr(struct rt_sdio_function *func)
{
    Int32 ret;
    UInt8 data;
    struct rt_sdio_function *func0 = func->card->sdio_function[0];

    data = sdio_io_readb(func0, 
        SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_STD_FUNC_IF, &ret);
    if (ret)
        goto err;

    data &= 0x0f;

    if (data == 0x0f) 
    {
        data = sdio_io_readb(func0, 
            SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_STD_IF_EXT, &ret);
        if (ret)
            goto err;
    }

    func->func_code = data;

err:
    return ret;
}


static Int32 sdio_initialize_function(struct rt_mmcsd_card *card,
                                           UInt32           func_num)
{
    Int32 ret;
    struct rt_sdio_function *func;

    assert(func_num <= SDIO_MAX_FUNCTIONS);

    func = malloc(sizeof(struct rt_sdio_function));
    if (!func)
    {
        printf("malloc rt_sdio_function failed\n");
        ret = -RT_ENOMEM;
        goto err;
    }
    memset(func, 0, sizeof(struct rt_sdio_function));

    func->card = card;
    func->num = func_num;

    ret = sdio_read_fbr(func);
    if (ret)
        goto err1;

    ret = sdio_read_cis(func);
    if (ret)
        goto err1;

    card->sdio_function[func_num] = func;

    return 0;

err1:
    sdio_free_cis(func);
    rt_free(func);
    card->sdio_function[func_num] = NULL;
err:
    return ret;
}

static Int32 sdio_set_highspeed(struct rt_mmcsd_card *card)
{
    Int32 ret;
    UInt8 speed;

    if (!(card->host->flags & MMCSD_SUP_HIGHSPEED))
        return 0;

    if (!card->cccr.high_speed)
        return 0;

    speed = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_SPEED, &ret);
    if (ret)
        return ret;

    speed |= SDIO_SPEED_EHS;

    ret = sdio_io_writeb(card->sdio_function[0], SDIO_REG_CCCR_SPEED, speed);
    if (ret)
        return ret;

    card->flags |= CARD_FLAG_HIGHSPEED;

    return 0;
}

static Int32 sdio_set_bus_wide(struct rt_mmcsd_card *card)
{
    Int32 ret;
    UInt8 busif;

    if (!(card->host->flags & MMCSD_BUSWIDTH_4))
        return 0;

    if (card->cccr.low_speed && !card->cccr.bus_width)
        return 0;

    busif = sdio_io_readb(card->sdio_function[0], SDIO_REG_CCCR_BUS_IF, &ret);
    if (ret)
        return ret;

    busif |= SDIO_BUS_WIDTH_4BIT;

    ret = sdio_io_writeb(card->sdio_function[0], SDIO_REG_CCCR_BUS_IF, busif);
    if (ret)
        return ret;

    mmcsd_set_bus_width(card->host, MMCSD_BUS_WIDTH_4);

    return 0;
}

static Int32 sdio_register_card(struct rt_mmcsd_card *card)
{
    struct sdio_card *sc;
    struct sdio_driver *sd;
    rt_list_t *l;

    sc = malloc(sizeof(struct sdio_card));
    if (sc == NULL)
    {
        printf("malloc sdio card failed\n");
        return -RT_ENOMEM;
    }

    sc->card = card;
    rt_list_insert_after(&sdio_cards, &sc->list);

    if (rt_list_isempty(&sdio_drivers))
    {
        goto out;
    }

    for (l = (&sdio_drivers)->next; l != &sdio_drivers; l = l->next)
    {
        sd = (struct sdio_driver *)rt_list_entry(l, struct sdio_driver, list);
        if (sdio_match_card(card, sd->drv->id))
        {
            sd->drv->probe(card);
        }
    }

out:
    return 0;
}

static Int32 sdio_init_card(struct rt_mmcsd_host *host, UInt32 ocr)
{
    Int32 err = 0;
    Int32 i, function_num;
    UInt32  cmd5_resp;
    struct rt_mmcsd_card *card;

    err = sdio_io_send_op_cond(host, ocr, &cmd5_resp);
    if (err)
        goto err;

    if (controller_is_spi(host)) 
    {
        err = mmcsd_spi_use_crc(host, host->spi_use_crc);
        if (err)
            goto err;
    }

    function_num = (cmd5_resp & 0x70000000) >> 28;

    card = malloc(sizeof(struct rt_mmcsd_card));
    if (!card) 
    {
        printf("malloc card failed\n");
        err = -RT_ENOMEM;
        goto err;
    }
    memset(card, 0, sizeof(struct rt_mmcsd_card));

    card->card_type = CARD_TYPE_SDIO;
    card->sdio_function_num = function_num;
    card->host = host;
    host->card = card;

    card->sdio_function[0] = malloc(sizeof(struct rt_sdio_function));
    if (!card->sdio_function[0])
    {
        printf("malloc sdio_func0 failed\n");
        err = -RT_ENOMEM;
        goto err1;
    }
    memset(card->sdio_function[0], 0, sizeof(struct rt_sdio_function));
    card->sdio_function[0]->card = card;
    card->sdio_function[0]->num = 0;

    if (!controller_is_spi(host)) 
    {
        err = mmcsd_get_card_addr(host, &card->rca);
        if (err)
            goto err2;

        mmcsd_set_bus_mode(host, MMCSD_BUSMODE_PUSHPULL);
    }

    if (!controller_is_spi(host)) 
    {
        err = mmcsd_select_card(card);
        if (err)
            goto err2;
    }

    err = sdio_read_cccr(card);
    if (err)
        goto err2;

    err = sdio_read_cis(card->sdio_function[0]);
    if (err)
        goto err2;

    err = sdio_set_highspeed(card);
    if (err)
        goto err2;

    if (card->flags & CARD_FLAG_HIGHSPEED) 
    {
        mmcsd_set_clock(host, 50000000);
    } 
    else 
    {
        mmcsd_set_clock(host, card->cis.max_tran_speed);
    }

    err = sdio_set_bus_wide(card);
    if (err)
        goto err2;

    for (i = 1; i < function_num + 1; i++) 
    {
        err = sdio_initialize_function(card, i);
        if (err)
            goto err3;
    }


    /* register sdio card */
    err = sdio_register_card(card);
    if (err)
    {
        goto err3;
    }

    return 0;

err3:
    if (host->card)
    {
        for (i = 1; i < host->card->sdio_function_num + 1; i++)
        {
            if (host->card->sdio_function[i])
            {
                sdio_free_cis(host->card->sdio_function[i]);
                rt_free(host->card->sdio_function[i]);
                host->card->sdio_function[i] = NULL;
                rt_free(host->card);
                host->card = NULL;
            }
        }
    }
err2:
    if (host->card && host->card->sdio_function[0])
    {
        sdio_free_cis(host->card->sdio_function[0]);
        rt_free(host->card->sdio_function[0]);
        host->card->sdio_function[0] = NULL;
    }
err1:
    if (host->card)
    {
        rt_free(host->card);
    }
err:
    printf("error %d while initialising SDIO card\n", err);
    
    return err;
}

Int32 init_sdio(struct rt_mmcsd_host *host, UInt32 ocr)
{
    Int32 err;
    UInt32  current_ocr;

    assert(host != NULL);

    if (ocr & 0x7F) 
    {
        printf("Card ocr below the defined voltage rang.\n");
        ocr &= ~0x7F;
    }

    if (ocr & VDD_165_195) 
    {
        printf("Can't support the low voltage SDIO card.\n");
        ocr &= ~VDD_165_195;
    }

    current_ocr = mmcsd_select_voltage(host, ocr);

    if (!current_ocr) 
    {
        err = -RT_ERROR;
        goto err;
    }

    err = sdio_init_card(host, current_ocr);
    if (err)
        goto remove_card;

    return 0;

remove_card:
    rt_free(host->card);
    host->card = NULL;
err:

    printf("init SDIO card failed\n");

    return err;
}

static void sdio_irq_thread(void *param)
{
    Int32 i, ret;
    UInt8 pending;
    struct rt_mmcsd_card *card;
    struct rt_mmcsd_host *host = (struct rt_mmcsd_host *)param;
    assert(host != NULL);
    card = host->card;
    assert(card != NULL);

    while (1) 
    {
        if (rt_sem_take(host->sdio_irq_sem, RT_WAITING_FOREVER) == RT_EOK)
        {
            mmcsd_host_lock(host);
            pending = sdio_io_readb(host->card->sdio_function[0], 
                        SDIO_REG_CCCR_INT_PEND, &ret);
            if (ret) 
            {
                mmcsd_dbg("error %d reading SDIO_REG_CCCR_INT_PEND\n", ret);
                goto out;
            }

            for (i = 1; i <= 7; i++) 
            {
                if (pending & (1 << i)) 
                {
                    struct rt_sdio_function *func = card->sdio_function[i];
                    if (!func) 
                    {
                        mmcsd_dbg("pending IRQ for "
                            "non-existant function %d\n", func->num);
                        goto out;
                    } 
                    else if (func->irq_handler) 
                    {
                        func->irq_handler(func);
                    } 
                    else 
                    {
                        mmcsd_dbg("pending IRQ with no register handler\n");
                        goto out;
                    }
                }
            }

        out:
            mmcsd_host_unlock(host);
            if (host->flags & MMCSD_SUP_SDIO_IRQ)
                host->ops->enable_sdio_irq(host, 1);
            continue;
        }
    }
}

static Int32 sdio_irq_thread_create(struct rt_mmcsd_card *card)
{
    struct rt_mmcsd_host *host = card->host;

    /* init semaphore and create sdio irq processing thread */
    if (!host->sdio_irq_num)
    {
        host->sdio_irq_num++;
        host->sdio_irq_sem = rt_sem_create("sdio_irq", 0, RT_IPC_FLAG_FIFO);
        assert(host->sdio_irq_sem != NULL);

        host->sdio_irq_thread = rt_thread_create("sdio_irq", sdio_irq_thread, host, 
                             RT_SDIO_STACK_SIZE, RT_SDIO_THREAD_PREORITY, 20);
        if (host->sdio_irq_thread != NULL) 
        {
            rt_thread_startup(host->sdio_irq_thread);
        }
    }

    return 0;
}

static Int32 sdio_irq_thread_delete(struct rt_mmcsd_card *card)
{
    struct rt_mmcsd_host *host = card->host;

    assert(host->sdio_irq_num > 0);

    host->sdio_irq_num--;
    if (!host->sdio_irq_num) 
    {
        if (host->flags & MMCSD_SUP_SDIO_IRQ)
                host->ops->enable_sdio_irq(host, 0);
        rt_sem_delete(host->sdio_irq_sem);
        host->sdio_irq_sem = NULL;
        rt_thread_delete(host->sdio_irq_thread);
        host->sdio_irq_thread = NULL;
    }

    return 0;
}

Int32 sdio_attach_irq(struct rt_sdio_function *func,
                           rt_sdio_irq_handler_t   *handler)
{
    Int32 ret;
    UInt8 reg;
    struct rt_sdio_function *func0;

    assert(func != NULL);
    assert(func->card != NULL);

    func0 = func->card->sdio_function[0];

    mmcsd_dbg("SDIO: enabling IRQ for function %d\n", func->num);

    if (func->irq_handler) 
    {
        mmcsd_dbg("SDIO: IRQ for already in use.\n");

        return -RT_EBUSY;
    }

    reg = sdio_io_readb(func0, SDIO_REG_CCCR_INT_EN, &ret);
    if (ret)
        return ret;

    reg |= 1 << func->num;

    reg |= 1; /* Master interrupt enable */

    ret = sdio_io_writeb(func0, SDIO_REG_CCCR_INT_EN, reg);
    if (ret)
        return ret;

    func->irq_handler = handler;

    ret = sdio_irq_thread_create(func->card);
    if (ret)
        func->irq_handler = NULL;

    return ret;
}

Int32 sdio_detach_irq(struct rt_sdio_function *func)
{
    Int32 ret;
    UInt8 reg;
    struct rt_sdio_function *func0;

    assert(func != NULL);
    assert(func->card != NULL);

    func0 = func->card->sdio_function[0];

    mmcsd_dbg("SDIO: disabling IRQ for function %d\n", func->num);

    if (func->irq_handler) 
    {
        func->irq_handler = NULL;
        sdio_irq_thread_delete(func->card);
    }

    reg = sdio_io_readb(func0, SDIO_REG_CCCR_INT_EN, &ret);
    if (ret)
        return ret;

    reg &= ~(1 << func->num);

    /* Disable master interrupt with the last function interrupt */
    if (!(reg & 0xFE))
        reg = 0;

    ret = sdio_io_writeb(func0, SDIO_REG_CCCR_INT_EN, reg);
    if (ret)
        return ret;

    return 0;
}

void sdio_irq_wakeup(struct rt_mmcsd_host *host)
{
    host->ops->enable_sdio_irq(host, 0);
    rt_sem_release(host->sdio_irq_sem);
}

Int32 sdio_enable_func(struct rt_sdio_function *func)
{
    Int32 ret;
    UInt8 reg;
    UInt32 timeout;
    struct rt_sdio_function *func0;

    assert(func != NULL);
    assert(func->card != NULL);

    func0 = func->card->sdio_function[0];

    mmcsd_dbg("SDIO: enabling function %d\n", func->num);

    reg = sdio_io_readb(func0, SDIO_REG_CCCR_IO_EN, &ret);
    if (ret)
        goto err;

    reg |= 1 << func->num;

    ret = sdio_io_writeb(func0, SDIO_REG_CCCR_IO_EN, reg);
    if (ret)
        goto err;

    timeout = rt_tick_get() + func->enable_timeout_val * 1000 / RT_TICK_PER_SECOND;

    while (1) 
    {
        reg = sdio_io_readb(func0, SDIO_REG_CCCR_IO_RDY, &ret);
        if (ret)
            goto err;
        if (reg & (1 << func->num))
            break;
        ret = -RT_ETIMEOUT;
        if (rt_tick_get() > timeout)
            goto err;
    }

    mmcsd_dbg("SDIO: enabled function successfull\n");

    return 0;

err:
    mmcsd_dbg("SDIO: failed to enable function %d\n", func->num);
    return ret;
}

Int32 sdio_disable_func(struct rt_sdio_function *func)
{
    Int32 ret;
    UInt8 reg;
    struct rt_sdio_function *func0;

    assert(func != NULL);
    assert(func->card != NULL);

    func0 = func->card->sdio_function[0];

    mmcsd_dbg("SDIO: disabling function %d\n", func->num);

    reg =  sdio_io_readb(func0, SDIO_REG_CCCR_IO_EN, &ret);
    if (ret)
        goto err;

    reg &= ~(1 << func->num);

    ret = sdio_io_writeb(func0, SDIO_REG_CCCR_IO_EN, reg);
    if (ret)
        goto err;

    mmcsd_dbg("SDIO: disabled function successfull\n");

    return 0;

err:
    mmcsd_dbg("SDIO: failed to disable function %d\n", func->num);
    return -RT_EIO;
}

Int32 sdio_set_block_size(struct rt_sdio_function *func,
                               UInt32              blksize)
{
    Int32 ret;
    struct rt_sdio_function *func0 = func->card->sdio_function[0];

    if (blksize > func->card->host->max_blk_size)
        return -RT_ERROR;

    if (blksize == 0) 
    {
        blksize = MIN(func->max_blk_size, func->card->host->max_blk_size);
        blksize = MIN(blksize, 512u);
    }

    ret = sdio_io_writeb(func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_BLKSIZE, 
                 blksize & 0xff);
    if (ret)
        return ret;
    ret = sdio_io_writeb(func0, SDIO_REG_FBR_BASE(func->num) + SDIO_REG_FBR_BLKSIZE + 1, 
                 (blksize >> 8) & 0xff);
    if (ret)
        return ret;
    func->cur_blk_size = blksize;

    return 0;
}

rt_inline Int32 sdio_match_card(struct rt_mmcsd_card           *card,
                                     const struct rt_sdio_device_id *id)
{
    if ((id->manufacturer != SDIO_ANY_MAN_ID) && 
        (id->manufacturer != card->cis.manufacturer))
        return 0;
    if ((id->product != SDIO_ANY_PROD_ID) && 
        (id->product != card->cis.product))
        return 0;

    return 1;
}

static struct rt_mmcsd_card *sdio_match_driver(struct rt_sdio_device_id *id)
{
    rt_list_t *l;
    struct sdio_card *sc;
    struct rt_mmcsd_card *card;

    for (l = (&sdio_cards)->next; l != &sdio_cards; l = l->next)
    {
        sc = (struct sdio_card *)rt_list_entry(l, struct sdio_card, list);
        card = sc->card;

        if (sdio_match_card(card, id))
        {
            return card;
        }
    }

    return NULL;
}

Int32 sdio_register_driver(struct rt_sdio_driver *driver)
{
    struct sdio_driver *sd;
    struct rt_mmcsd_card *card;

    sd = malloc(sizeof(struct sdio_driver));
    if (sd == NULL)
    {
        printf("malloc sdio driver failed\n");

        return -RT_ENOMEM;
    }

    rt_list_insert_after(&sdio_drivers, &sd->list);

    if (!rt_list_isempty(&sdio_cards))
    {
        card = sdio_match_driver(driver->id);
        if (card != NULL)
        {
            driver->probe(card);
        }
    }

    return 0;
}

Int32 sdio_unregister_driver(struct rt_sdio_driver *driver)
{
    rt_list_t *l;
    struct sdio_driver *sd = NULL;
    struct rt_mmcsd_card *card;


    rt_list_insert_after(&sdio_drivers, &sd->list);

    for (l = (&sdio_drivers)->next; l != &sdio_drivers; l = l->next)
    {
        sd = (struct sdio_driver *)rt_list_entry(l, struct sdio_driver, list);
        if (sd->drv != driver)
        {
            sd = NULL;
        }
    }

    if (sd == NULL)
    {
        printf("SDIO driver %s not register\n", driver->name);
        return -RT_ERROR;
    }

    if (!rt_list_isempty(&sdio_cards))
    {
        card = sdio_match_driver(driver->id);
        if (card != NULL)
        {
            driver->remove(card);
            rt_list_remove(&sd->list);
            rt_free(sd);
        }
    }

    return 0;
}

void rt_sdio_init(void)
{
    rt_list_init(&sdio_cards);
    rt_list_init(&sdio_drivers);
}

