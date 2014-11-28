/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

/*
 definitions for memory addresses of omap4 platform
 listed in the order of ascending memory address, similar to the datasheet
 the size must all be powers of 2 and page aligned (4k or 1M).
*/

#ifndef __PLAT_MACHINE_DEVICES_H
#define __PLAT_MACHINE_DEVICES_H

// These devices are used by the seL4 kernel.
// PPTR: Page-PTR
#define UART3_PPTR                     0xfff01000
#define INTC_PPTR                      0xfff02000
#define GPTIMER11_PPTR                 0xfff03000

//check with OMAP4460_ES1.x_PUBLIC_TRM_vI.pdf
#define PWR_AND_RST_MANAGER_PADDR      0x48306000
#define WDT2_PADDR                     0x48314000
#define TIMER32K_PADDR                 0x48320000
#define L4_WAKEUP_CONFIG_PADDR         0x48328000

#define UART1_PADDR                    0x4806a000
#define UART2_PADDR                    0x4806c000
#define UART3_PADDR                    0x48020000
#define UART4_PADDR                    0x4806e000
#define GPIO1_PADDR                    0x4a310000
#define GPIO2_PADDR                    0x48055000
#define GPIO3_PADDR                    0x48057000
#define GPIO4_PADDR                    0x48059000
#define GPIO5_PADDR                    0x4805b000
#define GPIO6_PADDR                    0x4805d000

#define GPTIMER1_PADDR                 0x4a318000
#define GPTIMER2_PADDR                 0x48032000
#define GPTIMER3_PADDR                 0x48034000
#define GPTIMER4_PADDR                 0x48036000
#define GPTIMER5_PADDR                 0x40138000
#define GPTIMER6_PADDR                 0x4013a000
#define GPTIMER7_PADDR                 0x4013c000
#define GPTIMER8_PADDR                 0x4013e000
#define GPTIMER9_PADDR                 0x4803e000
#define GPTIMER10_PADDR                0x48086000
#define GPTIMER11_PADDR                0x48088000

#define I2C1_PADDR                     0x48070000
#define I2C2_PADDR                     0x48072000
#define I2C3_PADDR                     0x48060000
#define I2C4_PADDR                     0x48350000

#define DISPLAY_SUBSYSTEM_PADDR        0x48040000

#define MMC_SD_SDIO1_PADDR             0x4809c000
#define MMC_SD_SDIO2_PADDR             0x480b4000
#define MMC_SD_SDIO3_PADDR             0x480ad000
#define MMC_SD_SDIO4_PADDR             0x480d1000
#define MMC_SD_SDIO5_PADDR             0x480d5000

#define MAILBOX_PADDR                  0x4a0f4000

/* Boot space */
/* 0x00000000 - 0x40000000 */
#define GPMC_PADDR                     0x00000000 /* 1GB */

/* FIXME: This is part of beagleboard, not SoC. Need to differentiate. */
#define ETHERNET_BASE_PADDR            0x2C000000

/* On-chip memory */
/* 0x40000000 - 0x48000000 */
#define BOOT_ROM0_PADDR                0x40000000 /* 20 pages */
#define BOOT_ROM1_PADDR                0x40014000 /* 8 pages */
#define SRAM_INTERNAL_PADDR            0x40200000 /* 16 pages */

/* L4 core (2 pages each unless specified) */
/* 0x48000000 - 0x48300000 */
#define SYSTEM_CONTROL_MODULE_PADDR    0x48002000
#define CLOCK_MANAGER_PADDR            0x48004000
#define L4_CORE_CONFIG_PADDR           0x48040000
#define SDMA_PADDR                     0x48056000
#define USBTLL_PADDR                   0x48062000
#define HS_USB_HOST_PADDR              0x48064000
#define MCBSP1_PADDR                   0x48074000
#define MCBSP5_PADDR                   0x48096000
#define MCSPI1_PADDR                   0x48098000
#define MCSPI2_PADDR                   0x4809a000
#define HS_USB_OTG_PADDR               0x480ab000
#define HDQ_TM_1WIRE_PADDR             0x480b2000
#define ICR_MPU_PORT_PADDR             0x480b6000
#define MCSPI3_PADDR                   0x480b8000
#define MCSPI4_PADDR                   0x480ba000
#define CAMERA_ISP_PADDR               0x480bc000
#define SR1_PADDR                      0x480c9000
#define SR2_PADDR                      0x480cb000
#define ICR_MODEM_PORT_PADDR           0x480cd000
#define INTC_PADDR                     0x48200000 /* 1 page - see IRQ chapter */
#define L4_WAKEUP_INTERCONNECT_A_PADDR 0x48300000
#define CONTROL_MODULE_ID_CODE_PADDR   0x4830a000
#define L4_WAKEUP_INTERCONNECT_B_PADDR 0x4830c000


/* L4 peripherals (2 pages each) */
/* 0x49000000 - 0x50000000 */
#define L4_PER_CONFIG_PADDR            0x49000000
#define MCBSP2_PADDR                   0x49022000
#define MCBSP3_PADDR                   0x49024000
#define MCBSP4_PADDR                   0x49026000
#define MCBSP2_SIDETONE_PADDR          0x49028000
#define MCBSP3_SIDETONE_PADDR          0x4902a000
#define WDT3_PADDR                     0x49030000

/* SGX */
/* 0x50000000 - 0x54000000 */
#define SGX_PADDR                      0x50000000 /* 16 pages */

/* L4 emu (2 pages each unless specified) */
/* 0x54000000 - 0x58000000 */
#define EMU_TPIU_PADDR                 0x54019000
#define EMU_ETB_PADDR                  0x5401b000
#define EMU_DAPCTL_PADDR               0x5401d000
#define EMU_SDTI_L4_INTERCONNECT_PADDR 0x5401f000
#define EMU_SDTI_CONFIG_PADDR          0x54500000
#define EMU_SDTI_WINDOW_PADDR          0x54600000
#define EMU_PWR_AND_RST_MANAGER_PADDR  0x54706000
#define EMU_GPIO1_PADDR                0x54710000
#define EMU_WDT2_PADDR                 0x54714000
#define EMU_GPTIMER1_PADDR             0x54718000
#define EMU_32KTIMER_PADDR             0x54720000
#define EMU_L4_WAKEUP_CONFIG_PADDR     0x54728000

/* IVA 2.2 Subsystem */
/* 0x5C000000 - 0x60000000 */
#define IVA_22_PADDR                   0x5C000000 /* 48 MB */

/* Level 3 Interconnect */
/* 0x68000000 - 0x69000000 */
#define L3_CONTROL_PADDR               0x68000000 /* 81KB */
#define L3_SMS_CONFIG                  0x68C00000 /* 16MB */
#define L3_SDRC_CONFIG                 0x68D00000 /* 16MB */
#define L3_GPMC_CONFIG                 0x68E00000 /* 16MB */

#endif
