/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include <utils/util.h>
#include <autoconf.h>
#include <platsupport/timer.h>
#include <vka/vka.h>
#include <vspace/vspace.h>
#include <simple/simple.h>
#include <sel4platsupport/timer.h>
#include <platsupport/raw_io.h>
#include <platsupport/plat/timer.h>
#include <platsupport/plat/machine.h>
#include <platsupport/plat/platform-conf.h>
#include "../../timer_common.h"


//Clock Source in A20, The CCU(Clock Control Unit) is made up of 7 PLLs.
enum tagA20ClockSource {
    CLK_LowerOSC  = 32768,
    CLK_PLL6      = 200 * 1024 * 1024,
    CLK_OSC24M    = 24 * 1024 * 1024,
    CLK_CLKIN0    = 24 * 1024 * 1024,
    CLK_AHB       = 276 * 1024 * 1024,
    CLK_USB       = 480 * 1024 * 1024,
    CLK_CPU32     = 1200 * 1024 * 1024
};


static seL4_timer_t *get_a20t(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr aep, int id, uint32_t prescaler);
static void destroy_a20t(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace);
pstimer_t *a20t_get_timer(void *vaddr, uint32_t prescaler);

seL4_timer_t *sel4platsupport_get_default_timer(vka_t *vka, vspace_t *vspace, simple_t *simple, seL4_CPtr aep)
{
    return get_a20t(vspace, simple, vka, aep, 0, 1);
}


static seL4_timer_t *get_a20t(vspace_t *vspace, simple_t *simple, vka_t *vka, seL4_CPtr aep, int id, uint32_t prescaler)
{

    void *paddr;
    uint32_t irq;

    seL4_timer_t *timer = calloc(1, sizeof(seL4_timer_t));
    if (timer == NULL) {
        LOG_ERROR("Failed to allocate object of size %u\n", sizeof(seL4_timer_t));
        goto error;
    }

    paddr = (void *)0x01c20000;
    switch (id) {
        case 0:
            irq = TIMER_0;
            break;
        case 1:
            irq = TIMER_1;
            break;
        case 2:
            irq = TIMER_2;
            break;
        case 3:
            irq = TIMER_3;
            break;
        case 4:
            irq = TIMER_4;
            break;
        case 5:
            irq = TIMER_5;
            break;
        default:
            LOG_ERROR("Wrong id %d\n", id);
            goto error;
    }

    timer_common_data_t *data = timer_common_init(vspace, simple, vka, aep, irq, paddr);
    timer->data = data;

    if (timer->data == NULL) {
         goto error;
    }

    timer->handle_irq = timer_common_handle_irq;

    timer->timer = a20t_get_timer(data->vaddr, prescaler);
    if (timer->timer == NULL) {
        goto error;
    }

    /* success */
    return timer;
error:
    if (timer != NULL) {
        timer_common_destroy(timer->data, vka, vspace);
        free(timer);
    }

    return NULL;
}

static void destroy_a20t(seL4_timer_t *timer, vka_t *vka, vspace_t *vspace)
{
    timer_stop(timer->timer);
    timer_common_destroy(timer->data, vka, vspace);
    free(timer);
}


typedef enum {
    PERIODIC_CONTINUOUS_MODE = 0,
    ONESHOT_SINGLE_MODE      = 1
} a20t_mode_t;

typedef struct tagA20TIMER {
    uint32_t a20t_map;
    uint64_t counter_start;
    uint32_t irq;
    int      id;
    uint32_t prescaler;
} A20TIMER;

static int a20t_timer_start(const pstimer_t *timer)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;

    switch (a20t->id) {
    	case 0:
            rawWriteUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF, rawReadUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF) | BIT(1) | BIT(0));
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_EN_REG_OFF, BIT(0));
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_STA_REG_OFF, BIT(0));
            break;
        case 1:
            rawWriteUInt32(a20t->a20t_map + TMR1_CTRL_REG_OFF, rawReadUInt32(a20t->a20t_map + TMR1_CTRL_REG_OFF) | BIT(1) | BIT(0));
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_EN_REG_OFF, BIT(1));
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_STA_REG_OFF, BIT(1));
            break;
    }

    return 0;
}


static int a20t_timer_stop(const pstimer_t *timer)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;

    switch (a20t->id) {
    	case 0:
            rawWriteUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF, rawReadUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF) & (~(BIT(0))));
            break;
        case 1:
            rawWriteUInt32(a20t->a20t_map + TMR1_CTRL_REG_OFF, rawReadUInt32(a20t->a20t_map + TMR1_CTRL_REG_OFF) & (~(BIT(0))));
            break;
    }

    return 0;
}

void configure_timeout(const pstimer_t *timer, uint64_t ns) {

    A20TIMER *a20t = (A20TIMER *)timer->data;

    switch (a20t->id) {
        case 0:
            rawWriteUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF, BIT(0) );        /* Make timer_0 pause */
            while ( ! (rawReadUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF) & BIT(0)) );
            rawWriteUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF, BIT(7) | (a20t->prescaler << 4) | (0x1 << 2) );
            rawWriteUInt32(a20t->a20t_map + TMR0_INTV_VALUE_REG_OFF, TIMER_CLK_INTERVAL_TICKS(CLK_OSC24M,ns)/a20t->prescaler);
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_EN_REG_OFF, BIT(0));
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_STA_REG_OFF, BIT(0));
            break;
    }
}

static int a20t_oneshot_absolute(const pstimer_t *timer, uint64_t ns)
{
    assert(!"Not implemented");
    return ENOSYS;
}


static int a20t_periodic(const pstimer_t *timer, uint64_t ns)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;

/* Set autoreload and start the timer. */
    configure_timeout(timer, ns);
    switch (a20t->id) {
        case 0:
            /*rawWriteUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF, rawReadUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF) | (a20t->prescaler << 4) | BIT(1) );*/
            /*rawWriteUInt32(a20t->a20t_map + TMR0_INTV_VALUE_REG_OFF, TIMER_INTERVAL_TICKS(ns));*/
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_EN_REG_OFF, BIT(0));
            rawWriteUInt32(a20t->a20t_map + TMR_IRQ_STA_REG_OFF, BIT(0));
            break;
    }

    return 0;
}

static int a20t_oneshot_relative(const pstimer_t *timer, uint64_t ns)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;

    configure_timeout(timer, ns);

    rawWriteUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF, rawReadUInt32(a20t->a20t_map + TMR0_CTRL_REG_OFF) | BIT(1) | BIT(0));

    return 0;
}

static void a20t_handle_irq(const pstimer_t *timer, uint32_t irq)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;

    /*rawWriteUInt32(a20t->a20t_map + TMR_IRQ_EN_REG_OFF, BIT(0));*/
    if (rawReadUInt32(a20t->a20t_map + TMR_IRQ_STA_REG_OFF) & BIT(0)) {
        rawWriteUInt32(a20t->a20t_map + TMR_IRQ_STA_REG_OFF, BIT(0));
    }
}

static uint64_t a20t_get_time(const pstimer_t *timer)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;
    uint32_t value;
    uint64_t ns;

    /* Set autoreload and start the timer. */
    switch (a20t->id) {
        case 0:
            value = rawReadUInt32(a20t->a20t_map + TMR0_CUR_VALUE_REG);
            break;
    }

    /*ns = TIMER_INTERVAL_NS(value);*/
    ns = TIMER_CLK_INTERVAL_NS(CLK_OSC24M, value) * 1000 * 1000;

    return ns;
}

static uint32_t a20t_get_nth_irq(const pstimer_t *timer, uint32_t n)
{
    A20TIMER *a20t = (A20TIMER *)timer->data;

    switch (n) {
        case 0:
            return TIMER_0;
        case 1:
            return TIMER_1;
        case 2:
            return TIMER_2;
        case 3:
            return TIMER_3;
        case 4:
            return TIMER_4;
        case 5:
            return TIMER_5;
    }
    return 0;
}

static pstimer_t singleton_timer;
static A20TIMER singleton_a20t;

pstimer_t *a20t_get_timer(void *vaddr, uint32_t prescaler)
{
    pstimer_t *timer = &singleton_timer;
    A20TIMER *a20t = &singleton_a20t;

    if (prescaler > 7) {
        fprintf(stderr, "Prescaler value set too large for device, value: %d, max 7", prescaler);
        return NULL;
    }
    timer->properties.upcounter = true;
    timer->properties.timeouts = true;
    timer->properties.bit_width = 32;
    timer->properties.irqs = 1;

    timer->data = (void *)a20t;
    timer->start = a20t_timer_start;
    timer->stop = a20t_timer_stop;
    timer->get_time = a20t_get_time;
    timer->oneshot_absolute = a20t_oneshot_absolute;
    timer->oneshot_relative = a20t_oneshot_relative;
    timer->periodic = a20t_periodic;
    timer->handle_irq = a20t_handle_irq;
    timer->get_nth_irq = a20t_get_nth_irq;

    a20t->prescaler = prescaler;
    a20t->a20t_map = vaddr + 0xc00;

    return timer;
}
