/*
 * File      : serial.c
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
 * 2006-03-13     bernard      first version
 * 2012-05-15     lgnq         modified according bernard's implementation.
 * 2012-05-28     bernard      code cleanup
 * 2012-11-23     bernard      fix compiler warning.
 * 2013-02-20     bernard      use RT_SERIAL_RB_BUFSZ to define
 *                             the size of ring buffer.
 * 2014-07-10     bernard      rewrite serial framework
 */

#include <rthw.h>
#include <hdElastosMantle.h>
#include <rtdevice.h>

/*
 * Serial poll routines
 */
rt_inline int _serial_poll_rx(struct rt_serial_device *serial, UInt8 *data, int length)
{
    int ch;
    int size;

    assert(serial != NULL);
    size = length;

    while (length)
    {
        ch = serial->ops->getc(serial);
        *data = ch;
        data ++; length --;

        if (ch == '\n') break;
    }

    return size - length;
}

rt_inline int _serial_poll_tx(struct rt_serial_device *serial, const UInt8 *data, int length)
{
    int size;
    assert(serial != NULL);

    size = length;
    while (length)
    {
        /*
         * to be polite with serial console add a line feed
         * to the carriage return character
         */
        if (*data == '\n' && (serial->parent.flag & RT_DEVICE_FLAG_STREAM))
        {
            serial->ops->putc(serial, '\r');
        }

        serial->ops->putc(serial, *data);

        ++ data;
        -- length;
    }

    return size - length;
}

/*
 * Serial interrupt routines
 */
rt_inline int _serial_int_rx(struct rt_serial_device *serial, UInt8 *data, int length)
{
    int size;
    struct rt_serial_rx_fifo* rx_fifo;

    assert(serial != NULL);
    size = length;

    rx_fifo = (struct rt_serial_rx_fifo*) serial->serial_rx;
    assert(rx_fifo != NULL);

    /* read from software FIFO */
    while (length)
    {
        int ch;
        rt_base_t level;

        /* disable interrupt */
        level = rt_hw_interrupt_disable();
        if (rx_fifo->get_index != rx_fifo->put_index)
        {
            ch = rx_fifo->buffer[rx_fifo->get_index];
            rx_fifo->get_index += 1;
            if (rx_fifo->get_index >= serial->config.bufsz) rx_fifo->get_index = 0;
        }
        else
        {
            /* no data, enable interrupt and break out */
            rt_hw_interrupt_enable(level);
            break;
        }

        /* enable interrupt */
        rt_hw_interrupt_enable(level);

        *data = ch & 0xff;
        data ++; length --;
    }

    return size - length;
}

rt_inline int _serial_int_tx(struct rt_serial_device *serial, const UInt8 *data, int length)
{
    int size;
    struct rt_serial_tx_fifo *tx;

    assert(serial != NULL);

    size = length;
    tx = (struct rt_serial_tx_fifo*) serial->serial_tx;
    assert(tx != NULL);

    while (length)
    {
        if (serial->ops->putc(serial, *(char*)data) == -1)
        {
            rt_completion_wait(&(tx->completion), RT_WAITING_FOREVER);
            continue;
        }

        data ++; length --;
    }

    return size - length;
}

/*
 * Serial DMA routines
 */
rt_inline int _serial_dma_rx(struct rt_serial_device *serial, UInt8 *data, int length)
{
    rt_base_t level;
    int result = RT_EOK;
    struct rt_serial_rx_dma *rx_dma;

    assert((serial != NULL) && (data != NULL));
    rx_dma = (struct rt_serial_rx_dma*)serial->serial_rx;
    assert(rx_dma != NULL);

    level = rt_hw_interrupt_disable();
    if (rx_dma->activated != RT_TRUE)
    {
        rx_dma->activated = RT_TRUE;
        serial->ops->dma_transmit(serial, data, length, RT_SERIAL_DMA_RX);
    }
    else result = -RT_EBUSY;
    rt_hw_interrupt_enable(level);

    if (result == RT_EOK) return length;

    rt_set_errno(result);
    return 0;
}

rt_inline int _serial_dma_tx(struct rt_serial_device *serial, const UInt8 *data, int length)
{
    rt_base_t level;
    Int32 result;
    struct rt_serial_tx_dma *tx_dma;

    tx_dma = (struct rt_serial_tx_dma*)(serial->serial_tx);

    result = rt_data_queue_push(&(tx_dma->data_queue), data, length, RT_WAITING_FOREVER);
    if (result == RT_EOK)
    {
        level = rt_hw_interrupt_disable();
        if (tx_dma->activated != RT_TRUE)
        {
            tx_dma->activated = RT_TRUE;
            rt_hw_interrupt_enable(level);

            /* make a DMA transfer */
            serial->ops->dma_transmit(serial, data, length, RT_SERIAL_DMA_TX);
        }
        else
        {
            rt_hw_interrupt_enable(level);
        }

        return length;
    }
    else
    {
        rt_set_errno(result);
        return 0;
    }
}

/* RT-Thread Device Interface */
/*
 * This function initializes serial device.
 */
static Int32 rt_serial_init(struct rt_device *dev)
{
    Int32 result = RT_EOK;
    struct rt_serial_device *serial;

    assert(dev != NULL);
    serial = (struct rt_serial_device *)dev;

    /* initialize rx/tx */
    serial->serial_rx = NULL;
    serial->serial_tx = NULL;

    /* apply configuration */
    if (serial->ops->configure)
        result = serial->ops->configure(serial, &serial->config);

    return result;
}

static Int32 rt_serial_open(struct rt_device *dev, UInt16 oflag)
{
    struct rt_serial_device *serial;

    assert(dev != NULL);
    serial = (struct rt_serial_device *)dev;

    /* check device flag with the open flag */
    if ((oflag & RT_DEVICE_FLAG_DMA_RX) && !(dev->flag & RT_DEVICE_FLAG_DMA_RX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_DMA_TX) && !(dev->flag & RT_DEVICE_FLAG_DMA_TX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_INT_RX) && !(dev->flag & RT_DEVICE_FLAG_INT_RX))
        return -RT_EIO;
    if ((oflag & RT_DEVICE_FLAG_INT_TX) && !(dev->flag & RT_DEVICE_FLAG_INT_TX))
        return -RT_EIO;

    /* get open flags */
    dev->open_flag = oflag & 0xff;

    /* initialize the Rx/Tx structure according to open flag */
    if (serial->serial_rx == NULL)
    {
        if (oflag & RT_DEVICE_FLAG_DMA_RX)
        {
            struct rt_serial_rx_dma* rx_dma;

            rx_dma = (struct rt_serial_rx_dma*) malloc (sizeof(struct rt_serial_rx_dma));
            assert(rx_dma != NULL);
            rx_dma->activated = RT_FALSE;

            serial->serial_rx = rx_dma;
            dev->open_flag |= RT_DEVICE_FLAG_DMA_RX;
        }
        else if (oflag & RT_DEVICE_FLAG_INT_RX)
        {
            struct rt_serial_rx_fifo* rx_fifo;

            rx_fifo = (struct rt_serial_rx_fifo*) malloc (sizeof(struct rt_serial_rx_fifo) +
                serial->config.bufsz);
            assert(rx_fifo != NULL);
            rx_fifo->buffer = (UInt8*) (rx_fifo + 1);
            memset(rx_fifo->buffer, 0, RT_SERIAL_RB_BUFSZ);
            rx_fifo->put_index = 0;
            rx_fifo->get_index = 0;

            serial->serial_rx = rx_fifo;
            dev->open_flag |= RT_DEVICE_FLAG_INT_RX;
            /* configure low level device */
            serial->ops->control(serial, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_RX);
        }
        else
        {
            serial->serial_rx = NULL;
        }
    }

    if (serial->serial_tx == NULL)
    {
        if (oflag & RT_DEVICE_FLAG_DMA_TX)
        {
            struct rt_serial_tx_dma* tx_dma;

            tx_dma = (struct rt_serial_tx_dma*) malloc (sizeof(struct rt_serial_tx_dma));
            assert(tx_dma != NULL);

            rt_data_queue_init(&(tx_dma->data_queue), 8, 4, NULL);
            serial->serial_tx = tx_dma;

            dev->open_flag |= RT_DEVICE_FLAG_DMA_TX;
        }
        else if (oflag & RT_DEVICE_FLAG_INT_TX)
        {
            struct rt_serial_tx_fifo *tx_fifo;

            tx_fifo = (struct rt_serial_tx_fifo*) malloc(sizeof(struct rt_serial_tx_fifo));
            assert(tx_fifo != NULL);

            rt_completion_init(&(tx_fifo->completion));
            serial->serial_tx = tx_fifo;

            dev->open_flag |= RT_DEVICE_FLAG_INT_TX;
            /* configure low level device */
            serial->ops->control(serial, RT_DEVICE_CTRL_SET_INT, (void *)RT_DEVICE_FLAG_INT_TX);
        }
        else
        {
            serial->serial_tx = NULL;
        }
    }

    return RT_EOK;
}

static Int32 rt_serial_close(struct rt_device *dev)
{
    struct rt_serial_device *serial;

    assert(dev != NULL);
    serial = (struct rt_serial_device *)dev;

    /* this device has more reference count */
    if (dev->ref_count > 1) return RT_EOK;

    if (dev->open_flag & RT_DEVICE_FLAG_INT_RX)
    {
        struct rt_serial_rx_fifo* rx_fifo;

        rx_fifo = (struct rt_serial_rx_fifo*)serial->serial_rx;
        assert(rx_fifo != NULL);

        rt_free(rx_fifo);
        serial->serial_rx = NULL;
        dev->open_flag &= ~RT_DEVICE_FLAG_INT_RX;
        /* configure low level device */
        serial->ops->control(serial, RT_DEVICE_CTRL_CLR_INT, (void*)RT_DEVICE_FLAG_INT_TX);
    }
    else if (dev->open_flag & RT_DEVICE_FLAG_DMA_RX)
    {
        struct rt_serial_rx_dma* rx_dma;

        rx_dma = (struct rt_serial_rx_dma*)serial->serial_tx;
        assert(rx_dma != NULL);

        rt_free(rx_dma);
        serial->serial_rx = NULL;
        dev->open_flag &= ~RT_DEVICE_FLAG_DMA_RX;
    }

    if (dev->open_flag & RT_DEVICE_FLAG_INT_TX)
    {
        struct rt_serial_tx_fifo* tx_fifo;

        tx_fifo = (struct rt_serial_tx_fifo*)serial->serial_rx;
        assert(tx_fifo != NULL);

        rt_free(tx_fifo);
        serial->serial_tx = NULL;
        dev->open_flag &= ~RT_DEVICE_FLAG_INT_TX;
        /* configure low level device */
        serial->ops->control(serial, RT_DEVICE_CTRL_CLR_INT, (void*)RT_DEVICE_FLAG_INT_TX);
    }
    else if (dev->open_flag & RT_DEVICE_FLAG_DMA_TX)
    {
        struct rt_serial_tx_dma* tx_dma;

        tx_dma = (struct rt_serial_tx_dma*)serial->serial_tx;
        assert(tx_dma != NULL);

        rt_free(tx_dma);
        serial->serial_tx = NULL;
        dev->open_flag &= ~RT_DEVICE_FLAG_DMA_TX;
    }

    return RT_EOK;
}

static UInt32 rt_serial_read(struct rt_device *dev,
                                Int32          pos,
                                void             *buffer,
                                UInt32         size)
{
    struct rt_serial_device *serial;

    assert(dev != NULL);
    if (size == 0) return 0;

    serial = (struct rt_serial_device *)dev;

    if (dev->open_flag & RT_DEVICE_FLAG_INT_RX)
    {
        return _serial_int_rx(serial, buffer, size);
    }
    else if (dev->open_flag & RT_DEVICE_FLAG_DMA_RX)
    {
        return _serial_dma_rx(serial, buffer, size);
    }

    return _serial_poll_rx(serial, buffer, size);
}

static UInt32 rt_serial_write(struct rt_device *dev,
                                 Int32          pos,
                                 const void       *buffer,
                                 UInt32         size)
{
    struct rt_serial_device *serial;

    assert(dev != NULL);
    if (size == 0) return 0;

    serial = (struct rt_serial_device *)dev;

    if (dev->open_flag & RT_DEVICE_FLAG_INT_TX)
    {
        return _serial_int_tx(serial, buffer, size);
    }
    else if (dev->open_flag & RT_DEVICE_FLAG_DMA_TX)
    {
        return _serial_dma_tx(serial, buffer, size);
    }
    else
    {
        return _serial_poll_tx(serial, buffer, size);
    }
}

static Int32 rt_serial_control(struct rt_device *dev,
                                  UInt8        cmd,
                                  void             *args)
{
    struct rt_serial_device *serial;

    assert(dev != NULL);
    serial = (struct rt_serial_device *)dev;

    switch (cmd)
    {
        case RT_DEVICE_CTRL_SUSPEND:
            /* suspend device */
            dev->flag |= RT_DEVICE_FLAG_SUSPENDED;
            break;

        case RT_DEVICE_CTRL_RESUME:
            /* resume device */
            dev->flag &= ~RT_DEVICE_FLAG_SUSPENDED;
            break;

        case RT_DEVICE_CTRL_CONFIG:
            /* configure device */
            serial->ops->configure(serial, (struct serial_configure *)args);
            break;

        default :
            /* control device */
            serial->ops->control(serial, cmd, args);
            break;
    }

    return RT_EOK;
}

/*
 * serial register
 */
Int32 rt_hw_serial_register(struct rt_serial_device *serial,
                               const char              *name,
                               UInt32              flag,
                               void                    *data)
{
    struct rt_device *device;
    assert(serial != NULL);

    device = &(serial->parent);

    device->type        = RT_Device_Class_Char;
    device->rx_indicate = NULL;
    device->tx_complete = NULL;

    device->init        = rt_serial_init;
    device->open        = rt_serial_open;
    device->close       = rt_serial_close;
    device->read        = rt_serial_read;
    device->write       = rt_serial_write;
    device->control     = rt_serial_control;
    device->user_data   = data;

    /* register a character device */
    return rt_device_register(device, name, flag);
}

/* ISR for serial interrupt */
void rt_hw_serial_isr(struct rt_serial_device *serial, int event)
{
    switch (event & 0xff)
    {
        case RT_SERIAL_EVENT_RX_IND:
        {
            int ch = -1;
            rt_base_t level;
            struct rt_serial_rx_fifo* rx_fifo;

            rx_fifo = (struct rt_serial_rx_fifo*)serial->serial_rx;
            assert(rx_fifo != NULL);

            /* interrupt mode receive */
            assert(serial->parent.open_flag & RT_DEVICE_FLAG_INT_RX);

            while (1)
            {
                ch = serial->ops->getc(serial);
                if (ch == -1) break;


                /* disable interrupt */
                level = rt_hw_interrupt_disable();

                rx_fifo->buffer[rx_fifo->put_index] = ch;
                rx_fifo->put_index += 1;
                if (rx_fifo->put_index >= serial->config.bufsz) rx_fifo->put_index = 0;

                /* if the next position is read index, discard this 'read char' */
                if (rx_fifo->put_index == rx_fifo->get_index)
                {
                    rx_fifo->get_index += 1;
                    if (rx_fifo->get_index >= serial->config.bufsz) rx_fifo->get_index = 0;
                }

                /* enable interrupt */
                rt_hw_interrupt_enable(level);
            }

            /* invoke callback */
            if (serial->parent.rx_indicate != NULL)
            {
                UInt32 rx_length;

                /* get rx length */
                level = rt_hw_interrupt_disable();
                rx_length = (rx_fifo->put_index >= rx_fifo->get_index)? (rx_fifo->put_index - rx_fifo->get_index):
                    (serial->config.bufsz - (rx_fifo->get_index - rx_fifo->put_index));
                rt_hw_interrupt_enable(level);

                serial->parent.rx_indicate(&serial->parent, rx_length);
            }
            break;
        }
        case RT_SERIAL_EVENT_TX_DONE:
        {
            struct rt_serial_tx_fifo* tx_fifo;

            tx_fifo = (struct rt_serial_tx_fifo*)serial->serial_tx;
            rt_completion_done(&(tx_fifo->completion));
            break;
        }
        case RT_SERIAL_EVENT_TX_DMADONE:
        {
            const void *data_ptr;
            UInt32 data_size;
            const void *last_data_ptr;
            struct rt_serial_tx_dma* tx_dma;

            tx_dma = (struct rt_serial_tx_dma*) serial->serial_tx;

            rt_data_queue_pop(&(tx_dma->data_queue), &last_data_ptr, &data_size, 0);
            if (rt_data_queue_peak(&(tx_dma->data_queue), &data_ptr, &data_size) == RT_EOK)
            {
                /* transmit next data node */
                tx_dma->activated = RT_TRUE;
                serial->ops->dma_transmit(serial, data_ptr, data_size, RT_SERIAL_DMA_TX);
            }
            else
            {
                tx_dma->activated = RT_FALSE;
            }

            /* invoke callback */
            if (serial->parent.tx_complete != NULL)
            {
                serial->parent.tx_complete(&serial->parent, (void*)last_data_ptr);
            }
            break;
        }
        case RT_SERIAL_EVENT_RX_DMADONE:
        {
            int length;
            struct rt_serial_rx_dma* rx_dma;

            rx_dma = (struct rt_serial_rx_dma*)serial->serial_rx;
            /* get DMA rx length */
            length = (event & (~0xff)) >> 8;
            serial->parent.rx_indicate(&(serial->parent), length);
            rx_dma->activated = RT_FALSE;
            break;
        }
    }
}
