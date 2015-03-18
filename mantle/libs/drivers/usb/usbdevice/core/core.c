/*
 * File      : core.c
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
 * 2012-12-30     heyuanjie87  change inferface handler
 * 2013-04-26     aozima       add DEVICEQUALIFIER support.
 * 2013-07-25     Yi Qiu       update for USB CV test
 */

#include <hdElastosMantle.h>
#include <rtdevice.h>

static rt_list_t device_list;

static UInt32 rt_usbd_ep_write(udevice_t device, uep_t ep, void *buffer, UInt32 size);
static UInt32 rt_usbd_ep_read_prepare(udevice_t device, uep_t ep, void *buffer, UInt32 size);
static Int32 rt_usbd_ep_assign(udevice_t device, uep_t ep);
static Int32 rt_usbd_ep_unassign(udevice_t device, uep_t ep);

/**
 * This function will handle get_device_descriptor request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _get_device_descriptor(struct udevice* device, ureq_t setup)
{
    UInt32 size;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_device_descriptor\n"));

    /* device descriptor length should less than USB_DESC_LENGTH_DEVICE*/
    size = (setup->length > USB_DESC_LENGTH_DEVICE) ?
           USB_DESC_LENGTH_DEVICE : setup->length;

    /* send device descriptor to endpoint 0 */
    rt_usbd_ep0_write(device, (UInt8*)&device->dev_desc,
                 size);

    return RT_EOK;
}

/**
 * This function will handle get_config_descriptor request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _get_config_descriptor(struct udevice* device, ureq_t setup)
{
    UInt32 size;
    ucfg_desc_t cfg_desc;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_config_descriptor\n"));

    cfg_desc = &device->curr_cfg->cfg_desc;
    size = (setup->length > cfg_desc->wTotalLength) ?
           cfg_desc->wTotalLength : setup->length;

    /* send configuration descriptor to endpoint 0 */
    rt_usbd_ep0_write(device, (UInt8*)cfg_desc, size);

    return RT_EOK;
}

/**
 * This function will handle get_string_descriptor request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful, -RT_ERROR on invalid request.
 */
static Int32 _get_string_descriptor(struct udevice* device, ureq_t setup)
{
    struct ustring_descriptor str_desc;
    UInt8 index, i;
    UInt32 len;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_string_descriptor\n"));

    str_desc.type = USB_DESC_TYPE_STRING;
    index = setup->value & 0xFF;

    if(index > USB_STRING_INTERFACE_INDEX)
    {
        printf("unknown string index\n");
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }
    if(index == 0)
    {
        str_desc.bLength = 4;
        str_desc.String[0] = 0x09;
        str_desc.String[1] = 0x04;
    }
    else
    {
        len = rt_strlen(device->str[index]);
        str_desc.bLength = len*2 + 2;

        for(i=0; i<len; i++)
        {
            str_desc.String[i*2] = device->str[index][i];
            str_desc.String[i*2 + 1] = 0;
        }
    }

    if (setup->length > str_desc.bLength)
        len = str_desc.bLength;
    else
        len = setup->length;

    /* send string descriptor to endpoint 0 */
    rt_usbd_ep0_write(device, (UInt8*)&str_desc, len);

    return RT_EOK;
}

static Int32 _get_qualifier_descriptor(struct udevice* device, ureq_t setup)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_qualifier_descriptor\n"));

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    if(device->dev_qualifier)
    {
        /* send device qualifier descriptor to endpoint 0 */
        rt_usbd_ep0_write(device, (UInt8*)device->dev_qualifier,
                     sizeof(struct usb_qualifier_descriptor));
    }
    else
    {
        rt_usbd_ep0_set_stall(device);
    }

    return RT_EOK;
}

/**
 * This function will handle get_descriptor request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _get_descriptor(struct udevice* device, ureq_t setup)
{
    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    if(setup->request_type == USB_REQ_TYPE_DIR_IN)
    {
        switch(setup->value >> 8)
        {
        case USB_DESC_TYPE_DEVICE:
            _get_device_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_CONFIGURATION:
            _get_config_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_STRING:
            _get_string_descriptor(device, setup);
            break;
        case USB_DESC_TYPE_DEVICEQUALIFIER:
            _get_qualifier_descriptor(device, setup);
            break;
        default:
            printf("unsupported descriptor request\n");
            rt_usbd_ep0_set_stall(device);
            break;
        }
    }
    else
    {
        printf("request direction error\n");
        rt_usbd_ep0_set_stall(device);
    }

    return RT_EOK;
}

/**
 * This function will handle get_interface request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _get_interface(struct udevice* device, ureq_t setup)
{
    UInt8 value;
    uintf_t intf;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_interface\n"));

    if (device->state != USB_STATE_CONFIGURED)
    {
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    /* find the specified interface and its alternate setting */
    intf = rt_usbd_find_interface(device, setup->index & 0xFF, NULL);
    value = intf->curr_setting->intf_desc->bAlternateSetting;

    /* send the interface alternate setting to endpoint 0*/
    rt_usbd_ep0_write(device, &value, 1);

    return RT_EOK;
}

/**
 * This function will handle set_interface request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _set_interface(struct udevice* device, ureq_t setup)
{
    uintf_t intf;
    uep_t ep;
    struct rt_list_node* i;
    ualtsetting_t setting;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_set_interface\n"));

    if (device->state != USB_STATE_CONFIGURED)
    {
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    /* find the specified interface */
    intf = rt_usbd_find_interface(device, setup->index & 0xFF, NULL);

    /* set alternate setting to the interface */
    rt_usbd_set_altsetting(intf, setup->value & 0xFF);
    setting = intf->curr_setting;

    /* start all endpoints of the interface alternate setting */
    for(i=setting->ep_list.next; i != &setting->ep_list; i=i->next)
    {
        ep = (uep_t)rt_list_entry(i, struct uendpoint, list);
        dcd_ep_disable(device->dcd, ep);
        dcd_ep_enable(device->dcd, ep);
    }
    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

/**
 * This function will handle get_config request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _get_config(struct udevice* device, ureq_t setup)
{
    UInt8 value;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);
    assert(device->curr_cfg != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_get_config\n"));

    if (device->state == USB_STATE_CONFIGURED)
    {
        /* get current configuration */
        value = device->curr_cfg->cfg_desc.bConfigurationValue;
    }
    else
    {
        value = 0;
    }
    /* write the current configuration to endpoint 0 */
    rt_usbd_ep0_write(device, &value, 1);

    return RT_EOK;
}

/**
 * This function will handle set_config request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _set_config(struct udevice* device, ureq_t setup)
{
    struct rt_list_node *i, *j, *k;
    uconfig_t cfg;
    uintf_t intf;
    ualtsetting_t setting;
    uep_t ep;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_set_config\n"));

    if (setup->value > device->dev_desc.bNumConfigurations)
    {
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    if (setup->value == 0)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("address state\n"));
        device->state = USB_STATE_ADDRESS;

        goto _exit;
    }

    /* set current configuration */
    rt_usbd_set_config(device, setup->value);
    dcd_set_config(device->dcd, setup->value);
    cfg = device->curr_cfg;

    for (i=cfg->func_list.next; i!=&cfg->func_list; i=i->next)
    {
        /* run all functiones and their endpoints in the configuration */
        ufunction_t func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        for(j=func->intf_list.next; j!=&func->intf_list; j=j->next)
        {
            intf = (uintf_t)rt_list_entry(j, struct uinterface, list);
            setting = intf->curr_setting;
            for(k=setting->ep_list.next; k != &setting->ep_list; k=k->next)
            {
                ep = (uep_t)rt_list_entry(k, struct uendpoint, list);

                /* first disable then enable an endpoint */
                dcd_ep_disable(device->dcd, ep);
                dcd_ep_enable(device->dcd, ep);
            }
        }
        /* after enabled endpoints, then enable function */
        FUNC_ENABLE(func);
    }

    device->state = USB_STATE_CONFIGURED;

_exit:
    /* issue status stage */
    dcd_ep0_send_status(device->dcd);

    return RT_EOK;
}

/**
 * This function will handle set_address request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _set_address(struct udevice* device, ureq_t setup)
{
    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    /* issue status stage */
    dcd_ep0_send_status(device->dcd);

    /* set address in device control driver */
    dcd_set_address(device->dcd, setup->value);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_set_address\n"));

    device->state = USB_STATE_ADDRESS;

    return RT_EOK;
}

/**
 * This function will handle standard request to
 * interface that defined in function-specifics
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _request_interface(struct udevice* device, ureq_t setup)
{
    uintf_t intf;
    ufunction_t func;
    Int32 ret;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    RT_DEBUG_LOG(RT_DEBUG_USB, ("_request_interface\n"));

    intf = rt_usbd_find_interface(device, setup->index & 0xFF, &func);
    if (intf != NULL)
    {
        ret = intf->handler(func, setup);
    }
    else
    {
        ret = -RT_ERROR;
    }

    return ret;
}

/**
 * This function will handle standard request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful.
 */
static Int32 _standard_request(struct udevice* device, ureq_t setup)
{
    udcd_t dcd;
    UInt16 value = 0;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    dcd = device->dcd;

    switch(setup->request_type & USB_REQ_TYPE_RECIPIENT_MASK)
    {
    case USB_REQ_TYPE_DEVICE:
        switch(setup->request)
        {
        case USB_REQ_GET_STATUS:
            rt_usbd_ep0_write(device, &value, 2);
            break;
        case USB_REQ_CLEAR_FEATURE:
            rt_usbd_clear_feature(device, setup->value, setup->index);
            dcd_ep0_send_status(dcd);
            break;
        case USB_REQ_SET_FEATURE:
            rt_usbd_set_feature(device, setup->value, setup->index);
            break;
        case USB_REQ_SET_ADDRESS:
            _set_address(device, setup);
            break;
        case USB_REQ_GET_DESCRIPTOR:
            _get_descriptor(device, setup);
            break;
        case USB_REQ_SET_DESCRIPTOR:
            rt_usbd_ep0_set_stall(device);
            break;
        case USB_REQ_GET_CONFIGURATION:
            _get_config(device, setup);
            break;
        case USB_REQ_SET_CONFIGURATION:
            _set_config(device, setup);
            break;
        default:
            printf("unknown device request\n");
            rt_usbd_ep0_set_stall(device);
            break;
        }
        break;
    case USB_REQ_TYPE_INTERFACE:
        switch(setup->request)
        {
        case USB_REQ_GET_INTERFACE:
            _get_interface(device, setup);
            break;
        case USB_REQ_SET_INTERFACE:
            _set_interface(device, setup);
            break;
        default:
            if (_request_interface(device, setup) != RT_EOK)
            {
                printf("unknown interface request\n");
                rt_usbd_ep0_set_stall(device);
                return - RT_ERROR;
            }
            else
                break;
        }
        break;
    case USB_REQ_TYPE_ENDPOINT:
        switch(setup->request)
        {
        case USB_REQ_GET_STATUS:
        {
            uep_t ep;

            ep = rt_usbd_find_endpoint(device, NULL, setup->index);
            value = ep->stalled;
            rt_usbd_ep0_write(device, &value, 2);
        }
        break;
        case USB_REQ_CLEAR_FEATURE:
        {
            uep_t ep;
            uio_request_t req;
            struct rt_list_node *node;

            ep = rt_usbd_find_endpoint(device, NULL, setup->index);
            if(USB_EP_HALT == setup->value && ep->stalled == RT_TRUE)
            {
                rt_usbd_clear_feature(device, setup->value, setup->index);
                dcd_ep0_send_status(dcd);
                ep->stalled = RT_FALSE;

                for (node = ep->request_list.next; node != &ep->request_list; node = node->next)
                {
                    req = (uio_request_t)rt_list_entry(node, struct uio_request, list);
                    rt_usbd_io_request(device, ep, req);
                    RT_DEBUG_LOG(RT_DEBUG_USB, ("fired a request\n"));
                }

                rt_list_init(&ep->request_list);
            }
        }
        break;
        case USB_REQ_SET_FEATURE:
        {
            uep_t ep;

            if(USB_EP_HALT == setup->value)
            {
                ep = rt_usbd_find_endpoint(device, NULL, setup->index);
                ep->stalled = RT_TRUE;
                rt_usbd_set_feature(device, setup->value, setup->index);
                dcd_ep0_send_status(dcd);
            }
        }
        break;
        case USB_REQ_SYNCH_FRAME:
            break;
        default:
            printf("unknown endpoint request\n");
            rt_usbd_ep0_set_stall(device);
            break;
        }
        break;
    case USB_REQ_TYPE_OTHER:
        printf("unknown other type request\n");
        rt_usbd_ep0_set_stall(device);
        break;
    default:
        printf("unknown type request\n");
        rt_usbd_ep0_set_stall(device);
        break;
    }

    return RT_EOK;
}

/**
 * This function will handle function request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful, -RT_ERROR on invalid request.
 */
static Int32 _function_request(udevice_t device, ureq_t setup)
{
    uintf_t intf;
    ufunction_t func;

    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    /* verify request value */
    if(setup->index > device->curr_cfg->cfg_desc.bNumInterfaces)
    {
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    switch(setup->request_type & USB_REQ_TYPE_RECIPIENT_MASK)
    {
    case USB_REQ_TYPE_INTERFACE:
        intf = rt_usbd_find_interface(device, setup->index & 0xFF, &func);
        if(intf == NULL)
        {
            printf("unkwown interface request\n");
            rt_usbd_ep0_set_stall(device);
        }
        else
        {
            intf->handler(func, setup);
        }
        break;
    case USB_REQ_TYPE_ENDPOINT:
        break;
    default:
        printf("unknown function request type\n");
        rt_usbd_ep0_set_stall(device);
        break;
    }

    return RT_EOK;
}

static Int32 _dump_setup_packet(ureq_t setup)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("[\n"));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("setup_request 0x%x\n",
                                setup->request_type));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("value 0x%x\n", setup->value));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("length 0x%x\n", setup->length));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("index 0x%x\n", setup->index));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("request 0x%x\n", setup->request));
    RT_DEBUG_LOG(RT_DEBUG_USB, ("]\n"));

    return RT_EOK;
}

/**
 * This function will handle setup request.
 *
 * @param device the usb device object.
 * @param setup the setup request.
 *
 * @return RT_EOK on successful, -RT_ERROR on invalid request.
 */
static Int32 _setup_request(udevice_t device, ureq_t setup)
{
    /* parameter check */
    assert(device != NULL);
    assert(setup != NULL);

    _dump_setup_packet(setup);

    switch((setup->request_type & USB_REQ_TYPE_MASK))
    {
    case USB_REQ_TYPE_STANDARD:
        _standard_request(device, setup);
        break;
    case USB_REQ_TYPE_CLASS:
        _function_request(device, setup);
        break;
    case USB_REQ_TYPE_VENDOR:
        printf("vendor type request\n");
        break;
    default:
        printf("unknown setup request type\n");
        rt_usbd_ep0_set_stall(device);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * This function will hanle data notify event.
 *
 * @param device the usb device object.
 * @param ep_msg the endpoint message.
 *
 * @return RT_EOK.
 */
static Int32 _data_notify(udevice_t device, struct ep_msg* ep_msg)
{
    uep_t ep;
    ufunction_t func;
    UInt32 size = 0;

    assert(device != NULL);
    assert(ep_msg != NULL);

    if (device->state != USB_STATE_CONFIGURED)
    {
        return -RT_ERROR;
    }

    ep = rt_usbd_find_endpoint(device, &func, ep_msg->ep_addr);
    if(ep == NULL)
    {
        printf("invalid endpoint\n");
        return -RT_ERROR;
    }

    if(EP_ADDRESS(ep) & USB_DIR_IN)
    {
        if(ep->request.remain_size >= EP_MAXPACKET(ep))
        {
            dcd_ep_write(device->dcd, EP_ADDRESS(ep),
                ep->request.buffer, EP_MAXPACKET(ep));
            ep->request.remain_size -= EP_MAXPACKET(ep);
            ep->request.buffer += EP_MAXPACKET(ep);
        }
        else if(ep->request.remain_size > 0)
        {
            dcd_ep_write(device->dcd, EP_ADDRESS(ep),
                ep->request.buffer, ep->request.remain_size);
            ep->request.remain_size = 0;
        }
        else
        {
            EP_HANDLER(ep, func, size);
        }
    }
    else
    {
        size = ep_msg->size;
        if(ep->request.remain_size == 0)
        {
            return RT_EOK;
        }

        if(size == 0)
        {
            size = dcd_ep_read(device->dcd, EP_ADDRESS(ep),
                ep->request.buffer);
        }

        if(size > ep->request.remain_size)
        {
            ep->request.remain_size = 0;
        }
        else
        {
            ep->request.remain_size -= size;
            ep->request.buffer += size;
        }

        if(ep->request.req_type == UIO_REQUEST_READ_MOST)
        {
            EP_HANDLER(ep, func, size);
        }
        else if(ep->request.remain_size == 0)
        {
            EP_HANDLER(ep, func, ep->request.size);
        }
    }

    return RT_EOK;
}

static Int32 _ep0_out_notify(udevice_t device, struct ep_msg* ep_msg)
{
    uep_t ep0;
    UInt32 size;

    assert(device != NULL);
    assert(ep_msg != NULL);
    assert(device->dcd != NULL);

    ep0 = &device->dcd->ep0;
    size = ep_msg->size;
    if(ep0->request.remain_size == 0)
    {
        return RT_EOK;
    }
    if(size == 0)
    {
        size = dcd_ep_read(device->dcd, EP0_OUT_ADDR, ep0->request.buffer);
        if(size == 0)
        {
            return RT_EOK;
        }
    }

    ep0->request.remain_size -= size;
    ep0->request.buffer += size;
    if(ep0->request.remain_size == 0)
    {
        /* invoke callback */
        if(ep0->rx_indicate != NULL)
        {
            ep0->rx_indicate(device, size);
        }
    }

    return RT_EOK;
}

/**
 * This function will notity sof event to all of function.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK.
 */
static Int32 _sof_notify(udevice_t device)
{
    struct rt_list_node *i;
    ufunction_t func;

    assert(device != NULL);

    /* to notity every function that sof event comes */
    for (i=device->curr_cfg->func_list.next;
            i!=&device->curr_cfg->func_list; i=i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        if(func->ops->sof_handler != NULL)
            func->ops->sof_handler(func);
    }

    return RT_EOK;
}

/**
 * This function will disable all USB functions.
 *
 * @param device the usb device object.
 *
 * @return RT_EOK.
 */
static Int32 _stop_notify(udevice_t device)
{
    struct rt_list_node *i;
    ufunction_t func;

    assert(device != NULL);

    /* to notity every function */
    for (i  = device->curr_cfg->func_list.next;
         i != &device->curr_cfg->func_list;
         i  = i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        FUNC_DISABLE(func);
    }

    return RT_EOK;
}

static UInt32 rt_usbd_ep_write(udevice_t device, uep_t ep, void *buffer, UInt32 size)
{
    UInt16 maxpacket;
    UInt32 sent_size;

    assert(device != NULL);
    assert(device->dcd != NULL);
    assert(ep != NULL);

    maxpacket = EP_MAXPACKET(ep);
    if(ep->request.remain_size >= maxpacket)
    {
        sent_size = dcd_ep_write(device->dcd, EP_ADDRESS(ep), ep->request.buffer, maxpacket);
        ep->request.remain_size -= sent_size;
        ep->request.buffer += maxpacket;
    }
    else
    {
        sent_size = dcd_ep_write(device->dcd, EP_ADDRESS(ep), ep->request.buffer,
            ep->request.remain_size);
        ep->request.remain_size -= sent_size;
    }

    return sent_size;
}

static UInt32 rt_usbd_ep_read_prepare(udevice_t device, uep_t ep, void *buffer, UInt32 size)
{
    assert(device != NULL);
    assert(device->dcd != NULL);
    assert(ep != NULL);
    assert(buffer != NULL);
    assert(ep->ep_desc != NULL);

    return dcd_ep_read_prepare(device->dcd, EP_ADDRESS(ep), buffer, size);
}

/**
 * This function will create an usb device object.
 *
 * @param ustring the usb string array to contain string descriptor.
 *
 * @return an usb device object on success, NULL on fail.
 */
udevice_t rt_usbd_device_new(void)
{
    udevice_t udevice;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_device_new\n"));

    /* allocate memory for the object */
    udevice = malloc(sizeof(struct udevice));
    if(udevice == NULL)
    {
        printf("alloc memery failed\n");
        return NULL;
    }
    memset(udevice, 0, sizeof(struct udevice));

    /* to initialize configuration list */
    rt_list_init(&udevice->cfg_list);

    /* insert the device object to device list */
    rt_list_insert_before(&device_list, &udevice->list);

    return udevice;
}

/**
 * This function will set usb device string description.
 *
 * @param device the usb device object.
 * @param ustring pointer to string pointer array.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_device_set_string(udevice_t device, const char** ustring)
{
    /* parameter check */
    assert(device != NULL);
    assert(ustring != NULL);

    /* set string descriptor array to the device object */
    device->str = ustring;

    return RT_EOK;
}

Int32 rt_usbd_device_set_qualifier(udevice_t device, struct usb_qualifier_descriptor* qualifier)
{
    /* parameter check */
    assert(device != NULL);
    assert(qualifier != NULL);

    device->dev_qualifier = qualifier;

    return RT_EOK;
}

/**
 * This function will set an usb controller driver to a device.
 *
 * @param device the usb device object.
 * @param dcd the usb device controller driver.
 *
 * @return RT_EOK on successful.
 */
Int32 rt_usbd_device_set_controller(udevice_t device, udcd_t dcd)
{
    /* parameter check */
    assert(device != NULL);
    assert(dcd != NULL);

    /* set usb device controller driver to the device */
    device->dcd = dcd;

    return RT_EOK;
}

/**
 * This function will set an usb device descriptor to a device.
 *
 * @param device the usb device object.
 * @param dev_desc the usb device descriptor.
 *
 * @return RT_EOK on successful.
 */
Int32 rt_usbd_device_set_descriptor(udevice_t device, udev_desc_t dev_desc)
{
    /* parameter check */
    assert(device != NULL);
    assert(dev_desc != NULL);

    /* copy the usb device descriptor to the device */
    memcpy((void *)&device->dev_desc, (void *)dev_desc, USB_DESC_LENGTH_DEVICE);

    return RT_EOK;
}

/**
 * This function will create an usb configuration object.
 *
 * @param none.
 *
 * @return an usb configuration object.
 */
uconfig_t rt_usbd_config_new(void)
{
    uconfig_t cfg;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_config_new\n"));

    /* allocate memory for the object */
    cfg = malloc(sizeof(struct uconfig));
    if(cfg == NULL)
    {
        printf("alloc memery failed\n");
        return NULL;
    }
    memset(cfg, 0, sizeof(struct uconfig));

    /* set default value */
    cfg->cfg_desc.bLength = USB_DESC_LENGTH_CONFIG;
    cfg->cfg_desc.type = USB_DESC_TYPE_CONFIGURATION;
    cfg->cfg_desc.wTotalLength = USB_DESC_LENGTH_CONFIG;
    cfg->cfg_desc.bmAttributes = 0xC0;
    cfg->cfg_desc.MaxPower = 0x32;

    /* to initialize function object list */
    rt_list_init(&cfg->func_list);

    return cfg;
}

/**
 * This function will create an usb interface object.
 *
 * @param device the usb device object.
 * @handler the callback handler of object
 *
 * @return an usb interface object on success, NULL on fail.
 */
uintf_t rt_usbd_interface_new(udevice_t device, uintf_handler_t handler)
{
    uintf_t intf;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_interface_new\n"));

    /* parameter check */
    assert(device != NULL);

    /* allocate memory for the object */
    intf = (uintf_t)malloc(sizeof(struct uinterface));
    if(intf == NULL)
    {
        printf("alloc memery failed\n");
        return NULL;
    }
    intf->intf_num = device->nr_intf;
    device->nr_intf++;
    intf->handler = handler;
    intf->curr_setting = NULL;

    /* to initialize the alternate setting object list */
    rt_list_init(&intf->setting_list);

    return intf;
}

/**
 * This function will create an usb alternate setting object.
 *
 * @param intf_desc the interface descriptor.
 * @desc_size the size of the interface descriptor.
 *
 * @return an usb alternate setting object on success, NULL on fail.
 */
ualtsetting_t rt_usbd_altsetting_new(UInt32 desc_size)
{
    ualtsetting_t setting;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_altsetting_new\n"));

    /* parameter check */
    assert(desc_size > 0);

    /* allocate memory for the object */
    setting = (ualtsetting_t)malloc(sizeof(struct ualtsetting));
    if(setting == NULL)
    {
        printf("alloc memery failed\n");
        return NULL;
    }
    /* allocate memory for the desc */
    setting->desc = malloc(desc_size);
    if (setting->desc == NULL)
    {
        printf("alloc desc memery failed\n");
        rt_free(setting);
        return NULL;
    }

    setting->desc_size = desc_size;
    setting->intf_desc = NULL;

    /* to initialize endpoint list */
    rt_list_init(&setting->ep_list);

    return setting;
}

/**
 * This function will config an desc in alternate setting object.
 *
 * @param setting the altsetting to be config.
 * @param desc use it to init desc in setting.
 * @param intf_pos the offset of interface descriptor in desc.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_altsetting_config_descriptor(ualtsetting_t setting, const void* desc, Int32 intf_pos)
{
    assert(setting != NULL);
    assert(setting->desc !=NULL);

    memcpy(setting->desc, desc, setting->desc_size);
    setting->intf_desc = (uintf_desc_t)((char*)setting->desc + intf_pos);

    return RT_EOK;
}

/**
 * This function will create an usb function object.
 *
 * @param device the usb device object.
 * @param dev_desc the device descriptor.
 * @param ops the operation set.
 *
 * @return an usb function object on success, NULL on fail.
 */
ufunction_t rt_usbd_function_new(udevice_t device, udev_desc_t dev_desc,
                              ufunction_ops_t ops)
{
    ufunction_t func;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_function_new\n"));

    /* parameter check */
    assert(device != NULL);
    assert(dev_desc != NULL);

    /* allocate memory for the object */
    func = (ufunction_t)malloc(sizeof(struct ufunction));
    if(func == NULL)
    {
        printf("alloc memery failed\n");
        return NULL;
    }
    func->dev_desc = dev_desc;
    func->ops = ops;
    func->device = device;
    func->enabled = RT_FALSE;

    /* to initialize interface list */
    rt_list_init(&func->intf_list);

    return func;
}

/**
 * This function will create an usb endpoint object.
 *
 * @param ep_desc the endpoint descriptor.
 * @handler the callback handler of object
 *
 * @return an usb endpoint object on success, NULL on fail.
 */
uep_t rt_usbd_endpoint_new(uep_desc_t ep_desc, udep_handler_t handler)
{
    uep_t ep;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_endpoint_new\n"));

    /* parameter check */
    assert(ep_desc != NULL);

    /* allocate memory for the object */
    ep = (uep_t)malloc(sizeof(struct uendpoint));
    if(ep == NULL)
    {
        printf("alloc memery failed\n");
        return NULL;
    }
    ep->ep_desc = ep_desc;
    ep->handler = handler;
    ep->buffer  = NULL;
    ep->stalled = RT_FALSE;
    rt_list_init(&ep->request_list);

    return ep;
}

/**
 * This function will find an usb device object.
 *
 * @dcd usd device controller driver.
 *
 * @return an usb device object on found or NULL on not found.
 */
udevice_t rt_usbd_find_device(udcd_t dcd)
{
    struct rt_list_node* node;
    udevice_t device;

    /* parameter check */
    assert(dcd != NULL);

    /* search a device in the the device list */
    for (node = device_list.next; node != &device_list; node = node->next)
    {
        device = (udevice_t)rt_list_entry(node, struct udevice, list);
        if(device->dcd == dcd) return device;
    }

    printf("can't find device\n");
    return NULL;
}

/**
 * This function will find an usb configuration object.
 *
 * @param device the usb device object.
 * @param value the configuration number.
 *
 * @return an usb configuration object on found or NULL on not found.
 */
uconfig_t rt_usbd_find_config(udevice_t device, UInt8 value)
{
    struct rt_list_node* node;
    uconfig_t cfg = NULL;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_find_config\n"));

    /* parameter check */
    assert(device != NULL);
    assert(value <= device->dev_desc.bNumConfigurations);

    /* search a configration in the the device */
    for (node = device->cfg_list.next; node != &device->cfg_list; node = node->next)
    {
        cfg = (uconfig_t)rt_list_entry(node, struct udevice, list);
        if(cfg->cfg_desc.bConfigurationValue == value)
        {
            return cfg;
        }
    }

    printf("can't find configuration %d\n", value);
    return NULL;
}

/**
 * This function will find an usb interface object.
 *
 * @param device the usb device object.
 * @param value the interface number.
 *
 * @return an usb configuration object on found or NULL on not found.
 */
uintf_t rt_usbd_find_interface(udevice_t device, UInt8 value, ufunction_t *pfunc)
{
    struct rt_list_node *i, *j;
    ufunction_t func;
    uintf_t intf;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_find_interface\n"));

    /* parameter check */
    assert(device != NULL);
    assert(value < device->nr_intf);

    /* search an interface in the current configuration */
    for (i=device->curr_cfg->func_list.next;
            i!=&device->curr_cfg->func_list; i=i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        for(j=func->intf_list.next; j!=&func->intf_list; j=j->next)
        {
            intf = (uintf_t)rt_list_entry(j, struct uinterface, list);
            if(intf->intf_num == value)
            {
                if (pfunc != NULL)
                    *pfunc = func;
                return intf;
            }
        }
    }

    printf("can't find interface %d\n", value);
    return NULL;
}

/**
 * This function will find an usb interface alternate setting object.
 *
 * @param device the usb device object.
 * @param value the alternate setting number.
 *
 * @return an usb interface alternate setting object on found or NULL on not found.
 */
ualtsetting_t rt_usbd_find_altsetting(uintf_t intf, UInt8 value)
{
    struct rt_list_node *i;
    ualtsetting_t setting;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_find_altsetting\n"));

    /* parameter check */
    assert(intf != NULL);

    if(intf->curr_setting != NULL)
    {
        /* if the value equal to the current alternate setting, then do not search */
        if(intf->curr_setting->intf_desc->bAlternateSetting == value)
            return intf->curr_setting;
    }

    /* search a setting in the alternate setting list */
    for(i=intf->setting_list.next; i!=&intf->setting_list; i=i->next)
    {
        setting =(ualtsetting_t)rt_list_entry(i, struct ualtsetting, list);
        if(setting->intf_desc->bAlternateSetting == value)
            return setting;
    }

    printf("can't find alternate setting %d\n", value);
    return NULL;
}

/**
 * This function will find an usb endpoint object.
 *
 * @param device the usb device object.
 * @param ep_addr endpoint address.
 *
 * @return an usb endpoint object on found or NULL on not found.
 */
uep_t rt_usbd_find_endpoint(udevice_t device, ufunction_t* pfunc, UInt8 ep_addr)
{
    uep_t ep;
    struct rt_list_node *i, *j, *k;
    ufunction_t func;
    uintf_t intf;

    /* parameter check */
    assert(device != NULL);

    /* search a endpoint in the current configuration */
    for (i=device->curr_cfg->func_list.next;
            i!=&device->curr_cfg->func_list; i=i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);
        for(j=func->intf_list.next; j!=&func->intf_list; j=j->next)
        {
            intf = (uintf_t)rt_list_entry(j, struct uinterface, list);
            for(k=intf->curr_setting->ep_list.next;
                    k!=&intf->curr_setting->ep_list; k=k->next)
            {
                ep = (uep_t)rt_list_entry(k, struct uendpoint, list);
                if(EP_ADDRESS(ep) == ep_addr)
                {
                    if (pfunc != NULL)
                        *pfunc = func;
                    return ep;
                }
            }
        }
    }

    printf("can't find endpoint 0x%x\n", ep_addr);
    return NULL;
}

/**
 * This function will add a configuration to an usb device.
 *
 * @param device the usb device object.
 * @param cfg the configuration object.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_device_add_config(udevice_t device, uconfig_t cfg)
{
    struct rt_list_node *i, *j, *k;
    ufunction_t func;
    uintf_t intf;
    uep_t ep;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_device_add_config\n"));

    /* parameter check */
    assert(device != NULL);
    assert(cfg != NULL);

    /* set configuration number to the configuration descriptor */
    cfg->cfg_desc.bConfigurationValue = device->dev_desc.bNumConfigurations + 1;
    device->dev_desc.bNumConfigurations++;

    for (i=cfg->func_list.next; i!=&cfg->func_list; i=i->next)
    {
        func = (ufunction_t)rt_list_entry(i, struct ufunction, list);

        for(j=func->intf_list.next; j!=&func->intf_list; j=j->next)
        {
            intf = (uintf_t)rt_list_entry(j, struct uinterface, list);
            cfg->cfg_desc.bNumInterfaces++;

            /* allocate address for every endpoint in the interface alternate setting */
            for(k=intf->curr_setting->ep_list.next;
                    k!=&intf->curr_setting->ep_list; k=k->next)
            {
                ep = (uep_t)rt_list_entry(k, struct uendpoint, list);
                if(rt_usbd_ep_assign(device, ep) != RT_EOK)
                {
                    printf("endpoint assign error\n");
                }
            }

            /* construct complete configuration descriptor */
            memcpy((void*)&cfg->cfg_desc.data[cfg->cfg_desc.wTotalLength - USB_DESC_LENGTH_CONFIG],
                        (void*)intf->curr_setting->desc,
                        intf->curr_setting->desc_size);
            cfg->cfg_desc.wTotalLength += intf->curr_setting->desc_size;
        }
    }

    /* insert the configuration to the list */
    rt_list_insert_before(&device->cfg_list, &cfg->list);

    return RT_EOK;
}

/**
 * This function will add a function to a configuration.
 *
 * @param cfg the configuration object.
 * @param func the function object.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_config_add_function(uconfig_t cfg, ufunction_t func)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_config_add_function\n"));

    /* parameter check */
    assert(cfg != NULL);
    assert(func != NULL);

    /* insert the function to the list */
    rt_list_insert_before(&cfg->func_list, &func->list);

    return RT_EOK;
}

/**
 * This function will add an interface to a function.
 *
 * @param func the function object.
 * @param intf the interface object.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_function_add_interface(ufunction_t func, uintf_t intf)
{

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_function_add_interface\n"));

    /* parameter check */
    assert(func != NULL);
    assert(intf != NULL);

    /* insert the interface to the list */
    rt_list_insert_before(&func->intf_list, &intf->list);

    return RT_EOK;
}

/**
 * This function will add an alternate setting to an interface.
 *
 * @param intf the interface object.
 * @param setting the alternate setting object.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_interface_add_altsetting(uintf_t intf, ualtsetting_t setting)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_interface_add_altsetting\n"));

    /* parameter check */
    assert(intf != NULL);
    assert(setting != NULL);

    setting->intf_desc->bInterfaceNumber = intf->intf_num;

    /* insert the alternate setting to the list */
    rt_list_insert_before(&intf->setting_list, &setting->list);

    return RT_EOK;
}

/**
 * This function will add an endpoint to an alternate setting.
 *
 * @param setting the alternate setting object.
 * @param ep the endpoint object.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_altsetting_add_endpoint(ualtsetting_t setting, uep_t ep)
{
    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_altsetting_add_endpoint\n"));

    /* parameter check */
    assert(setting != NULL);
    assert(ep != NULL);

    /* insert the endpoint to the list */
    rt_list_insert_before(&setting->ep_list, &ep->list);

    return RT_EOK;
}

/**
 * This function will set an alternate setting for an interface.
 *
 * @param intf_desc the interface descriptor.
 * @param value the alternate setting number.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_set_altsetting(uintf_t intf, UInt8 value)
{
    ualtsetting_t setting;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_set_altsetting\n"));

    /* parameter check */
    assert(intf != NULL);

    /* find an alternate setting */
    setting = rt_usbd_find_altsetting(intf, value);

    /* set as current alternate setting */
    intf->curr_setting = setting;

    return RT_EOK;
}

/**
 * This function will set a configuration for an usb device.
 *
 * @param device the usb device object.
 * @param value the configuration number.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_set_config(udevice_t device, UInt8 value)
{
    uconfig_t cfg;

    RT_DEBUG_LOG(RT_DEBUG_USB, ("rt_usbd_set_config\n"));

    /* parameter check */
    assert(device != NULL);
    assert(value <= device->dev_desc.bNumConfigurations);

    /* find a configuration */
    cfg = rt_usbd_find_config(device, value);

    /* set as current configuration */
    device->curr_cfg = cfg;

    return RT_TRUE;
}

/**
 * This function will request an IO transaction.
 *
 * @param device the usb device object.
 * @param ep the endpoint object.
 * @param req IO request.
 *
 * @return RT_EOK.
 */
UInt32 rt_usbd_io_request(udevice_t device, uep_t ep, uio_request_t req)
{
    UInt32 size = 0;

    assert(device != NULL);
    assert(req != NULL);

    if(ep->stalled == RT_FALSE)
    {
        switch(req->req_type)
        {
        case UIO_REQUEST_READ_MOST:
        case UIO_REQUEST_READ_FULL:
            ep->request.remain_size = ep->request.size;
            size = rt_usbd_ep_read_prepare(device, ep, req->buffer, req->size);
            break;
        case UIO_REQUEST_WRITE:
            ep->request.remain_size = ep->request.size;
            size = rt_usbd_ep_write(device, ep, req->buffer, req->size);
            break;
        default:
            printf("unknown request type\n");
            break;
        }
    }
    else
    {
        rt_list_insert_before(&ep->request_list, &req->list);
        RT_DEBUG_LOG(RT_DEBUG_USB, ("suspend a request\n"));
    }

    return size;
}

/**
 * This function will set feature for an usb device.
 *
 * @param device the usb device object.
 * @param value the configuration number.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_set_feature(udevice_t device, UInt16 value, UInt16 index)
{
    assert(device != NULL);

    if (value == USB_FEATURE_DEV_REMOTE_WAKEUP)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("set feature remote wakeup\n"));
    }
    else if (value == USB_FEATURE_ENDPOINT_HALT)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("set feature stall\n"));
        dcd_ep_set_stall(device->dcd, (UInt32)(index & 0xFF));
    }

    return RT_EOK;
}

/**
 * This function will clear feature for an usb device.
 *
 * @param device the usb device object.
 * @param value the configuration number.
 *
 * @return RT_EOK.
 */
Int32 rt_usbd_clear_feature(udevice_t device, UInt16 value, UInt16 index)
{
    assert(device != NULL);

    if (value == USB_FEATURE_DEV_REMOTE_WAKEUP)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("clear feature remote wakeup\n"));
    }
    else if (value == USB_FEATURE_ENDPOINT_HALT)
    {
        RT_DEBUG_LOG(RT_DEBUG_USB, ("clear feature stall\n"));
        dcd_ep_clear_stall(device->dcd, (UInt32)(index & 0xFF));
    }

    return RT_EOK;
}

Int32 rt_usbd_ep0_set_stall(udevice_t device)
{
    assert(device != NULL);

    return dcd_ep_set_stall(device->dcd, 0);
}

Int32 rt_usbd_ep0_clear_stall(udevice_t device)
{
    assert(device != NULL);

    return dcd_ep_clear_stall(device->dcd, 0);
}

Int32 rt_usbd_ep_set_stall(udevice_t device, uep_t ep)
{
    Int32 ret;

    assert(device != NULL);
    assert(ep != NULL);
    assert(ep->ep_desc != NULL);

    ret = dcd_ep_set_stall(device->dcd, EP_ADDRESS(ep));
    if(ret == RT_EOK)
    {
        ep->stalled = RT_TRUE;
    }

    return ret;
}

Int32 rt_usbd_ep_clear_stall(udevice_t device, uep_t ep)
{
    Int32 ret;

    assert(device != NULL);
    assert(ep != NULL);
    assert(ep->ep_desc != NULL);

    ret = dcd_ep_clear_stall(device->dcd, EP_ADDRESS(ep));
    if(ret == RT_EOK)
    {
        ep->stalled = RT_FALSE;
    }

    return ret;
}

static Int32 rt_usbd_ep_assign(udevice_t device, uep_t ep)
{
    int i = 0;

    assert(device != NULL);
    assert(device->dcd != NULL);
    assert(device->dcd->ep_pool != NULL);
    assert(ep != NULL);
    assert(ep->ep_desc != NULL);

    while(device->dcd->ep_pool[i].addr != 0xFF)
    {
        if(device->dcd->ep_pool[i].status == ID_UNASSIGNED &&
            ep->ep_desc->bmAttributes == device->dcd->ep_pool[i].type &&
            ep->ep_desc->bEndpointAddress == device->dcd->ep_pool[i].dir)
        {
            EP_ADDRESS(ep) |= device->dcd->ep_pool[i].addr;
            ep->id = &device->dcd->ep_pool[i];
            device->dcd->ep_pool[i].status = ID_ASSIGNED;

            RT_DEBUG_LOG(RT_DEBUG_USB, ("assigned %d\n", device->dcd->ep_pool[i].addr));
            return RT_EOK;
        }

        i++;
    }

    return -RT_ERROR;
}

static Int32 rt_usbd_ep_unassign(udevice_t device, uep_t ep)
{
    assert(device != NULL);
    assert(device->dcd != NULL);
    assert(device->dcd->ep_pool != NULL);
    assert(ep != NULL);
    assert(ep->ep_desc != NULL);

    ep->id->status = ID_UNASSIGNED;

    return RT_EOK;
}

Int32 rt_usbd_ep0_setup_handler(udcd_t dcd, struct urequest* setup)
{
    struct udev_msg msg;
    UInt32 size;

    assert(dcd != NULL);

    if(setup == NULL)
    {
        size = dcd_ep_read(dcd, EP0_OUT_ADDR, (void*)&msg.content.setup);
        if(size != sizeof(struct urequest))
        {
            printf("read setup packet error\n");
            return -RT_ERROR;
        }
    }
    else
    {
        memcpy((void*)&msg.content.setup, (void*)setup, sizeof(struct urequest));
    }

    msg.type = USB_MSG_SETUP_NOTIFY;
    msg.dcd = dcd;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_ep0_in_handler(udcd_t dcd)
{
    assert(dcd != NULL);

    if(dcd->ep0.request.remain_size >= dcd->ep0.id->maxpacket)
    {
        dcd_ep_write(dcd, EP0_IN_ADDR, dcd->ep0.request.buffer, dcd->ep0.id->maxpacket);
        dcd->ep0.request.remain_size -= dcd->ep0.id->maxpacket;
    }
    else if(dcd->ep0.request.remain_size > 0)
    {
        dcd_ep_write(dcd, EP0_IN_ADDR, dcd->ep0.request.buffer, dcd->ep0.request.remain_size);
        dcd->ep0.request.remain_size = 0;
    }
    else
    {
        dcd_ep_write(dcd, EP0_IN_ADDR, NULL, 0);
    }

    return RT_EOK;
}

Int32 rt_usbd_ep0_out_handler(udcd_t dcd, UInt32 size)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_EP0_OUT;
    msg.dcd = dcd;
    msg.content.ep_msg.size = size;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_ep_in_handler(udcd_t dcd, UInt8 address)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_DATA_NOTIFY;
    msg.dcd = dcd;
    msg.content.ep_msg.ep_addr = address;
    msg.content.ep_msg.size = 0;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_ep_out_handler(udcd_t dcd, UInt8 address, UInt32 size)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_DATA_NOTIFY;
    msg.dcd = dcd;
    msg.content.ep_msg.ep_addr = address;
    msg.content.ep_msg.size = size;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_reset_handler(udcd_t dcd)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_RESET;
    msg.dcd = dcd;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_connect_handler(udcd_t dcd)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_PLUG_IN;
    msg.dcd = dcd;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_disconnect_handler(udcd_t dcd)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_PLUG_OUT;
    msg.dcd = dcd;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

Int32 rt_usbd_sof_handler(udcd_t dcd)
{
    struct udev_msg msg;

    assert(dcd != NULL);

    msg.type = USB_MSG_SOF;
    msg.dcd = dcd;
    rt_usbd_event_signal(&msg);

    return RT_EOK;
}

UInt32 rt_usbd_ep0_write(udevice_t device, void *buffer, UInt32 size)
{
    uep_t ep0;
    UInt32 sent_size = 0;

    assert(device != NULL);
    assert(device->dcd != NULL);
    assert(buffer != NULL);
    assert(size > 0);

    ep0 = &device->dcd->ep0;
    ep0->request.size = size;
    ep0->request.buffer = buffer;
    ep0->request.remain_size = size;
    if(ep0->request.remain_size >= ep0->id->maxpacket)
    {
        sent_size = dcd_ep_write(device->dcd, EP0_IN_ADDR, ep0->request.buffer, ep0->id->maxpacket);
        ep0->request.remain_size -= sent_size;
        ep0->request.buffer += ep0->id->maxpacket;
    }
    else
    {
        sent_size = dcd_ep_write(device->dcd, EP0_IN_ADDR, ep0->request.buffer, ep0->request.remain_size);
        ep0->request.remain_size -= sent_size;
    }

    return sent_size;
}

UInt32 rt_usbd_ep0_read(udevice_t device, void *buffer, UInt32 size,
    Int32 (*rx_ind)(udevice_t device, UInt32 size))
{
    uep_t ep0;

    assert(device != NULL);
    assert(device->dcd != NULL);
    assert(buffer != NULL);

    ep0 = &device->dcd->ep0;
    ep0->request.size = size;
    ep0->request.buffer = buffer;
    ep0->request.remain_size = size;
    ep0->rx_indicate = rx_ind;
    dcd_ep_read_prepare(device->dcd, EP0_OUT_ADDR, buffer, size);

    return size;
}

static struct rt_messagequeue usb_mq;

/**
 * This function is the main entry of usb device thread, it is in charge of
 * processing all messages received from the usb message buffer.
 *
 * @param parameter the parameter of the usb device thread.
 *
 * @return none.
 */
static void rt_usbd_thread_entry(void* parameter)
{
    while(1)
    {
        struct udev_msg msg;
        udevice_t device;

        /* receive message */
        if(rt_mq_recv(&usb_mq, &msg, sizeof(struct udev_msg),
                    RT_WAITING_FOREVER) != RT_EOK )
            continue;

        device = rt_usbd_find_device(msg.dcd);
        if(device == NULL)
        {
            printf("invalid usb device\n");
            continue;
        }

        RT_DEBUG_LOG(RT_DEBUG_USB, ("message type %d\n", msg.type));

        switch (msg.type)
        {
        case USB_MSG_SOF:
            _sof_notify(device);
            break;
        case USB_MSG_DATA_NOTIFY:
            /* some buggy drivers will have USB_MSG_DATA_NOTIFY before the core
             * got configured. */
            _data_notify(device, &msg.content.ep_msg);
            break;
        case USB_MSG_SETUP_NOTIFY:
            _setup_request(device, &msg.content.setup);
            break;
        case USB_MSG_EP0_OUT:
            _ep0_out_notify(device, &msg.content.ep_msg);
            break;
        case USB_MSG_RESET:
            RT_DEBUG_LOG(RT_DEBUG_USB, ("reset %d\n", device->state));
            if (device->state == USB_STATE_ADDRESS)
                _stop_notify(device);
            break;
        case USB_MSG_PLUG_IN:
            device->state = USB_STATE_ATTACHED;
            break;
        case USB_MSG_PLUG_OUT:
            device->state = USB_STATE_NOTATTACHED;
            _stop_notify(device);
            break;
        default:
            printf("unknown msg type %d\n", msg.type);
            break;
        }
    }
}

/**
 * This function will post an message to usb message queue,
 *
 * @param msg the message to be posted
 * @param size the size of the message .
 *
 * @return the error code, RT_EOK on successfully.
 */
Int32 rt_usbd_event_signal(struct udev_msg* msg)
{
    assert(msg != NULL);

    /* send message to usb message queue */
    return rt_mq_send(&usb_mq, (void*)msg, sizeof(struct udev_msg));
}


ALIGN(RT_ALIGN_SIZE)
static UInt8 usb_thread_stack[RT_USBD_THREAD_STACK_SZ];
static struct rt_thread usb_thread;
#define USBD_MQ_MSG_SZ  32
#define USBD_MQ_MAX_MSG 16
/* internal of the message queue: every message is associated with a pointer,
 * so in order to recveive USBD_MQ_MAX_MSG messages, we have to allocate more
 * than USBD_MQ_MSG_SZ*USBD_MQ_MAX_MSG memery. */
static UInt8 usb_mq_pool[(USBD_MQ_MSG_SZ+sizeof(void*))*USBD_MQ_MAX_MSG];

/**
 * This function will initialize usb device thread.
 *
 * @return none.
 *
 */
Int32 rt_usbd_core_init(void)
{
    rt_list_init(&device_list);

    /* create an usb message queue */
    rt_mq_init(&usb_mq, "usbd", usb_mq_pool, USBD_MQ_MSG_SZ,
            sizeof(usb_mq_pool), RT_IPC_FLAG_FIFO);

    /* init usb device thread */
    rt_thread_init(&usb_thread, "usbd", rt_usbd_thread_entry, NULL,
            usb_thread_stack, RT_USBD_THREAD_STACK_SZ, RT_USBD_THREAD_PRIO, 20);
    /* rt_thread_init should always be OK, so start the thread without further
     * checking. */
    return rt_thread_startup(&usb_thread);
}

