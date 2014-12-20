/*
 * File      : usb_device.h
 * 
 * COPYRIGHT (C) 2012, RT-Thread Development Team
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
 * 2012-10-01     Yi Qiu       first version
 * 2012-12-12     heyuanjie87  change endpoint and function handler
 * 2013-04-26     aozima       add DEVICEQUALIFIER support.
 */

#ifndef  __USB_DEVICE_H__
#define  __USB_DEVICE_H__

#include <hdElastos.h>
#include "usb_common.h"

/* Vendor ID */
#ifdef USB_VENDOR_ID
#define _VENDOR_ID              USB_VENDOR_ID
#else
#define _VENDOR_ID              0x0EFF
#endif
/* Product ID */
#ifdef USB_PRODUCT_ID
#define _PRODUCT_ID                 USB_PRODUCT_ID
#else
#define _PRODUCT_ID                 0x0001
#endif

#define USB_BCD_DEVICE              0x0200   /* USB Specification Release Number in Binary-Coded Decimal */
#define USB_BCD_VERSION             0x0200   /* USB 2.0 */
#define EP0_IN_ADDR                 0x80
#define EP0_OUT_ADDR                0x00
#define EP_HANDLER(ep, func, size)  assert(ep != NULL); ep->handler(func, size)
#define EP_ADDRESS(ep)              ep->ep_desc->bEndpointAddress
#define EP_MAXPACKET(ep)            ep->ep_desc->wMaxPacketSize
#define FUNC_ENABLE(func)           do{                                             \
                                        if(func->ops->enable != NULL &&          \
                                            func->enabled == RT_FALSE)              \
                                        {                                           \
                                            if(func->ops->enable(func) == RT_EOK)   \
                                                func->enabled = RT_TRUE;            \
                                        }                                           \
                                    }while(0)
#define FUNC_DISABLE(func)          do{                                             \
                                        if(func->ops->disable != NULL &&         \
                                            func->enabled == RT_TRUE)               \
                                        {                                           \
                                                func->enabled = RT_FALSE;           \
                                                func->ops->disable(func);           \
                                        }                                           \
                                    }while(0)

struct ufunction;
struct udevice;
struct uendpoint;

typedef enum 
{
    /* request to read full count */
    UIO_REQUEST_READ_FULL,
    /* request to read any count */
    UIO_REQUEST_READ_MOST,  
    /* request to write full count */
    UIO_REQUEST_WRITE,
}UIO_REQUEST_TYPE;

struct udcd_ops
{
    Int32 (*set_address)(UInt8 address);
    Int32 (*set_config)(UInt8 address);
    Int32 (*ep_set_stall)(UInt8 address);
    Int32 (*ep_clear_stall)(UInt8 address);
    Int32 (*ep_enable)(struct uendpoint* ep);
    Int32 (*ep_disable)(struct uendpoint* ep);
    UInt32 (*ep_read_prepare)(UInt8 address, void *buffer, UInt32 size);
    UInt32 (*ep_read)(UInt8 address, void *buffer);
    UInt32 (*ep_write)(UInt8 address, void *buffer, UInt32 size);
    Int32 (*ep0_send_status)(void);
    Int32 (*suspend)(void);
    Int32 (*wakeup)(void);    
};

struct ep_id
{
    UInt8 addr;
    UInt8 type;
    UInt8 dir;
    UInt8 maxpacket;
    UInt8 status;
};

typedef Int32 (*udep_handler_t)(struct ufunction* func, UInt32 size);

struct uio_request
{
    rt_list_t list;
    UIO_REQUEST_TYPE req_type;
    UInt8* buffer;
    UInt32 size;
    UInt32 remain_size;
};
typedef struct uio_request* uio_request_t;

struct uendpoint
{
    rt_list_t list;
    uep_desc_t ep_desc;
    rt_list_t request_list;
    struct uio_request request;
    UInt8* buffer;
    rt_bool_t stalled;
    struct ep_id* id;
    udep_handler_t handler;
    Int32 (*rx_indicate)(struct udevice* dev, UInt32 size);
};
typedef struct uendpoint* uep_t;

struct udcd
{
    struct rt_device parent;
    const struct udcd_ops* ops;
    struct uendpoint ep0;
    struct ep_id* ep_pool;
};
typedef struct udcd* udcd_t;

struct ualtsetting
{
    rt_list_t list;
    uintf_desc_t intf_desc;
    void* desc;
    UInt32 desc_size;
    rt_list_t ep_list;
};
typedef struct ualtsetting* ualtsetting_t;

typedef Int32 (*uintf_handler_t)(struct ufunction* func, ureq_t setup);

struct uinterface
{
    rt_list_t list;
    UInt8 intf_num;
    ualtsetting_t curr_setting;
    rt_list_t setting_list;
    uintf_handler_t handler;
};
typedef struct uinterface* uintf_t;

struct ufunction_ops
{
    Int32 (*enable)(struct ufunction* func);
    Int32 (*disable)(struct ufunction* func);
    Int32 (*sof_handler)(struct ufunction* func);
};
typedef struct ufunction_ops* ufunction_ops_t;

struct ufunction
{
    rt_list_t list;
    ufunction_ops_t ops;
    struct udevice* device;
    udev_desc_t dev_desc;
    void* user_data;
    rt_bool_t enabled;

    rt_list_t intf_list;
};
typedef struct ufunction* ufunction_t;

struct uconfig
{
    rt_list_t list;
    struct uconfig_descriptor cfg_desc;
    rt_list_t func_list;
};
typedef struct uconfig* uconfig_t;

struct udevice
{
    rt_list_t list;
    struct udevice_descriptor dev_desc;

    struct usb_qualifier_descriptor * dev_qualifier;
    const char** str;

    udevice_state_t state;
    rt_list_t cfg_list;
    uconfig_t curr_cfg;
    UInt8 nr_intf;

    udcd_t dcd;
};
typedef struct udevice* udevice_t;

enum udev_msg_type
{
    USB_MSG_SETUP_NOTIFY,
    USB_MSG_DATA_NOTIFY,
    USB_MSG_EP0_OUT,
    USB_MSG_EP_CLEAR_FEATURE,        
    USB_MSG_SOF,
    USB_MSG_RESET,
    USB_MSG_PLUG_IN,    
    /* we don't need to add a "PLUG_IN" event because after the cable is
     * plugged in(before any SETUP) the classed have nothing to do. If the host
     * is ready, it will send RESET and we will have USB_MSG_RESET. So, a RESET
     * should reset and run the class while plug_in is not. */
    USB_MSG_PLUG_OUT,
};
typedef enum udev_msg_type udev_msg_type;

struct ep_msg
{
    UInt32 size;
    UInt8 ep_addr;
};

struct udev_msg
{
    udev_msg_type type;
    udcd_t dcd;
    union
    {
        struct ep_msg ep_msg;
        struct urequest setup;
    } content;
};
typedef struct udev_msg* udev_msg_t;

udevice_t rt_usbd_device_new(void);
uconfig_t rt_usbd_config_new(void);
ufunction_t rt_usbd_function_new(udevice_t device, udev_desc_t dev_desc,
                              ufunction_ops_t ops);
uintf_t rt_usbd_interface_new(udevice_t device, uintf_handler_t handler);
uep_t rt_usbd_endpoint_new(uep_desc_t ep_desc, udep_handler_t handler);
ualtsetting_t rt_usbd_altsetting_new(UInt32 desc_size);

Int32 rt_usbd_core_init(void);
Int32 rt_usb_device_init(void);
Int32 rt_usbd_event_signal(struct udev_msg* msg);
Int32 rt_usbd_device_set_controller(udevice_t device, udcd_t dcd);
Int32 rt_usbd_device_set_descriptor(udevice_t device, udev_desc_t dev_desc);
Int32 rt_usbd_device_set_string(udevice_t device, const char** ustring);
Int32 rt_usbd_device_set_qualifier(udevice_t device, struct usb_qualifier_descriptor* qualifier);
Int32 rt_usbd_device_add_config(udevice_t device, uconfig_t cfg);
Int32 rt_usbd_config_add_function(uconfig_t cfg, ufunction_t func);
Int32 rt_usbd_function_add_interface(ufunction_t func, uintf_t intf);
Int32 rt_usbd_interface_add_altsetting(uintf_t intf, ualtsetting_t setting);
Int32 rt_usbd_altsetting_add_endpoint(ualtsetting_t setting, uep_t ep);
Int32 rt_usbd_altsetting_config_descriptor(ualtsetting_t setting, const void* desc, Int32 intf_pos);
Int32 rt_usbd_set_config(udevice_t device, UInt8 value);
Int32 rt_usbd_set_altsetting(uintf_t intf, UInt8 value);

udevice_t rt_usbd_find_device(udcd_t dcd);
uconfig_t rt_usbd_find_config(udevice_t device, UInt8 value);
uintf_t rt_usbd_find_interface(udevice_t device, UInt8 value, ufunction_t *pfunc);
uep_t rt_usbd_find_endpoint(udevice_t device, ufunction_t* pfunc, UInt8 ep_addr);
UInt32 rt_usbd_io_request(udevice_t device, uep_t ep, uio_request_t req);
UInt32 rt_usbd_ep0_write(udevice_t device, void *buffer, UInt32 size);
UInt32 rt_usbd_ep0_read(udevice_t device, void *buffer, UInt32 size, 
    Int32 (*rx_ind)(udevice_t device, UInt32 size));

ufunction_t rt_usbd_function_mstorage_create(udevice_t device);
ufunction_t rt_usbd_function_cdc_create(udevice_t device);
ufunction_t rt_usbd_function_rndis_create(udevice_t device);
ufunction_t rt_usbd_function_dap_create(udevice_t device);

#ifdef RT_USB_DEVICE_COMPOSITE
Int32 rt_usbd_function_set_iad(ufunction_t func, uiad_desc_t iad_desc);
#endif

Int32 rt_usbd_set_feature(udevice_t device, UInt16 value, UInt16 index);
Int32 rt_usbd_clear_feature(udevice_t device, UInt16 value, UInt16 index);
Int32 rt_usbd_ep_set_stall(udevice_t device, uep_t ep);
Int32 rt_usbd_ep_clear_stall(udevice_t device, uep_t ep);
Int32 rt_usbd_ep0_set_stall(udevice_t device);
Int32 rt_usbd_ep0_clear_stall(udevice_t device);
Int32 rt_usbd_ep0_setup_handler(udcd_t dcd, struct urequest* setup);
Int32 rt_usbd_ep0_in_handler(udcd_t dcd);
Int32 rt_usbd_ep0_out_handler(udcd_t dcd, UInt32 size);
Int32 rt_usbd_ep_in_handler(udcd_t dcd, UInt8 address);
Int32 rt_usbd_ep_out_handler(udcd_t dcd, UInt8 address, UInt32 size);
Int32 rt_usbd_reset_handler(udcd_t dcd);
Int32 rt_usbd_connect_handler(udcd_t dcd);
Int32 rt_usbd_disconnect_handler(udcd_t dcd);
Int32 rt_usbd_sof_handler(udcd_t dcd);

rt_inline Int32 dcd_set_address(udcd_t dcd, UInt8 address)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->set_address != NULL);

    return dcd->ops->set_address(address);
}

rt_inline Int32 dcd_set_config(udcd_t dcd, UInt8 address)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->set_config != NULL);

    return dcd->ops->set_config(address);
}

rt_inline Int32 dcd_ep_enable(udcd_t dcd, uep_t ep)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->ep_enable != NULL);

    return dcd->ops->ep_enable(ep);
}

rt_inline Int32 dcd_ep_disable(udcd_t dcd, uep_t ep)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->ep_disable != NULL);

    return dcd->ops->ep_disable(ep);
}

rt_inline UInt32 dcd_ep_read_prepare(udcd_t dcd, UInt8 address, void *buffer,
                               UInt32 size)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);

    if(dcd->ops->ep_read_prepare != NULL)
    {
        return dcd->ops->ep_read_prepare(address, buffer, size);
    }
    else
    {
        return 0;
    }
}

rt_inline UInt32 dcd_ep_read(udcd_t dcd, UInt8 address, void *buffer)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);

    if(dcd->ops->ep_read != NULL)
    {
        return dcd->ops->ep_read(address, buffer);
    }
    else
    {
        return 0;
    }
}

rt_inline UInt32 dcd_ep_write(udcd_t dcd, UInt8 address, void *buffer,
                                 UInt32 size)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->ep_write != NULL);

    return dcd->ops->ep_write(address, buffer, size);
}

rt_inline Int32 dcd_ep0_send_status(udcd_t dcd)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->ep0_send_status != NULL);

    return dcd->ops->ep0_send_status();
}

rt_inline Int32 dcd_ep_set_stall(udcd_t dcd, UInt8 address)
{    
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->ep_set_stall != NULL);

    return dcd->ops->ep_set_stall(address);
}

rt_inline Int32 dcd_ep_clear_stall(udcd_t dcd, UInt8 address)
{
    assert(dcd != NULL);
    assert(dcd->ops != NULL);
    assert(dcd->ops->ep_clear_stall != NULL);

    return dcd->ops->ep_clear_stall(address);
}

#endif
