/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include "stdint.h"
#include "elfloader.h"
#include "stdio.h"

/* Short descriptor */
#define ARM_SECTION_BITS 20
/* Long descriptor */
#define ARM_1GB_BLOCK_BITS 30
#define ARM_2MB_BLOCK_BITS 21

/*
 * Why we need init_boot_pd()?
 *
 * The reason for also mapping the physical address of the segment is no MMU
 * on now. After a while for opening the MMU, the contents of the kernel code
 * segment supposedly doing a mapped, so that the PC value from CPU to the MMU
 * should be the virtual address. However, due to the prefetch function of
 * pipeline, there are several PC values which are not virtual but physical
 * address. If there is no corresponding address mappings, MMU will not
 * know which part of the actual physical memory access and cause problems.
 * So it is necessary to write this part of the physical address in the
 * corresponding page table entries L1. in essence, it's "PA = PA" (many other
 * articles refers "PA = VA". in fact, it's same. But it's easy to make people
 * misunderstand, because it's really that there are several address from CPU
 * access being physical. If don't create mapping, MMU cannot understand.
 * So we write L1 page table entries related to the physical page here, and
 * "PA = PA" is actually doing the mapping to MMU)
 */

/*
 * Create a "boot" page directory, which contains a 1:1 mapping below
 * the kernel's first vaddr, and a virtual-to-physical mapping above the
 * kernel's first vaddr.
 */
void init_boot_pd(struct image_info *kernel_info)
{
    uint32_t i;
    vaddr_t first_vaddr = kernel_info->virt_region_start;
    paddr_t first_paddr = kernel_info->phys_region_start;

    /* identity mapping below kernel window */
    for (i = 0; i < (first_vaddr >> ARM_SECTION_BITS); i++) {
        _boot_pd[i] = (i << ARM_SECTION_BITS)
                      | BIT(10) /* kernel-only access */
#ifdef ARMV5
                      | BIT(4)  /* must be set for ARMv5 */
#endif
                      | BIT(1); /* 1M section */
    }

    /* mapping of kernel window */
    for (i = 0; i < ((-first_vaddr) >> ARM_SECTION_BITS); i++) {
        _boot_pd[i + (first_vaddr >> ARM_SECTION_BITS)]
            = ((i << ARM_SECTION_BITS) + first_paddr)
              | BIT(10) /* kernel-only access */
#ifdef ARMV5
              | BIT(4)  /* must be set for ARMv5 */
#endif
              | BIT(1); /* 1M section */
    }
}


/**
 * Performs the same operation as init_boot_pd, but initialises
 * the LPAE page table. In this case, 3 L2 tables are concatinated.
 * PGD entries point to the approprite L2 table.
 */
void init_lpae_boot_pd(struct image_info *kernel_info)
{
    uint32_t i, k;
    vaddr_t first_vaddr = kernel_info->virt_region_start;
    paddr_t first_paddr = kernel_info->phys_region_start;

    /* Map in L2 page tables */
    for(i = 0; i < 4; i++){
        _lpae_boot_pgd[i] = ((uintptr_t)_lpae_boot_pmd + (i << PAGE_BITS))
                           | BIT(1)  /* Page table */
                           | BIT(0); /* Valid */
    }
    /* identity mapping below kernel window */
    for (i = 0; i < (first_vaddr >> ARM_2MB_BLOCK_BITS); i++) {
        _lpae_boot_pmd[i] = (i << ARM_2MB_BLOCK_BITS)
                            | BIT(10) /* AF - Not always HW managed */
                            | BIT(0); /* Valid */
    }
    /* mapping of kernel window */
    for (k = 0; k < ((-first_vaddr) >> ARM_2MB_BLOCK_BITS); k++) {
        _lpae_boot_pmd[i + k] = ((k << ARM_2MB_BLOCK_BITS) + first_paddr)
                                | BIT(10) /* AF - Not always HW managed */
                                | BIT(0); /* Valid */
    }
}
