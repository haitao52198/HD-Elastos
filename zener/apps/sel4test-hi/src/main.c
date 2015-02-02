/*
 * Copyright 2014, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <allocman/vka.h>
#include <allocman/bootstrap.h>

#include <sel4/sel4.h>
#include <sel4/types.h>

#include <sel4platsupport/timer.h>
#include <sel4platsupport/plat/timer.h>
#include <platsupport/timer.h>

#ifdef CONFIG_IA32
#include <platsupport/plat/pit.h>
#endif

#include <sel4utils/util.h>
#include <sel4utils/mapping.h>
#include <sel4utils/vspace.h>

// #include <sel4test/test.h>

#include <vka/capops.h>

// #include "helpers.h"
#include "test.h"

struct env {
    /* An initialised vka that may be used by the test. */
    vka_t vka;
    /* virtual memory management interface */
    vspace_t vspace;
    /* initialised timer */
    seL4_timer_t *timer;
    /* abstract interface over application init */
    simple_t simple;
    /* aep for timer */
    vka_object_t timer_aep;

    /* caps for the current process */
    seL4_CPtr cspace_root;
    seL4_CPtr page_directory;
    seL4_CPtr endpoint;
    seL4_CPtr tcb;
#ifndef CONFIG_KERNEL_STABLE
    seL4_CPtr asid_pool;
#endif /* CONFIG_KERNEL_STABLE */
#ifdef CONFIG_IOMMU
    seL4_CPtr io_space;
#endif /* CONFIG_IOMMU */
    seL4_CPtr domain;

    int priority;
    int cspace_size_bits;
    int num_regions;
    sel4utils_elf_region_t regions[MAX_REGIONS];
};
typedef struct env *env_t;

/* dummy global for libsel4muslcsys */
char _cpio_archive[1];

/* endpoint to call back to the test driver on */
static seL4_CPtr endpoint;

/* global static memory for init */
static sel4utils_alloc_data_t alloc_data;

/* dimensions of virtual memory for the allocator to use */
#define ALLOCATOR_VIRTUAL_POOL_SIZE ((1 << seL4_PageBits) * 200)

/* allocator static pool */
#define ALLOCATOR_STATIC_POOL_SIZE ((1 << seL4_PageBits) * 10)
static char allocator_mem_pool[ALLOCATOR_STATIC_POOL_SIZE];

static test_init_data_t *
receive_init_data(seL4_CPtr endpoint)
{
    /* wait for a message */
    seL4_Word badge;
    UNUSED seL4_MessageInfo_t info;

    info = seL4_Wait(endpoint, &badge);

    /* check the label is correct */
    assert(seL4_MessageInfo_get_label(info) == seL4_NoFault);
    assert(seL4_MessageInfo_get_length(info) == 1);

    test_init_data_t *init_data = (test_init_data_t *) seL4_GetMR(0);
    assert(init_data->free_slots.start != 0);
    assert(init_data->free_slots.end != 0);

    return init_data;
}

static void
init_allocator(env_t env, test_init_data_t *init_data)
{
    UNUSED int error;
    UNUSED reservation_t virtual_reservation;

    /* initialise allocator */
    allocman_t *allocator = bootstrap_use_current_1level(init_data->root_cnode,
                                                         init_data->cspace_size_bits, init_data->free_slots.start,
                                                         init_data->free_slots.end, ALLOCATOR_STATIC_POOL_SIZE,
                                                         allocator_mem_pool);
    assert(allocator != NULL);

    allocman_make_vka(&env->vka, allocator);

    /* fill the allocator with untypeds */
    int slot, size_bits_index;
    for (slot = init_data->untypeds.start, size_bits_index = 0;
            slot <= init_data->untypeds.end;
            slot++, size_bits_index++) {

        cspacepath_t path;
        vka_cspace_make_path(&env->vka, slot, &path);
        /* allocman doesn't require the paddr unless we need to ask for phys addresses,
         * which we don't. */
        uint32_t fake_paddr = 0;
        uint32_t size_bits = init_data->untyped_size_bits_list[size_bits_index];
        error = allocman_utspace_add_uts(allocator, 1, &path, &size_bits, &fake_paddr);
        assert(!error);
    }

    /* create a vspace */
    void *existing_frames[] = { init_data, seL4_GetIPCBuffer()};
    error = sel4utils_bootstrap_vspace(&env->vspace, &alloc_data, init_data->page_directory, &env->vka,                 NULL, NULL, existing_frames);

    /* switch the allocator to a virtual memory pool */
    void *vaddr;
    virtual_reservation = vspace_reserve_range(&env->vspace, ALLOCATOR_VIRTUAL_POOL_SIZE,
                                               seL4_AllRights, 1, &vaddr);
    assert(virtual_reservation.res);
    bootstrap_configure_virtual_pool(allocator, vaddr, ALLOCATOR_VIRTUAL_POOL_SIZE,
                                     env->page_directory);

}

/* basic simple functions */
#ifdef CONFIG_ARCH_ARM
static seL4_Error
get_frame_cap(void *data, void *paddr, int size_bits, cspacepath_t *path)
{
    test_init_data_t *init = (test_init_data_t *) data;
    assert(paddr == (void*) DEFAULT_TIMER_PADDR);
    assert(size_bits == seL4_PageBits);


    int error = seL4_CNode_Copy(path->root, path->capPtr, path->capDepth, init->root_cnode,
                                init->timer_frame, seL4_WordBits, seL4_AllRights);
    assert(error == seL4_NoError);

    return error;
}
#endif /* CONFIG_ARCH_ARM */

#ifdef CONFIG_ARCH_IA32
seL4_CPtr get_IOPort_cap(void *data, uint16_t start_port, uint16_t end_port)
{
    test_init_data_t *init = (test_init_data_t *) data;
    assert(start_port >= PIT_IO_PORT_MIN);
    assert(end_port <= PIT_IO_PORT_MAX);

    return init->io_port;
}
#endif /* CONFIG_ARCH_IA32 */

static seL4_Error
get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth)
{
    test_init_data_t *init = (test_init_data_t *) data;
    assert(irq == DEFAULT_TIMER_INTERRUPT);

    int error = seL4_CNode_Copy(root, index, depth, init->root_cnode,
                                init->timer_irq, seL4_WordBits, seL4_AllRights);
    assert(error == 0);
    return error;
}

void init_timer(env_t env, test_init_data_t *init_data)
{
    /* minimal simple implementation to get the platform
     * default timer off the ground */
#ifdef CONFIG_ARCH_ARM
    env->simple.frame_cap = get_frame_cap;
#elif CONFIG_ARCH_IA32
    env->simple.IOPort_cap = get_IOPort_cap;
#endif
    env->simple.irq = get_irq;
    env->simple.data = (void *) init_data;

    UNUSED int error;


    error = vka_alloc_async_endpoint(&env->vka, &env->timer_aep);
    assert(error == 0);

    env->timer = sel4platsupport_get_default_timer(&env->vka, &env->vspace,
                                                   &env->simple, env->timer_aep.cptr);
    assert(env->timer != NULL);
}

int
main(int argc, char **argv)
{
    test_init_data_t *init_data;
    struct env env;

    /* parse args */
    endpoint = (seL4_CPtr) atoi(argv[0]);

    /* read in init data */
    init_data = receive_init_data(endpoint);

    /* configure env */
    env.cspace_root = init_data->root_cnode;
    env.page_directory = init_data->page_directory;
    env.endpoint = endpoint;
    env.priority = init_data->priority;
    env.cspace_size_bits = init_data->cspace_size_bits;
    env.tcb = init_data->tcb;
    env.domain = init_data->domain;
#ifndef CONFIG_KERNEL_STABLE
    env.asid_pool = init_data->asid_pool;
#endif /* CONFIG_KERNEL_STABLE */
#ifdef CONFIG_IOMMU
    env.io_space = init_data->io_space;
#endif
    env.num_regions = init_data->num_elf_regions;
    memcpy(env.regions, init_data->elf_regions, sizeof(sel4utils_elf_region_t) * env.num_regions);

    /* initialse cspace, vspace and untyped memory allocation */
    init_allocator(&env, init_data);

    /* initialise the timer */
    init_timer(&env, init_data);

    /* ================ Do it! =============== */
    printf("This is another process:\n\n>>> Hi~\n");
    
    return 0;
}


