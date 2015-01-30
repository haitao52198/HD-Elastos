<!-- MarkdownTOC -->

- fatload(u-boot)
- bootelf(u-boot)
- `main`(tools/elfloader/src/arch-arm/boot.c)
    - Unpack ELF image
        - Load kernel
        - Load userspace
    - Setup MMU
        - `init_boot_pd`(function)
        - `arm_enable_mmu`(function)
    - Enter kernel
        - `_start`(kernel/src/arch/arm/head.S)
- `init_kernel`(kernel/src/arch/arm/kernel/boot.c)
- `try_init_kernel`(function)
    - `map_kernel_window`(kernel/src/arch/arm/kernel/vspace.c)
    - `init_cpu`
        - `activate_global_pd`(kernel/src/arch/arm/kernel/vspace.c)
            - `cleanInvalidateL1Caches`
            - `setCurrentPD`
            - `invalidateTLB`
            - `lockTLBEntry(kernelBase)`
            - `lockTLBEntry(PPTR_VECTOR_TABLE)`
    - `init_plat`
        - `initIRQController`
        - `initTimer`
        - `initL2Cache`
    - `init_freemem`
    - About capability and thread
        - Create the root cnode
        - Create some basic capability
            - `create_domain_cap`
            - `create_irq_cnode`
        - `init_irqs`
        - bootinfo preparation
            - `allocate_bi_frame`
            - `create_it_address_space`
            - `create_bi_frame_cap`
        - IPC buffer
        - Frames for userland image
        - ASID pool
        - `create_idle_thread`
        - `cleanInvalidateL1Caches`
        - `create_initial_thread`
        - `create_untypeds`
    - `create_device_frames`
    - _no shared-frame caps_
    - `bi_finalise`
    - `cleanInvalidateL1Caches`

<!-- /MarkdownTOC -->

# fatload(u-boot)

```shell
fatload mmc 0 0x80000000 <image_name>
```

Load elf-image from disk to memory.

# bootelf(u-boot)

Jump to entry point of elf-image.

# `main`(tools/elfloader/src/arch-arm/boot.c)

## Unpack ELF image

### Load kernel

### Load userspace

We assume (and check) that the kernel is the first file in the archive,
and then load the (n+1)’th file in the archive onto the (n)’th CPU.

> We can load userspaces image for each CPU core.
> But the source code of seL4 just load for one CPU.

## Setup MMU

```c
/* Setup MMU. */
cpu_mode = read_cpsr() & CPSR_MODE_MASK;
    if(cpu_mode == CPSR_MODE_HYPERVISOR){
        printf(“Enabling hypervisor MMU and paging\n”);
        init_lpae_boot_pd(&kernel_info);
        arm_enable_hyp_mmu();
    }
}
/* If we are in HYP mode, we enable the SV MMU and paging
 * just in case the kernel does not support hyp mode. */
printf(“Enabling MMU and paging\n”);
init_boot_pd(&kernel_info);
arm_enable_mmu();
```

### `init_boot_pd`(function)

Create a “boot” page directory, which contains a 1:1 mapping below
the kernel’s first vaddr, and a virtual-to-physical mapping above the
kernel’s first vaddr.

### `arm_enable_mmu`(function)

This is a function writen by arm assembly language, defined in
‘tools/elfloader/src/arch-arm/mmu-v7a.S’.

It is expected that the code of this function will be mapped 1:1
virtual/physical in the pagetable we activate.

## Enter kernel

Jumping to kernel-image entry point.

```c
/* Jump to the kernel. */
((init_kernel_t)kernel_info.virt_entry)(user_info.phys_region_start,
                                        user_info.phys_region_end,
                                        user_info.phys_virt_offset,
                                        user_info.virt_entry);
```

kernel_info is a symbol whose address defined in 'linker.lds'.

kernel_info.virt_entry = 0xf0000000.

And this is the address of symbol `_start` defined in ‘kernel/src/arch/arm/head.S'.

### `_start`(kernel/src/arch/arm/head.S)

```arm
BEGIN_FUNC(_start)
    ...

    /* Call bootstrapping implemented in C */
    blx init_kernel

    /* Restore the initial thread */
    ldr r7, =ksCurThread
    ldr sp, [r7]
    add sp, sp, #PT_LR_svc
    ldmdb sp, {r0-lr}^
    rfeia sp
END_FUNC(_start)
```

Instruction `blx init_kernel` will call the function in 'kernel/src/arch/arm/kernel/boot.c' to initialize the kernel.

# `init_kernel`(kernel/src/arch/arm/kernel/boot.c)

This is the entry point of kernel-image.

Call function `try_init_kernel`.

# `try_init_kernel`(function)

## `map_kernel_window`(kernel/src/arch/arm/kernel/vspace.c)

1. Mapping of kernelBase (virtual address) to kernel’s physBase
up to end of virtual address space minus 16M using 16M frames[^1].
2. Mapping of the next 15M[^3] using 1M frames[^2].
3. Map page table covering last 1M of virtual address space to page directory.
4. Start initialising the page table.
5. Map vector table
6. Map globals table
7. Map stack table

[^1]: The size of a SuperSection is 16M.
    See `enum` type variable `vm_page_sie` and `frameSizeConstants`.
[^2]: The size of a normal Section is 1M. Refer to above.
[^3]: If Define `CONFIG_BENCHMARK`, just map 14M and steal the last MB for logging.

> What is `BANCHMARK`?

## `init_cpu`

### `activate_global_pd`(kernel/src/arch/arm/kernel/vspace.c)

Ensure that there’s nothing stale in newly-mapped regions, and
that everything we’ve written (particularly the kernel page tables)
is committed.

#### `cleanInvalidateL1Caches`

#### `setCurrentPD`

#### `invalidateTLB`

#### `lockTLBEntry(kernelBase)`

#### `lockTLBEntry(PPTR_VECTOR_TABLE)`

## `init_plat`

### `initIRQController`

### `initTimer`

### `initL2Cache`

## `init_freemem`

make the free memory available to alloc_region()

## About capability and thread

Some operations about capability and thread by following.

### Create the root cnode

```c
root_cnode_cap = create_root_cnode();
if (cap_get_capType(root_cnode_cap) == cap_null_cap) {
    return false;
}
```

In function `create_root_cnode`(kernel/src/kernel/boot.c):

1. Write the number of root CNode slots to global state.
2. Create an empty root CNode.
3. Write the root CNode cap into the root CNode.

### Create some basic capability

```c
/* create the cap for managing thread domains */
create_domain_cap(root_cnode_cap);

/* create the IRQ CNode */
if (!create_irq_cnode()) {
    return false;
}
```

#### `create_domain_cap`

Create the capability for managing thread domains.

```c
cap = cap_domain_cap_new();
write_slot(SLOT_PTR(pptr_of_cap(root_cnode_cap), BI_CAP_DOM), cap);
```

#### `create_irq_cnode`

Create an empty region for intStateIRQNode.

```c
pptr = alloc_region(PAGE_BITS);
...
memzero((void*)pptr, 1 << PAGE_BITS);
intStateIRQNode = (cte_t*)pptr;
```

### `init_irqs`

Initialise the IRQ states and provide the IRQ control cap.

This function set all IRQState as inactive, except of KERNEL_TIMER_IRQ set as Timer. And then write slot to provide the IRQ control capability.

### bootinfo preparation

#### `allocate_bi_frame`

Create the bootinfo frame.

#### `create_it_address_space`

Construct an initial address space with enough virtual addresses
to cover the user image + ipc buffer and bootinfo frames

#### `create_bi_frame_cap`

Create and map bootinfo frame cap.

### IPC buffer

```c
/* create the initial thread's IPC buffer */
ipcbuf_cap = create_ipcbuf_frame(root_cnode_cap, it_pd_cap, ipcbuf_vptr);
if (cap_get_capType(ipcbuf_cap) == cap_null_cap) {
    return false;
}
```

### Frames for userland image

Call `create_frames_of_region` to create a region for `ndks_boot.bi_frame->ui_frame_caps`.

### ASID pool

Create/initialise the initial thread's ASID pool.

```c
it_ap_cap = create_it_asid_pool(root_cnode_cap);
if (cap_get_capType(it_ap_cap) == cap_null_cap) {
    return false;
}
write_it_asid_pool(it_ap_cap, it_pd_cap);
```

### `create_idle_thread`

1. Allocate TCB(Thread Control Block) for idle thread.
2. configure idle thread by function `configureIdleThread`.

### `cleanInvalidateL1Caches`

### `create_initial_thread`

```c
BOOT_CODE bool_t
create_initial_thread(
    cap_t  root_cnode_cap,
    cap_t  it_pd_cap,
    vptr_t ui_v_entry,
    vptr_t bi_frame_vptr,
    vptr_t ipcbuf_vptr,
    cap_t  ipcbuf_cap
);
```

Above is the declare of function `create_initial_thread`. Process by following.

1. Allocate TCB.
2. Derive a copy of the IPC buffer cap for inserting.
3. Initialise TCB (corresponds directly to abstract specification).
4. Initialise TCB.
5. Initialise current thread pointer.
6. Create initial thread's TCB cap.

### `create_untypeds`

Convert the remaining free memory into UT objects and provide the caps.

## `create_device_frames`

create device frames

## _no shared-frame caps_

## `bi_finalise`

## `cleanInvalidateL1Caches`

