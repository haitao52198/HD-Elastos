/*
 * File      : mstorage.h
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
 * 2012-12-12     heyuanjie87  add MASS endpoints collection
 */
 
#ifndef  __MSTORAGE_H__
#define  __MSTORAGE_H__

#include <hdElastos.h>

#pragma pack(1)

struct umass_descriptor
{
    struct uinterface_descriptor intf_desc;
    struct uendpoint_descriptor ep_out_desc;
    struct uendpoint_descriptor ep_in_desc;    
};
typedef struct umass_descriptor* umass_desc_t;

struct capacity_data
{
    UInt8 LastLogicalBlockAddress[4];
    UInt8 BlockLengthInBytes[4];
};

struct request_sense_data
{
    UInt8 ErrorCode:7;
    UInt8 Valid:1;
    UInt8 Reserved1;
    UInt8 SenseKey:4;
    UInt8 Reserved2:4;
    UInt8 Information[4];
    UInt8 AdditionalSenseLength;
    UInt8 Reserved3[4];
    UInt8 AdditionalSenseCode;
    UInt8 AdditionalSenseCodeQualifier;
    UInt8 Reserved4[4];
}request_sense_data_t;

#pragma pack()

#endif
