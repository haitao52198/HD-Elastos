/*
 * File      : sd.h
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
 * 2011-07-25     weety         first version
 */

#ifndef __SD_H__
#define __SD_H__

#include <hdElastosMantle.h>
#include <drivers/mmcsd_host.h>

#ifdef __cplusplus
extern "C" {
#endif

Int32 mmcsd_send_if_cond(struct rt_mmcsd_host *host, UInt32 ocr);
Int32 mmcsd_send_app_op_cond(struct rt_mmcsd_host *host, UInt32 ocr, UInt32 *rocr);
Int32 init_sd(struct rt_mmcsd_host *host, UInt32 ocr);

#ifdef __cplusplus
}
#endif

#endif
