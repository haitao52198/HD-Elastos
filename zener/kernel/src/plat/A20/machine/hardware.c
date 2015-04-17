/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <types.h>
#include <machine/io.h>
#include <kernel/vspace.h>
#include <arch/machine.h>
#include <arch/kernel/vspace.h>
#include <plat/machine.h>
#include <arch/linker.h>
#include <plat/machine/devices.h>
#include <plat/machine/hardware.h>

#include <jiffies.h>

/* Available physical memory regions on platform (RAM minus kernel image). */
/* NOTE: Regions are not allowed to be adjacent! */

const p_region_t BOOT_RODATA avail_p_regs[] = {
    /* 512 MiB */
    { .start = 0x40000000, .end = 0x80000000 }
};

BOOT_CODE int get_num_avail_p_regs(void)
{
    return sizeof(avail_p_regs) / sizeof(p_region_t);
}

BOOT_CODE p_region_t get_avail_p_reg(unsigned int i)
{
    return avail_p_regs[i];
}

//temporary define
#define A20GPT_DEVICE_PADDR 0x01c20000

const p_region_t BOOT_RODATA dev_p_regs[] = {
    /* sorted by increasing memory address */
    /* region caps must be a power of 2. */
    { A20GPT_DEVICE_PADDR           , A20GPT_DEVICE_PADDR            + ( 2 << PAGE_BITS) },


#if 0
    /* Boot space */
    /* 0x00000000 - 0x40000000 */
//  { GPMC_PADDR                    , GPMC_PADDR                     + ( 1 << 30       ) },

    /* TODO: Board specific devices should ultimately be replaced with a more general solution. */
    { ETHERNET_BASE_PADDR           , ETHERNET_BASE_PADDR            + ( 1 << PAGE_BITS) },

    /* On-chip memory */
    /* 0x40000000 - 0x48000000 */
//  { BOOT_ROM0_PADDR               , BOOT_ROM0_PADDR                + (20 << PAGE_BITS) },
//  { BOOT_ROM1_PADDR               , BOOT_ROM1_PADDR                + ( 8 << PAGE_BITS) },
//  { SRAM_INTERNAL_PADDR           , SRAM_INTERNAL_PADDR            + (16 << PAGE_BITS) },

    /* L4 core (2 pages each unless specified) */
    /* 0x48000000 - 0x48300000 */
    { SYSTEM_CONTROL_MODULE_PADDR   , SYSTEM_CONTROL_MODULE_PADDR    + ( 2 << PAGE_BITS) },
    { CLOCK_MANAGER_PADDR           , CLOCK_MANAGER_PADDR            + ( 2 << PAGE_BITS) },
    { L4_CORE_CONFIG_PADDR          , L4_CORE_CONFIG_PADDR           + ( 2 << PAGE_BITS) },
    { DISPLAY_SUBSYSTEM_PADDR       , DISPLAY_SUBSYSTEM_PADDR        + ( 2 << PAGE_BITS) },
    { SDMA_PADDR                    , SDMA_PADDR                     + ( 2 << PAGE_BITS) },
    { I2C3_PADDR                    , I2C3_PADDR                     + ( 2 << PAGE_BITS) },
    { USBTLL_PADDR                  , USBTLL_PADDR                   + ( 2 << PAGE_BITS) },
    { HS_USB_HOST_PADDR             , HS_USB_HOST_PADDR              + ( 2 << PAGE_BITS) },
    { UART0_PADDR                   , UART0_PADDR                    + ( 1 << PAGE_BITS) },
    { UART1_PADDR                   , UART1_PADDR                    + ( 2 << PAGE_BITS) },
    { UART2_PADDR                   , UART2_PADDR                    + ( 2 << PAGE_BITS) },
    { I2C1_PADDR                    , I2C1_PADDR                     + ( 2 << PAGE_BITS) },
    { I2C2_PADDR                    , I2C2_PADDR                     + ( 2 << PAGE_BITS) },
    { MCBSP1_PADDR                  , MCBSP1_PADDR                   + ( 2 << PAGE_BITS) },
    { GPTIMER10_PADDR               , GPTIMER10_PADDR                + ( 2 << PAGE_BITS) },
//  { GPTIMER11_PADDR               , GPTIMER11_PADDR                + ( 2 << PAGE_BITS) },
    { MAILBOX_PADDR                 , MAILBOX_PADDR                  + ( 2 << PAGE_BITS) },
    { MCBSP5_PADDR                  , MCBSP5_PADDR                   + ( 2 << PAGE_BITS) },
    { MCSPI1_PADDR                  , MCSPI1_PADDR                   + ( 2 << PAGE_BITS) },
    { MCSPI2_PADDR                  , MCSPI2_PADDR                   + ( 2 << PAGE_BITS) },
    { MMC_SD_SDIO1_PADDR            , MMC_SD_SDIO1_PADDR             + ( 2 << PAGE_BITS) },
    { HS_USB_OTG_PADDR              , HS_USB_OTG_PADDR               + ( 2 << PAGE_BITS) },
    { MMC_SD_SDIO3_PADDR            , MMC_SD_SDIO3_PADDR             + ( 2 << PAGE_BITS) },
    { HDQ_TM_1WIRE_PADDR            , HDQ_TM_1WIRE_PADDR             + ( 2 << PAGE_BITS) },
    { MMC_SD_SDIO2_PADDR            , MMC_SD_SDIO2_PADDR             + ( 2 << PAGE_BITS) },
    { ICR_MPU_PORT_PADDR            , ICR_MPU_PORT_PADDR             + ( 2 << PAGE_BITS) },
    { MCSPI3_PADDR                  , MCSPI3_PADDR                   + ( 2 << PAGE_BITS) },
    { MCSPI4_PADDR                  , MCSPI4_PADDR                   + ( 2 << PAGE_BITS) },
    { CAMERA_ISP_PADDR              , CAMERA_ISP_PADDR               + ( 2 << PAGE_BITS) },
    { SR1_PADDR                     , SR1_PADDR                      + ( 2 << PAGE_BITS) },
    { SR2_PADDR                     , SR2_PADDR                      + ( 2 << PAGE_BITS) },
    { ICR_MODEM_PORT_PADDR          , ICR_MODEM_PORT_PADDR           + ( 2 << PAGE_BITS) },
//  { INTC_PADDR                    , INTC_PADDR                     + ( 1 << PAGE_BITS) },
    { L4_WAKEUP_INTERCONNECT_A_PADDR, L4_WAKEUP_INTERCONNECT_A_PADDR + ( 2 << PAGE_BITS) },
    { CONTROL_MODULE_ID_CODE_PADDR  , CONTROL_MODULE_ID_CODE_PADDR   + ( 2 << PAGE_BITS) },
    { L4_WAKEUP_INTERCONNECT_B_PADDR, L4_WAKEUP_INTERCONNECT_B_PADDR + ( 2 << PAGE_BITS) },

    /* L4 Wakeup (2 pages each unless specified) */
    /* 0x48300000 - 0x49000000 */
    { PWR_AND_RST_MANAGER_PADDR     , PWR_AND_RST_MANAGER_PADDR      + ( 2 << PAGE_BITS) },
    { GPIO1_PADDR                   , GPIO1_PADDR                    + ( 2 << PAGE_BITS) },
    { WDT2_PADDR                    , WDT2_PADDR                     + ( 2 << PAGE_BITS) },
    { GPTIMER1_PADDR                , GPTIMER1_PADDR                 + ( 2 << PAGE_BITS) },
    { TIMER32K_PADDR                , TIMER32K_PADDR                 + ( 2 << PAGE_BITS) },
    { L4_WAKEUP_CONFIG_PADDR        , L4_WAKEUP_CONFIG_PADDR         + ( 2 << PAGE_BITS) },

    /* L4 peripherals (2 pages each) */
    /* 0x49000000 - 0x50000000 */
    { L4_PER_CONFIG_PADDR           , L4_PER_CONFIG_PADDR            + ( 2 << PAGE_BITS) },
    { UART3_PADDR                   , UART3_PADDR                    + ( 2 << PAGE_BITS) },
    { MCBSP2_PADDR                  , MCBSP2_PADDR                   + ( 2 << PAGE_BITS) },
    { MCBSP3_PADDR                  , MCBSP3_PADDR                   + ( 2 << PAGE_BITS) },
    { MCBSP4_PADDR                  , MCBSP4_PADDR                   + ( 2 << PAGE_BITS) },
    { MCBSP2_SIDETONE_PADDR         , MCBSP2_SIDETONE_PADDR          + ( 2 << PAGE_BITS) },
    { MCBSP3_SIDETONE_PADDR         , MCBSP3_SIDETONE_PADDR          + ( 2 << PAGE_BITS) },
    { WDT3_PADDR                    , WDT3_PADDR                     + ( 2 << PAGE_BITS) },
    { GPTIMER2_PADDR                , GPTIMER2_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER3_PADDR                , GPTIMER3_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER4_PADDR                , GPTIMER4_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER5_PADDR                , GPTIMER5_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER6_PADDR                , GPTIMER6_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER7_PADDR                , GPTIMER7_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER8_PADDR                , GPTIMER8_PADDR                 + ( 2 << PAGE_BITS) },
    { GPTIMER9_PADDR                , GPTIMER9_PADDR                 + ( 2 << PAGE_BITS) },
    { UART4_PADDR                   , UART4_PADDR                    + ( 2 << PAGE_BITS) },
    { GPIO2_PADDR                   , GPIO2_PADDR                    + ( 2 << PAGE_BITS) },
    { GPIO3_PADDR                   , GPIO3_PADDR                    + ( 2 << PAGE_BITS) },
    { GPIO4_PADDR                   , GPIO4_PADDR                    + ( 2 << PAGE_BITS) },
    { GPIO5_PADDR                   , GPIO5_PADDR                    + ( 2 << PAGE_BITS) },
    { GPIO6_PADDR                   , GPIO6_PADDR                    + ( 2 << PAGE_BITS) },

    /* SGX */
    /* 0x50000000 - 0x54000000 */
    { SGX_PADDR                     , SGX_PADDR                      + (16 << PAGE_BITS) },

    /* L4 emu (2 pages each unless specified) */
    /* 0x54000000 - 0x58000000 */
    { EMU_TPIU_PADDR                , EMU_TPIU_PADDR                 + ( 2 << PAGE_BITS) },
    { EMU_ETB_PADDR                 , EMU_ETB_PADDR                  + ( 2 << PAGE_BITS) },
    { EMU_DAPCTL_PADDR              , EMU_DAPCTL_PADDR               + ( 2 << PAGE_BITS) },
    { EMU_SDTI_L4_INTERCONNECT_PADDR, EMU_SDTI_L4_INTERCONNECT_PADDR + ( 1 << PAGE_BITS) },
    { EMU_SDTI_CONFIG_PADDR         , EMU_SDTI_CONFIG_PADDR          + ( 1 << PAGE_BITS) },
    { EMU_SDTI_WINDOW_PADDR         , EMU_SDTI_WINDOW_PADDR          + ( 1 << 20       ) },
    { EMU_PWR_AND_RST_MANAGER_PADDR , EMU_PWR_AND_RST_MANAGER_PADDR  + ( 4 << PAGE_BITS) },
    { EMU_GPIO1_PADDR               , EMU_GPIO1_PADDR                + ( 2 << PAGE_BITS) },
//  { EMU_WDT2_PADDR                , EMU_WDT2_PADDR                 + ( 2 << PAGE_BITS) },
    { EMU_GPTIMER1_PADDR            , EMU_GPTIMER1_PADDR             + ( 2 << PAGE_BITS) },
    { EMU_32KTIMER_PADDR            , EMU_32KTIMER_PADDR             + ( 2 << PAGE_BITS) },
    { EMU_L4_WAKEUP_CONFIG_PADDR    , EMU_L4_WAKEUP_CONFIG_PADDR     + ( 3 << PAGE_BITS) },

    /* IVA 2.2 Subsystem */
    /* 0x5C000000 - 0x60000000 */
    { IVA_22_PADDR                  , IVA_22_PADDR                   + (48 << 20       ) },

    /* Level 3 Interconnect */
    /* 0x68000000 - 0x70000000 */
    { L3_CONTROL_PADDR              , L3_CONTROL_PADDR               + (16 << 20       ) },
//  { L3_SMS_CONFIG                 , L3_SMS_CONFIG                  + (16 << 20       ) },
//  { L3_SDRC_CONFIG                , L3_SDRC_CONFIG                 + (16 << 20       ) },
//  { L3_GPMC_CONFIG                , L3_GPMC_CONFIG                 + (16 << 20       ) }
#endif
};

BOOT_CODE int get_num_dev_p_regs(void)
{
    return sizeof(dev_p_regs) / sizeof(p_region_t);
}

BOOT_CODE p_region_t get_dev_p_reg(unsigned int i)
{
    return dev_p_regs[i];
}


BOOT_CODE void
map_kernel_devices(void)
{
    /* map kernel device: GIC_PL390 */
	map_kernel_frame(
		AW_TIMER_MAP_BASE,
		ARM_MP_PPTR1,
		VMKernelOnly,
        vm_attributes_new(
            true,  /* armExecuteNever */
            false, /* armParityEnabled */
            false  /* armPageCacheable */
        )
	);
    map_kernel_frame(
		AW_GIC_DIST_BASE,
		ARM_MP_PPTR2,
        VMKernelOnly,
        vm_attributes_new(
            true,  /* armExecuteNever */
            false, /* armParityEnabled */
            false  /* armPageCacheable */
        )
    );
    map_kernel_frame(
        AW_GIC_CPU_BASE,
        ARM_MP_PPTR3,
        VMKernelOnly,
        vm_attributes_new(
            true,  /* armExecuteNever */
            false, /* armParityEnabled */
            false  /* armPageCacheable */
        )
    );

    /* map kernel device: L2CC_L2C310 */
    map_kernel_frame(
        L2CC_L2C310_PADDR,
        L2CC_L2C310_PPTR,
        VMKernelOnly,
        vm_attributes_new(
            true,  /* armExecuteNever */
            false, /* armParityEnabled */
            false  /* armPageCacheable */
        )
    );

    /* map kernel device: TIMERS */
    map_kernel_frame(
        AW_TIMER_MAP_BASE,
        ARM_MP_PPTR0,
        VMKernelOnly,
        vm_attributes_new(
            true,  /* armExecuteNever */
            false, /* armParityEnabled */
            false  /* armPageCacheable */
        )
    );

#ifdef DEBUG
    /* map kernel device: UART */
    map_kernel_frame(
        UART0_PADDR,
        UART0_PPTR,
        VMKernelOnly,
        vm_attributes_new(
            true,  /* armExecuteNever */
            false, /* armParityEnabled */
            false  /* armPageCacheable */
        )
    );
#endif
}

#define INTCPS_SYSCONFIG_SOFTRESET BIT(1)
#define INTCPS_SYSSTATUS_RESETDONE BIT(0)
#define INTCPS_SIR_IRQ_SPURIOUSIRQFLAG 0xFF0000

/*
 * The struct below is used to discourage the compiler from generating literals
 * for every single address we might access.
 */

/* Determine if the given IRQ should be reserved by the kernel. */
bool_t
isReservedIRQ(interrupt_t irq)
{
    return irq == KERNEL_TIMER_IRQ;
}

/* Handle a platform-reserved IRQ. */
void handleReservedIRQ(irq_t irq)
{
    /* We shouldn't be receiving any reserved IRQs anyway. */
    maskInterrupt(true, irq);

    return;
}

#define TIMER_INTERVAL_MS (CONFIG_TIMER_TICK_MS)

#define TICKS_PER_SECOND 32768
#define TIMER_INTERVAL_TICKS ((int)(1UL * TIMER_INTERVAL_MS * TICKS_PER_SECOND / 1000))

#define TMR_CTRL_ENABLE      BIT(0)
#define TMR_CTRL_AUTORELOAD  BIT(1)
#define TMR_CTRL_COMPENABLE  BIT(1)
#define TMR_CTRL_IRQEN       BIT(2)
#define TMR_CTRL_AUTOINC     BIT(3)
#define TMR_CTRL_PRESCALE    8

#define TMR_INTS_EVENT       BIT(0)

#define CLK_MHZ 400ULL
#define TIMER_COUNT_BITS 32

#define PRESCALE ((CLK_MHZ*1000 * TIMER_INTERVAL_MS) >> TIMER_COUNT_BITS)
#define TMR_LOAD ((CLK_MHZ*1000 * TIMER_INTERVAL_MS) / (PRESCALE + 1))

/* A7 MPCORE timer map */
/* timer registers */
volatile struct TIMER_map {
    uint32_t tier;          /* Timer IRQ Enable Register: 0x00 */
    uint32_t tisr;          /* Timer Status Register: 0x04 */
    uint64_t padding0;      /* Padding: 0x08 */
    uint32_t tcr0;          /* Timer0 Control Register: 0x10 */
    uint32_t tivr0;         /* Timer0 Interval Value Register: 0x14 */
    uint32_t tcvr0;         /* Timer0 Current Value Register: 0x18 */
    uint32_t padding1;      /* Padding: 0x1c */
    uint32_t tcr1;          /* Timer1 Control Register: 0x20 */
    uint32_t tivr1;         /* Timer1 Interval Value Register: 0x24 */
    uint32_t tcvr1;         /* Timer1 Current Value Register: 0x28 */
    uint32_t padding2;      /* Padding: 0x2c */
    uint32_t tcr2;          /* Timer2 Control Register: 0x30 */
    uint32_t tivr2;         /* Timer2 Interval Value Register: 0x34 */
    uint32_t tcvr2;         /* Timer2 Current Value Register: 0x38 */
    uint32_t padding3;      /* Padding: 0x3c */
    uint32_t tcr3;          /* Timer3 Control Register: 0x40 */
    uint32_t tivr3;         /* Timer3 Interval Value Register: 0x44 */
    uint32_t tcvr3;         /* Timer3 Current Value Register: 0x48 */
    uint32_t padding4;      /* Padding: 0x4c */
    uint32_t tcr4;          /* Timer4 Control Register: 0x50 */
    uint32_t tivr4;         /* Timer4 Interval Value Register: 0x54 */
    uint32_t tcvr4;         /* Timer4 Current Value Register: 0x58 */
    uint32_t padding5;      /* Padding: 0x5c */
    uint32_t tcr5;          /* Timer5 Control Register: 0x60 */
    uint32_t tivr5;         /* Timer5 Interval Value Register: 0x64 */
    uint32_t tcvr5;         /* Timer5 Current Value Register: 0x68 */
} *timer = (volatile void*) TIMERS_PPTR;

void
resetTimer(void)
{
    /*priv_timer->ints = TMR_INTS_EVENT;*/
    timer->tier |= BIT(5);
    timer->tisr |= BIT(5);
    timer->tcr5 = BIT(1) | BIT(0);
}

#define GLOB_TIMER_PRESCALE     3
uint64_t readGlobTimerCounter(void);
void writeGlobTimerCounter(uint64_t u64);

BOOT_CODE void
initTimer(void)
{
    /**
       Initialize Timer5 as Global Timer
     */
    /* reset */
    timer->tcvr5 = BIT(0);
    while (! (timer->tisr & BIT(5)) );

    maskInterrupt(true, TIMER_5);

    /* Set the reload value */
    timer->tcr5 = (PRESCALE << 4);
    timer->tier |= BIT(5);
    timer->tivr5 = TIMER_INTERVAL_TICKS;
    /* Clear timer5 pending */
    timer->tisr |= BIT(5);
    /* Set autoreload and start the timer */
    timer->tcr5 = BIT(1) | BIT(0);
}

inline uint64_t get_jiffies(void)
{
    /* No implement */
}

inline void set_jiffies(uint64_t u64)
{
    /* No implement */
}

