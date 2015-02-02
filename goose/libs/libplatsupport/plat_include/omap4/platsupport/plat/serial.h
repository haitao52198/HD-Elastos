/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_CHARDEV_H__
#define __PLATSUPPORT_PLAT_CHARDEV_H__

#define UART1_PADDR  0x4806a000
#define UART2_PADDR  0x4806c000
#define UART3_PADDR  0x48020000
#define UART4_PADDR  0x4806e000

#define UART1_IRQ    72
#define UART2_IRQ    73
#define UART3_IRQ    74
#define UART4_IRQ    70

/* official device names */
enum chardev_id {
    OMAP4_UART1,
    OMAP4_UART2,
    OMAP4_UART3,
    OMAP4_UART4,
    /* Aliases */
    PS_SERIAL0 = OMAP4_UART1,
    PS_SERIAL1 = OMAP4_UART2,
    PS_SERIAL2 = OMAP4_UART3,
    PS_SERIAL3 = OMAP4_UART4,
    /* defaults */
    PS_SERIAL_DEFAULT = OMAP4_UART3
};

#endif /* __PLATSUPPORT_PLAT_CHARDEV_H__ */
