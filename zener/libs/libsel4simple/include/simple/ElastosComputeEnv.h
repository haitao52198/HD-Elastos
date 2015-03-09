/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef __TEST_H
#define __TEST_H

#include <autoconf.h>
#include <sel4/bootinfo.h>
#include <sel4/sel4.h>
#include <vka/vka.h>
#include <vka/object.h>
#include <simple/simple.h>
#include <vspace/vspace.h>

/* max test name size */
#define TEST_NAME_MAX 20

/* Increase if the sel4test-tests binary
 * has new loadable sections added */
#define MAX_REGIONS 4

typedef struct sel4utils_elf_region {
    seL4_CapRights rights;
    /* These two vstarts may differ if the elf was not mapped 1to1. Such an elf is not
     * runnable, but allows it to be loaded into a vspace where it is not intended to be run */
    void *elf_vstart;
    void *reservation_vstart;
    uint32_t size;
    reservation_t reservation;
    int cacheable;
} sel4utils_elf_region_t;


/* data shared between sel4test-driver and the sel4test-tests app.
 * all caps are in the sel4test-tests process' cspace */
typedef struct {

    /*
     * The compute environment struct type
     * This must be the first field of this kind of sturct.
     */
    uint32_t  ComputeEnvType;

    /* page directory of the test process */
    seL4_CPtr page_directory;
    /* root cnode of the test process */
    seL4_CPtr root_cnode;
    /* tcb of the test process */
    seL4_CPtr tcb;
    /* the domain cap */
    seL4_CPtr domain;
#ifndef CONFIG_KERNEL_STABLE
    /* asid pool cap for the test process to use when creating new processes */
    seL4_CPtr asid_pool;
#endif
#ifdef CONFIG_IOMMU
    seL4_CPtr io_space;
#endif /* CONFIG_IOMMU */
    /* cap to the sel4platsupport default timer irq handler */
    seL4_CPtr timer_irq;
#ifdef CONFIG_ARCH_ARM
    /* cap to the sel4platsupport default timer physical frame */
    seL4_CPtr timer_frame;
#endif
#ifdef CONFIG_ARCH_IA32
    /* cap to the sel4platsupport default timer io port */
    seL4_CPtr io_port;
#endif

    /* size of the test processes cspace */
    seL4_Word cspace_size_bits;
    /* range of free slots in the cspace */
    seL4_SlotRegion free_slots;

    /* range of untyped memory in the cspace */
    seL4_SlotRegion untypeds;
    /* size of untyped that each untyped cap corresponds to
     * (size of the cap at untypeds.start is untyped_size_bits_lits[0]) */
    uint8_t untyped_size_bits_list[CONFIG_MAX_NUM_BOOTINFO_UNTYPED_CAPS];
    /* name of the test to run */
    char name[TEST_NAME_MAX];
    /* priority the test process is running at */
    int priority;

    /* List of elf regions in the test process image, this
     * is provided so the test process can launch copies of itself.
     *
     * Note: copies should not rely on state from the current process
     * or the image. Only use copies to run code functions, pass all
     * required state as arguments. */
    sel4utils_elf_region_t elf_regions[MAX_REGIONS];

    /* the number of elf regions */
    int num_elf_regions;
} compute_env_data_t;


/* ===================== Copy from <sel4test/test.h> ========================= */
/* Fails a test case, stop everything. */
static inline void _test_abort()
{
    printf("Halting on fatal assertion...\n");
#ifdef CONFIG_DEBUG_BUILD
    seL4_DebugHalt();
#endif /* CONFIG_DEBUG_BUILD */
    while(1);
}

#define test_assert_fatal(e) if (!(e)) _test_abort()

#endif /* __TEST_H */
