/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef __PLATSUPPORT_PLAT_CHARDEV_H__
#define __PLATSUPPORT_PLAT_CHARDEV_H__

#define UART0_PADDR  0x01c28000
#define UART1_PADDR  0x01c28400
#define UART2_PADDR  0x01c28800
#define UART3_PADDR  0x01c28c00
#define UART4_PADDR  0x01c29000
#define UART5_PADDR  0x01c29400
#define UART6_PADDR  0x01c29800
#define UART7_PADDR  0x01c29c00

#define UART0_IRQ    33
#define UART1_IRQ    34
#define UART2_IRQ    35
#define UART3_IRQ    36
#define UART4_IRQ    49
#define UART4_IRQ    50
#define UART4_IRQ    51
#define UART4_IRQ    52

/* official device names */
enum chardev_id {
    A20_UART0,
    A20_UART1,
    A20_UART2,
    A20_UART3,
    A20_UART4,
    A20_UART5,
    A20_UART6,
    A20_UART7,

    /* Aliases */
    PS_SERIAL0 = A20_UART0,
    PS_SERIAL1 = A20_UART1,
    PS_SERIAL2 = A20_UART2,
    PS_SERIAL3 = A20_UART3,
    PS_SERIAL4 = A20_UART4,
    PS_SERIAL5 = A20_UART5,
    PS_SERIAL6 = A20_UART6,
    PS_SERIAL7 = A20_UART7,
    /* defaults */
    PS_SERIAL_DEFAULT = A20_UART0
};

#endif /* __PLATSUPPORT_PLAT_CHARDEV_H__ */
