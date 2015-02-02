/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

/*
 * Contains definitions for all character devices on this
 * platform
 */

#include "../../chardev.h"
#include "../../common.h"

#include <utils/arith.h>
#include "serial.h"

static const int uart0_irqs[] = {UART0_IRQ, -1};
static const int uart1_irqs[] = {UART1_IRQ, -1};
static const int uart2_irqs[] = {UART2_IRQ, -1};
static const int uart3_irqs[] = {UART3_IRQ, -1};
static const int uart4_irqs[] = {UART4_IRQ, -1};
static const int uart5_irqs[] = {UART5_IRQ, -1};
static const int uart6_irqs[] = {UART6_IRQ, -1};
static const int uart7_irqs[] = {UART7_IRQ, -1};


#define UART_DEFN(devid) {                     \
        .id      = A20_UART##devid,          \
        .paddr   = UART##devid##_PADDR,        \
        .size    = (1<<12),                    \
        .irqs    = uart##devid##_irqs,         \
        .init_fn = &uart_init                  \
    }


static const struct dev_defn dev_defn[] = {
    UART_DEFN(0),
    UART_DEFN(1),
    UART_DEFN(2),
    UART_DEFN(3),
    UART_DEFN(4),
    UART_DEFN(5),
    UART_DEFN(6),
    UART_DEFN(7)
};


/* It would be nice to reuse this, but it requires knowledge of the variable *
 * sized 'dev_defn'                                                          */
struct ps_chardevice*
ps_cdev_init(enum chardev_id id, const ps_io_ops_t* o,
             struct ps_chardevice* d)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(dev_defn); i++) {
        if (dev_defn[i].id == id) {
            return (dev_defn[i].init_fn(dev_defn + i, o, d)) ? NULL : d;
        }
    }
    return NULL;
}

