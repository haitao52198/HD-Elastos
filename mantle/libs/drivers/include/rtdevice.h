/*
 * File      : rtdevice.h
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
 * 2012-01-08     bernard      first version.
 * 2014-07-12     bernard      Add workqueue implementation.
 */

#ifndef __RT_DEVICE_H__
#define __RT_DEVICE_H__

#include <hdElastos.h>

#define RT_DEVICE(device)            ((rt_device_t)device)

/* completion flag */
struct rt_completion
{
    UInt32 flag;

    /* suspended list */
    rt_list_t suspended_list;
};

/* ring buffer */
struct rt_ringbuffer
{
    UInt8 *buffer_ptr;
    /* use the msb of the {read,write}_index as mirror bit. You can see this as
     * if the buffer adds a virtual mirror and the pointers point either to the
     * normal or to the mirrored buffer. If the write_index has the same value
     * with the read_index, but in a different mirror, the buffer is full.
     * While if the write_index and the read_index are the same and within the
     * same mirror, the buffer is empty. The ASCII art of the ringbuffer is:
     *
     *          mirror = 0                    mirror = 1
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Full
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     *  read_idx-^                   write_idx-^
     *
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * | 0 | 1 | 2 | 3 | 4 | 5 | 6 ||| 0 | 1 | 2 | 3 | 4 | 5 | 6 | Empty
     * +---+---+---+---+---+---+---+|+~~~+~~~+~~~+~~~+~~~+~~~+~~~+
     * read_idx-^ ^-write_idx
     *
     * The tradeoff is we could only use 32KiB of buffer for 16 bit of index.
     * But it should be enough for most of the cases.
     *
     * Ref: http://en.wikipedia.org/wiki/Circular_buffer#Mirroring */
    UInt16 read_mirror : 1;
    UInt16 read_index : 15;
    UInt16 write_mirror : 1;
    UInt16 write_index : 15;
    /* as we use msb of index as mirror bit, the size should be signed and
     * could only be positive. */
    rt_int16_t buffer_size;
};

/* portal device */
struct rt_portal_device
{
    struct rt_device parent;
    struct rt_device *write_dev;
    struct rt_device *read_dev;
};

/* pipe device */
#define PIPE_DEVICE(device)          ((struct rt_pipe_device*)(device))
enum rt_pipe_flag
{
    /* both read and write won't block */
    RT_PIPE_FLAG_NONBLOCK_RDWR = 0x00,
    /* read would block */
    RT_PIPE_FLAG_BLOCK_RD = 0x01,
    /* write would block */
    RT_PIPE_FLAG_BLOCK_WR = 0x02,
    /* write to this pipe will discard some data when the pipe is full.
     * When this flag is set, RT_PIPE_FLAG_BLOCK_WR will be ignored since write
     * operation will always be success. */
    RT_PIPE_FLAG_FORCE_WR = 0x04,
};

struct rt_pipe_device
{
    struct rt_device parent;

    /* ring buffer in pipe device */
    struct rt_ringbuffer ringbuffer;

    enum rt_pipe_flag flag;

    /* suspended list */
    rt_list_t suspended_read_list;
    rt_list_t suspended_write_list;

    struct rt_portal_device *write_portal;
    struct rt_portal_device *read_portal;
};

#define PIPE_CTRL_GET_SPACE          0x14            /**< get the remaining size of a pipe device */

#define RT_DATAQUEUE_EVENT_UNKNOWN   0x00
#define RT_DATAQUEUE_EVENT_POP       0x01
#define RT_DATAQUEUE_EVENT_PUSH      0x02
#define RT_DATAQUEUE_EVENT_LWM       0x03

struct rt_data_item;
#define RT_DATAQUEUE_SIZE(dq)        ((dq)->put_index - (dq)->get_index)
#define RT_DATAQUEUE_EMPTY(dq)       ((dq)->size - RT_DATAQUEUE_SIZE(dq))
/* data queue implementation */
struct rt_data_queue
{
    UInt16 size;
    UInt16 lwm;
    rt_bool_t   waiting_lwm;

    UInt16 get_index;
    UInt16 put_index;

    struct rt_data_item *queue;

    rt_list_t suspended_push_list;
    rt_list_t suspended_pop_list;

    /* event notify */
    void (*evt_notify)(struct rt_data_queue *queue, UInt32 event);
};

/* workqueue implementation */
struct rt_workqueue
{
	rt_list_t   work_list;
	rt_thread_t work_thread;
};

struct rt_work
{
	rt_list_t list;

	void (*work_func)(struct rt_work* work, void* work_data);
	void *work_data;
};

/**
 * Completion
 */
void rt_completion_init(struct rt_completion *completion);
Int32 rt_completion_wait(struct rt_completion *completion,
                            Int32            timeout);
void rt_completion_done(struct rt_completion *completion);

/**
 * RingBuffer for DeviceDriver
 *
 * Please note that the ring buffer implementation of RT-Thread
 * has no thread wait or resume feature.
 */
void rt_ringbuffer_init(struct rt_ringbuffer *rb,
                        UInt8           *pool,
                        rt_int16_t            size);
UInt32 rt_ringbuffer_put(struct rt_ringbuffer *rb,
                            const UInt8     *ptr,
                            UInt16           length);
UInt32 rt_ringbuffer_put_force(struct rt_ringbuffer *rb,
                                  const UInt8     *ptr,
                                  UInt16           length);
UInt32 rt_ringbuffer_putchar(struct rt_ringbuffer *rb,
                                const UInt8      ch);
UInt32 rt_ringbuffer_putchar_force(struct rt_ringbuffer *rb,
                                      const UInt8      ch);
UInt32 rt_ringbuffer_get(struct rt_ringbuffer *rb,
                            UInt8           *ptr,
                            UInt16           length);
UInt32 rt_ringbuffer_getchar(struct rt_ringbuffer *rb, UInt8 *ch);

enum rt_ringbuffer_state
{
    RT_RINGBUFFER_EMPTY,
    RT_RINGBUFFER_FULL,
    /* half full is neither full nor empty */
    RT_RINGBUFFER_HALFFULL,
};

rt_inline UInt16 rt_ringbuffer_get_size(struct rt_ringbuffer *rb)
{
    assert(rb != NULL);
    return rb->buffer_size;
}

rt_inline enum rt_ringbuffer_state
rt_ringbuffer_status(struct rt_ringbuffer *rb)
{
    if (rb->read_index == rb->write_index)
    {
        if (rb->read_mirror == rb->write_mirror)
            return RT_RINGBUFFER_EMPTY;
        else
            return RT_RINGBUFFER_FULL;
    }
    return RT_RINGBUFFER_HALFFULL;
}

/** return the size of data in rb */
rt_inline UInt16 rt_ringbuffer_data_len(struct rt_ringbuffer *rb)
{
    switch (rt_ringbuffer_status(rb))
    {
    case RT_RINGBUFFER_EMPTY:
        return 0;
    case RT_RINGBUFFER_FULL:
        return rb->buffer_size;
    case RT_RINGBUFFER_HALFFULL:
    default:
        if (rb->write_index > rb->read_index)
            return rb->write_index - rb->read_index;
        else
            return rb->buffer_size - (rb->read_index - rb->write_index);
    };
}

/** return the size of empty space in rb */
#define rt_ringbuffer_space_len(rb) ((rb)->buffer_size - rt_ringbuffer_data_len(rb))

/**
 * Pipe Device
 */
Int32 rt_pipe_init(struct rt_pipe_device *pipe,
                      const char *name,
                      enum rt_pipe_flag flag,
                      UInt8 *buf,
                      UInt32 size);
Int32 rt_pipe_detach(struct rt_pipe_device *pipe);
#ifdef RT_USING_HEAP
Int32 rt_pipe_create(const char *name, enum rt_pipe_flag flag, UInt32 size);
void rt_pipe_destroy(struct rt_pipe_device *pipe);
#endif

/**
 * Portal for DeviceDriver
 */

Int32 rt_portal_init(struct rt_portal_device *portal,
                        const char *portal_name,
                        const char *write_dev,
                        const char *read_dev);
Int32 rt_portal_detach(struct rt_portal_device *portal);

#ifdef RT_USING_HEAP
Int32 rt_portal_create(const char *name,
                          const char *write_dev,
                          const char *read_dev);
void rt_portal_destroy(struct rt_portal_device *portal);
#endif

/**
 * DataQueue for DeviceDriver
 */
Int32 rt_data_queue_init(struct rt_data_queue *queue,
                            UInt16           size,
                            UInt16           lwm,
                            void (*evt_notify)(struct rt_data_queue *queue, UInt32 event));
Int32 rt_data_queue_push(struct rt_data_queue *queue,
                            const void           *data_ptr,
                            UInt32             data_size,
                            Int32            timeout);
Int32 rt_data_queue_pop(struct rt_data_queue *queue,
                           const void          **data_ptr,
                           UInt32            *size,
                           Int32            timeout);
Int32 rt_data_queue_peak(struct rt_data_queue *queue,
                            const void          **data_ptr,
                            UInt32            *size);
void rt_data_queue_reset(struct rt_data_queue *queue);

#ifdef RT_USING_HEAP
/**
 * WorkQueue for DeviceDriver
 */
struct rt_workqueue *rt_workqueue_create(const char* name, UInt16 stack_size, UInt8 priority);
Int32 rt_workqueue_destroy(struct rt_workqueue* queue);
Int32 rt_workqueue_dowork(struct rt_workqueue* queue, struct rt_work* work);
Int32 rt_workqueue_cancel_work(struct rt_workqueue* queue, struct rt_work* work);

rt_inline void rt_work_init(struct rt_work* work, void (*work_func)(struct rt_work* work, void* work_data), 
    void* work_data)
{
    rt_list_init(&(work->list));
    work->work_func = work_func;
    work->work_data = work_data;
}
#endif

#ifdef RT_USING_RTC
#include "drivers/rtc.h"
#ifdef RT_USING_ALARM
#include "drivers/alarm.h"
#endif
#endif /* RT_USING_RTC */

#ifdef RT_USING_SPI
#include "drivers/spi.h"
#endif /* RT_USING_SPI */

#ifdef RT_USING_MTD_NOR
#include "drivers/mtd_nor.h"
#endif /* RT_USING_MTD_NOR */

#ifdef RT_USING_MTD_NAND
#include "drivers/mtd_nand.h"
#endif /* RT_USING_MTD_NAND */

#ifdef RT_USING_USB_DEVICE
#include "drivers/usb_device.h"
#endif /* RT_USING_USB_DEVICE */

#ifdef RT_USING_USB_HOST
#include "drivers/usb_host.h"
#endif /* RT_USING_USB_HOST */

#ifdef RT_USING_SERIAL
#include "drivers/serial.h"
#endif /* RT_USING_SERIAL */

#ifdef RT_USING_I2C
#include "drivers/i2c.h"
#include "drivers/i2c_dev.h"

#ifdef RT_USING_I2C_BITOPS
#include "drivers/i2c-bit-ops.h"
#endif /* RT_USING_I2C_BITOPS */
#endif /* RT_USING_I2C */

#ifdef RT_USING_SDIO
#include "drivers/mmcsd_core.h"
#include "drivers/sd.h"
#include "drivers/sdio.h"
#endif

#ifdef RT_USING_WDT
#include "drivers/watchdog.h"
#endif

#endif /* __RT_DEVICE_H__ */

