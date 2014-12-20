/*
 * File      : usb_host.h
 * 
 * COPYRIGHT (C) 2011, RT-Thread Development Team
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
 * 2011-3-12     Yi Qiu      first version
 */

#ifndef __RT_USB_HOST_H__
#define __RT_USB_HOST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <hdElastos.h>
#include "usb_common.h"

#define USB_MAX_DEVICE                  0x20
#define USB_MAX_INTERFACE               0x08
#define USB_HUB_PORT_NUM                0x04
#define SIZEOF_USB_REQUEST              0x08

#define DEV_STATUS_IDLE               0x00
#define DEV_STATUS_BUSY               0x01
#define DEV_STATUS_ERROR              0x02

#define UPIPE_STATUS_OK                 0x00
#define UPIPE_STATUS_STALL              0x01
#define UPIPE_STATUS_ERROR              0x02

struct uhcd;
struct uintf;
struct uhub;

struct uclass_driver
{
    rt_list_t list;
    int class_code;
    int subclass_code;
    
    Int32 (*enable)(void* arg);
    Int32 (*disable)(void* arg);
    
    void* user_data;
};
typedef struct uclass_driver* ucd_t;

struct uprotocal
{
    rt_list_t list;
    int pro_id;
    
    Int32 (*init)(void* arg);
    Int32 (*callback)(void* arg);    
};
typedef struct uprotocal* uprotocal_t;

struct uinstance
{
    struct udevice_descriptor dev_desc;
    ucfg_desc_t cfg_desc;
    struct uhcd *hcd;

    UInt8 status;
    UInt8 type;
    UInt8 index;
    UInt8 address;    
    UInt8 speed;
    UInt8 max_packet_size;    
    UInt8 port;

    struct uhub* parent;
    struct uintf* intf[USB_MAX_INTERFACE];        
};
typedef struct uinstance* uinst_t;

struct uintf
{
    struct uinstance* device;
    uintf_desc_t intf_desc;

    ucd_t drv;
    void *user_data;
};

struct upipe
{
    UInt32 status;
    struct uendpoint_descriptor ep;
    struct uintf* intf;
    func_callback callback;
    void* user_data;
};
typedef struct upipe* upipe_t;

struct uhub
{
    struct uhub_descriptor hub_desc;
    UInt8 num_ports;
    UInt32 port_status[USB_HUB_PORT_NUM];
    struct uinstance* child[USB_HUB_PORT_NUM];        

    rt_bool_t is_roothub;
    upipe_t pipe_in;
    UInt8 buffer[8];    
    struct uinstance* self;
    struct uhcd *hcd;
};    
typedef struct uhub* uhub_t;

struct uhcd_ops
{
    int (*ctl_xfer)(struct uinstance* inst, ureq_t setup, void* buffer, int nbytes, 
        int timeout);
    int (*bulk_xfer)(upipe_t pipe, void* buffer, int nbytes, int timeout);
    int (*int_xfer)(upipe_t pipe, void* buffer, int nbytes, int timeout);
    int (*iso_xfer)(upipe_t pipe, void* buffer, int nbytes, int timeout);
    
    Int32 (*alloc_pipe)(struct upipe** pipe, struct uintf* intf, uep_desc_t ep, 
        func_callback callback);
    Int32 (*free_pipe)(upipe_t pipe);    
    Int32 (*hub_ctrl)(UInt16 port, UInt8 cmd, void *args);    
};

struct uhcd
{
    struct rt_device parent;
    struct uhcd_ops* ops;
    struct uhub* roothub; 
};
typedef struct uhcd* uhcd_t;

enum uhost_msg_type
{
    USB_MSG_CONNECT_CHANGE,
    USB_MSG_CALLBACK,
};
typedef enum uhost_msg_type uhost_msg_type;

struct uhost_msg
{
    uhost_msg_type type; 
    union
    {
        struct uhub* hub;
        struct 
        {
            func_callback function;
            void *context;
        }cb;
    }content;
};
typedef struct uhost_msg* uhost_msg_t;

/* usb host system interface */
Int32 rt_usb_host_init(void);
void rt_usbh_hub_init(void);

/* usb host core interface */
struct uinstance* rt_usbh_alloc_instance(void);
Int32 rt_usbh_attatch_instance(struct uinstance* device);
Int32 rt_usbh_detach_instance(struct uinstance* device);
Int32 rt_usbh_get_descriptor(struct uinstance* device, UInt8 type, void* buffer, 
    int nbytes);
Int32 rt_usbh_set_configure(struct uinstance* device, int config);
Int32 rt_usbh_set_address(struct uinstance* device);
Int32 rt_usbh_set_interface(struct uinstance* device, int intf);
Int32 rt_usbh_clear_feature(struct uinstance* device, int endpoint, int feature);
Int32 rt_usbh_get_interface_descriptor(ucfg_desc_t cfg_desc, int num, 
    uintf_desc_t* intf_desc);
Int32 rt_usbh_get_endpoint_descriptor(uintf_desc_t intf_desc, int num, 
    uep_desc_t* ep_desc);

/* usb class driver interface */
Int32 rt_usbh_class_driver_init(void);
Int32 rt_usbh_class_driver_register(ucd_t drv);
Int32 rt_usbh_class_driver_unregister(ucd_t drv);
Int32 rt_usbh_class_driver_enable(ucd_t drv, void* args);
Int32 rt_usbh_class_driver_disable(ucd_t drv, void* args);
ucd_t rt_usbh_class_driver_find(int class_code, int subclass_code);

/* usb class driver implement */
ucd_t rt_usbh_class_driver_hid(void);
ucd_t rt_usbh_class_driver_hub(void);
ucd_t rt_usbh_class_driver_storage(void);
ucd_t rt_usbh_class_driver_adk(void);

/* usb hid protocal implement */
uprotocal_t rt_usbh_hid_protocal_kbd(void);
uprotocal_t rt_usbh_hid_protocal_mouse(void);

/* usb adk class driver interface */
Int32 rt_usbh_adk_set_string(const char* manufacturer, const char* model,
    const char* description, const char* version, const char* uri, 
    const char* serial);

/* usb hub interface */
Int32 rt_usbh_hub_get_descriptor(struct uinstance* device, UInt8 *buffer, 
    UInt32 size);
Int32 rt_usbh_hub_get_status(struct uinstance* device, UInt8* buffer);
Int32 rt_usbh_hub_get_port_status(uhub_t uhub, UInt16 port, 
    UInt8* buffer);
Int32 rt_usbh_hub_clear_port_feature(uhub_t uhub, UInt16 port, 
    UInt16 feature);
Int32 rt_usbh_hub_set_port_feature(uhub_t uhub, UInt16 port, 
    UInt16 feature);
Int32 rt_usbh_hub_reset_port(uhub_t uhub, UInt16 port);
Int32 rt_usbh_event_signal(struct uhost_msg* msg);

/* usb host controller driver interface */
rt_inline Int32 rt_usb_hcd_alloc_pipe(uhcd_t hcd, upipe_t* pipe, 
    struct uintf* intf, uep_desc_t ep, func_callback callback)
{
    if(intf == NULL) return -RT_EIO;

    return hcd->ops->alloc_pipe(pipe, intf, ep, callback);
}

rt_inline Int32 rt_usb_hcd_free_pipe(uhcd_t hcd, upipe_t pipe)
{
    assert(pipe != NULL);
    
    return hcd->ops->free_pipe(pipe);
}

rt_inline int rt_usb_hcd_bulk_xfer(uhcd_t hcd, upipe_t pipe, void* buffer, 
    int nbytes, int timeout)
{
    if(pipe == NULL) return -1;
    if(pipe->intf == NULL) return -1;
    if(pipe->intf->device == NULL) return -1;    
    if(pipe->intf->device->status == DEV_STATUS_IDLE) 
        return -1;

    return hcd->ops->bulk_xfer(pipe, buffer, nbytes, timeout);
}

rt_inline int rt_usb_hcd_control_xfer(uhcd_t hcd, struct uinstance* device, ureq_t setup, 
    void* buffer, int nbytes, int timeout)
{
    if(device->status == DEV_STATUS_IDLE) return -1;

    return hcd->ops->ctl_xfer(device, setup, buffer, nbytes, timeout);
}

rt_inline int rt_usb_hcd_int_xfer(uhcd_t hcd, upipe_t pipe, void* buffer, 
    int nbytes, int timeout)
{    
    if(pipe == NULL) return -1;
    if(pipe->intf == NULL) return -1;
    if(pipe->intf->device == NULL) return -1;    
    if(pipe->intf->device->status == DEV_STATUS_IDLE) 
        return -1;

    return hcd->ops->int_xfer(pipe, buffer, nbytes, timeout);
}

rt_inline Int32 rt_usb_hcd_hub_control(uhcd_t hcd, UInt16 port, 
    UInt8 cmd, void *args)
{    
    return hcd->ops->hub_ctrl(port, cmd, args);
}

#ifdef __cplusplus
}
#endif

#endif

