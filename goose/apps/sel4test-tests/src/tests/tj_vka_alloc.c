/*
 * Copyright 2014, Tongji University Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <sel4/sel4.h>

#include "../helpers.h"
#include <allocman/vka.h>


static int
test_vka_allocator(env_t env, void *args)
{
    int err;
    allocman_t *a = env->vka.data;
    void *mem = a->mspace.alloc(a, a->mspace.mspace, 10, &err);
    if (mem) {
        printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n\
                allocator->mspace.alloc(allocator, allocator_mem_pool, 10, &err);\n");
        return SUCCESS;
    }

    return FAILURE;
}
DEFINE_TEST(ALLOC0000, "Test allocator", test_vka_allocator)
