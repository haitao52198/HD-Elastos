#include <hdElastosMantle.h>
#include "skeleton.h"
#include <netif/ethernetif.h>

#define MAX_ADDR_LEN	6
struct rt_skeleton_eth
{
    /* inherit from ethernet device */
    struct eth_device parent;

    /* interface address info. */
    UInt8  dev_addr[MAX_ADDR_LEN];		/* hw address	*/
};
static struct rt_skeleton_eth _skeleton_device;

void rt_skeleton_isr(int irqno)
{
	UInt32 status;

	/* read status */

    /* Received the coming packet */
    if (status)
    {
	    /* disable receive interrupt */

        /* a frame has been received */
        eth_device_ready(&(_skeleton_device.parent));
    }

    /* Transmit Interrupt check */
    if (status)
    {
	}
}

static Int32 rt_skeleton_init(rt_device_t dev)
{
    return RT_EOK;
}

static Int32 rt_skeleton_open(rt_device_t dev, UInt16 oflag)
{
    return RT_EOK;
}

static Int32 rt_skeleton_close(rt_device_t dev)
{
    return RT_EOK;
}

static UInt32 rt_skeleton_read(rt_device_t dev, Int32 pos, void* buffer, UInt32 size)
{
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static UInt32 rt_skeleton_write (rt_device_t dev, Int32 pos, const void* buffer, UInt32 size)
{
    rt_set_errno(-RT_ENOSYS);
    return 0;
}

static Int32 rt_skeleton_control(rt_device_t dev, UInt8 cmd, void *args)
{
	struct rt_skeleton_eth *eth;

	eth = (struct rt_skeleton_eth*)dev;
	assert(eth != NULL);

    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* get mac address */
        if (args) memcpy(args, _skeleton_device.dev_addr, 6);
        else return -RT_ERROR;
        break;

    default :
        break;
    }

    return RT_EOK;
}

Int32 rt_skeleton_tx( rt_device_t dev, struct pbuf* p)
{
    {
		/* q traverses through linked list of pbuf's
		 * This list MUST consist of a single packet ONLY */
		struct pbuf *q;
		UInt16 pbuf_index = 0;
		UInt8 word[2], word_index = 0;

		q = p;
		/* Write data into hardware, two bytes at a time
		 * Handling pbuf's with odd number of bytes correctly
		 * No attempt to optimize for speed has been made */
		while (q)
		{
			if (pbuf_index < q->len)
			{
				word[word_index++] = ((u8_t*)q->payload)[pbuf_index++];
				if (word_index == 2)
				{
					/* write data to hardware */

					word_index = 0;
				}
			}
			else
			{
				q = q->next;
				pbuf_index = 0;
			}
		}
		/* One byte could still be unsent */
		if (word_index == 1)
		{
			/* write data to hardware */

		}
    }

    return RT_EOK;
}

struct pbuf *rt_skeleton_rx(rt_device_t dev)
{
    struct pbuf* p;
    UInt32 rxbyte;

    /* init p pointer */
    p = NULL;
    rxbyte = 0;		/* get eth status */
    if (rxbyte)		/* there is data received */
    {
        UInt16 rx_len;
        UInt16* data;

		/* get recieved packet length */
        rx_len = 0;

        /* allocate buffer */
        p = pbuf_alloc(PBUF_LINK, rx_len, PBUF_RAM);
        if (p != NULL)
        {
            struct pbuf* q;
            Int32 len;

            for (q = p; q != NULL; q= q->next)
            {
                data = (UInt16*)q->payload;
                len = q->len;

                while (len > 0)
                {
                    *data = 0; /* read data from hardware */
                    data ++;
                    len -= 2;
                }
            }
        }
        else
        {
            UInt16 dummy;
			Int32 len;

            /* no pbuf, discard data from EMAC */
            data = &dummy;
			len = rx_len;
            while (len > 0)
            {
                *data = 0; /* read from hardware */
                len -= 2;
            }
        }
    }
    else
    {
        /* restore receive interrupt */
    }

    return p;
}

void rt_hw_skeleton_init()
{
	/* set default MAC address */
    _skeleton_device.dev_addr[0] = 0x00;
    _skeleton_device.dev_addr[1] = 0x60;
    _skeleton_device.dev_addr[2] = 0x6E;
    _skeleton_device.dev_addr[3] = 0x11;
    _skeleton_device.dev_addr[4] = 0x22;
    _skeleton_device.dev_addr[5] = 0x33;

	/* set RT-Thread device interface */
    _skeleton_device.parent.parent.init       = rt_skeleton_init;
    _skeleton_device.parent.parent.open       = rt_skeleton_open;
    _skeleton_device.parent.parent.close      = rt_skeleton_close;
    _skeleton_device.parent.parent.read       = rt_skeleton_read;
    _skeleton_device.parent.parent.write      = rt_skeleton_write;
    _skeleton_device.parent.parent.control    = rt_skeleton_control;
    _skeleton_device.parent.parent.user_data  = NULL;

	/* set ethernet interface */
    _skeleton_device.parent.eth_rx     = rt_skeleton_rx;
    _skeleton_device.parent.eth_tx     = rt_skeleton_tx;

    eth_device_init(&(_skeleton_device.parent), "e0");
}
