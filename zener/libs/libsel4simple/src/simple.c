/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#include <autoconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <sel4/sel4.h>
#include <simple/simple.h>
#include <simple/simple_helpers.h>
#include <simple/ElastosComputeEnv.h>

static void *simple_default_get_frame_info(void *data, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *offset);
static seL4_Error get_frame_cap(void *data, void *paddr, int size_bits, cspacepath_t *path);
static seL4_CPtr get_IOPort_cap(void *data, uint16_t start_port, uint16_t end_port);
static seL4_Error get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth);

#if defined(CONFIG_ARCH_ARM)
#define seL4_ARCH_ASIDPool_Assign      seL4_ARM_ASIDPool_Assign
#elif defined(CONFIG_ARCH_IA32) /* CONFIG_ARCH_ARM */
#define seL4_ARCH_ASIDPool_Assign      seL4_IA32_ASIDPool_Assign
#endif /* CONFIG_ARCH_IA32 */

int simple_is_untyped_cap(simple_t *simple, seL4_CPtr pos)
{
    int i;

    for(i = 0; i < simple_get_untyped_count(simple); i++) {
        uint32_t paddr;
        uint32_t size_bits;
        seL4_CPtr ut_pos = simple_get_nth_untyped(simple, i, &size_bits, &paddr);
        if(ut_pos == pos) {
            return 1;
        }
    }

    return 0;
}

seL4_Error simple_copy_caps(simple_t *simple, seL4_CNode cspace, int copy_untypeds)
{
    int i;
    seL4_Error error = seL4_NoError;

    for(i = 0; i < simple_get_cap_count(simple); i++) {
        seL4_CPtr pos = simple_get_nth_cap(simple, i);
        /* Don't copy this, put the cap to the new cspace */
        if(pos == seL4_CapInitThreadCNode) continue;

        /* If we don't want to copy untypeds and it is one move on */
        if(!copy_untypeds && simple_is_untyped_cap(simple, pos)) continue;

        error = seL4_CNode_Copy(
                    cspace, pos, seL4_WordBits,
                    seL4_CapInitThreadCNode, pos, seL4_WordBits,
                    seL4_AllRights);
        /* Don't error on the low cap numbers, we might have tried to copy a control cap */
        if(error && i > 10) {
            return error;
        }
    }

    return error;
}

int simple_vka_cspace_alloc(void *data, seL4_CPtr *slot)
{
    assert(data && slot);

    simple_t *simple = (simple_t *) data;
    seL4_CNode cnode = simple_get_cnode(simple);
    int i = 0;

    /* Keep trying to find the next free slot by seeing if we can copy something there */
    seL4_Error error = seL4_CNode_Copy(cnode, simple_get_cap_count(simple) + i, 32, cnode, cnode, 32, seL4_AllRights);
    while(error == seL4_DeleteFirst) {
        i++;
        error = seL4_CNode_Copy(cnode, simple_get_cap_count(simple) + i, 32, cnode, cnode, 32, seL4_AllRights);
    }

    if(error != seL4_NoError) {
        error = seL4_CNode_Delete(cnode, simple_get_cap_count(simple) + i, 32);
        return error;
    }

    error = seL4_CNode_Delete(cnode, simple_get_cap_count(simple) + i, 32);
    if(error != seL4_NoError) {
        return error;
    }

    *slot = simple_get_cap_count(simple) + i;
    return seL4_NoError;
}

void simple_vka_cspace_make_path(void *data, seL4_CPtr slot, cspacepath_t *path)
{
    assert(data && path);

    simple_t *simple = (simple_t *) data;

    path->capPtr = slot;
    path->capDepth = 32;
    path->root = simple_get_cnode(simple);
    path->dest = simple_get_cnode(simple);
    path->offset = slot;
    path->destDepth = 32;
}

void simple_make_vka(simple_t *simple, vka_t *vka)
{
    vka->data = simple;
    vka->cspace_alloc = &simple_vka_cspace_alloc;
    vka->cspace_make_path = &simple_vka_cspace_make_path;
    vka->utspace_alloc = &dummy_vka_utspace_alloc;
    vka->cspace_free = &dummy_vka_cspace_free;
    vka->utspace_free = &dummy_vka_utspace_free;
}

seL4_CPtr simple_last_valid_cap(simple_t *simple)
{
    seL4_CPtr largest = 0;
    int i;

    for (i = 0; i < simple_get_cap_count(simple); i++) {
        seL4_CPtr cap = simple_get_nth_cap(simple, i);
        if (cap > largest) largest = cap;
    }
    return largest;
}

/**
 * Request data to a region of physical memory.
 *
 * Note: This function will only return the mapped virtual address that it knows about. It does not do any mapping its self nor can it guess where mapping functions are going to map.
 *
 * @param data cookie for the underlying implementation
 * @param page aligned physical address for the frame
 * @param size of the region in bits
 * @param cap to the frame gets set. Will return the untyped cap unless the underlying implementation has access to the frame cap. Check with implementation but it should be a frame cap if and only if a vaddr is returned.
 * @param (potentially) the offset within the untyped cap that was returned
 * Returns the vritual address to which this physical address is mapped or NULL if frame is unmapped
 */
void *simple_get_frame_info(simple_t *simple, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *offset)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    return simple_default_get_frame_info(data, paddr, size_bits, frame_cap, offset);
}

/**
 * Get the cap to the physcial frame of memory and put it at specified location
 *
 * @param data cookie for the underlying implementation
 * @param page aligned physical address
 * @param size of the region in bits
 * @param The path to where to put this cap
 */
seL4_Error simple_get_frame_cap(simple_t *simple, void *paddr, int size_bits, cspacepath_t *path)
{
    assert(simple);

    void *data = simple->data;
    assert(data && paddr);

    seL4_CPtr frame_cap;
    seL4_Word offset;

    /* Lookup ComputeEnvType */
    switch ( *(int *)data ) {
        case 1:
            simple_default_get_frame_info(data, paddr, size_bits, &frame_cap, &offset);
            if (frame_cap == seL4_CapNull) {
                return seL4_FailedLookup;
            }
            return seL4_CNode_Copy(path->root, path->capPtr, path->capDepth, seL4_CapInitThreadCNode, frame_cap, seL4_WordBits, seL4_AllRights);
        case 2:
            return get_frame_cap(data, paddr, size_bits, path);
    }

}

/**
 * Request mapped address to a region of physical memory.
 *
 * Note: This function will only return the mapped virtual address that it knows about. It does not do any mapping its self nor can it guess where mapping functions are going to map.
 *
 * @param data cookie for the underlying implementation
 * @param page aligned physical address
 * @param size of the region in bits
 * Returns the vritual address to which this physical address is mapped or NULL if frame is unmapped
 */
void *simple_get_frame_vaddr(simple_t *simple, void *paddr, int size_bits)
{
    return NULL;
}

/**
 * Request a cap to a specific IRQ number on the system
 *.
 * @param data cookie for the underlying implementation
 * @param the CNode in which to put this cap
 * @param the index within the CNode to put cap
 * @param Depth of index
 */
seL4_Error simple_get_IRQ_control(simple_t *simple, int irq, cspacepath_t path)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    /* Lookup ComputeEnvType */
    switch ( *(int *)data ) {
        case 1:
            return seL4_IRQControl_Get(seL4_CapIRQControl, irq, path.root, path.capPtr, path.capDepth);
        case 2:
            return get_irq(data, irq, path.root, path.capPtr, path.capDepth);
    }
    return seL4_InvalidArgument;
}

/**
 * Assign the vpsace to the current threads ASID pool
 *
 * @param data cookie for the underlying implementation
 * @param vspace to assign
 */
seL4_Error simple_ASIDPool_assign(simple_t *simple, seL4_CPtr vspace)
{
    return seL4_ARCH_ASIDPool_Assign(seL4_CapInitThreadASIDPool, vspace);
}

seL4_CPtr simple_get_IOPort_cap(simple_t *simple, uint16_t start_port, uint16_t end_port)
{
    return seL4_CapIOPort;
}


#define INIT_CAP_BASE_RANGE (seL4_CapInitThreadASIDPool)

//the +1 in INIT_CAP_TOP_RANGE is for seL4_CapDomain
#if defined(CONFIG_ARCH_ARM)
    #if defined(CONFIG_IOMMU)
        #define INIT_CAP_TOP_RANGE (seL4_CapInitThreadIPCBuffer - seL4_CapIOPort + INIT_CAP_BASE_RANGE + 1)
    #else
        #define INIT_CAP_TOP_RANGE (seL4_CapInitThreadIPCBuffer - seL4_CapIOSpace + INIT_CAP_BASE_RANGE + 1)
    #endif
#elif defined(CONFIG_ARCH_IA32)
    #if defined(CONFIG_IOMMU)
        #define INIT_CAP_TOP_RANGE (seL4_CapInitThreadIPCBuffer - seL4_CapInitThreadASIDPool + INIT_CAP_BASE_RANGE + 1)
    #else
        #define INIT_CAP_TOP_RANGE (seL4_CapInitThreadIPCBuffer - seL4_CapInitThreadASIDPool - 1 + INIT_CAP_BASE_RANGE + 1)
    #endif
#endif
#define SHARED_FRAME_RANGE ((bi->sharedFrames.end - bi->sharedFrames.start) + INIT_CAP_TOP_RANGE)
#define USER_IMAGE_FRAMES_RANGE ((bi->userImageFrames.end - bi->userImageFrames.start) + SHARED_FRAME_RANGE)
#define USER_IMAGE_PTS_RANGE ((bi->userImagePTs.end - bi->userImagePTs.start) + USER_IMAGE_FRAMES_RANGE)
#define UNTYPED_RANGE ((bi->untyped.end - bi->untyped.start) + USER_IMAGE_PTS_RANGE)
#define DEVICE_RANGE (device_caps + UNTYPED_RANGE)


/**
 * Get the total number of caps this library can address
 *
 * @param data cookie for the underlying implementation
 */
int simple_get_cap_count(simple_t *simple)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    seL4_BootInfo * bi = (seL4_BootInfo *) data;

    int device_caps = 0;
    int i;
    for(i = 0; i < bi->numDeviceRegions; i++) {
        device_caps += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
    }

    return (device_caps)
           + (bi->sharedFrames.end - bi->sharedFrames.start)
           + (bi->userImageFrames.end - bi->userImageFrames.start)
           + (bi->userImagePTs.end - bi->userImagePTs.start)
           + (bi->untyped.end - bi->untyped.start)
           + INIT_CAP_TOP_RANGE; //Include all the init caps
}

/**
 * Get the nth cap that this library can address
 *
 * @param data cookie for the underlying implementation
 * @param the nth starting at 0
 */
seL4_CPtr simple_get_nth_cap(simple_t *simple, int n)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    seL4_BootInfo * bi = (seL4_BootInfo *) data;

    int i;
    int device_caps = 0;
    seL4_CPtr true_return = seL4_CapNull;

    for(i = 0; i < bi->numDeviceRegions; i++) {
        device_caps += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
    }
    if(n < INIT_CAP_BASE_RANGE) {
        true_return = (seL4_CPtr) n+1;
    } else if(n == INIT_CAP_TOP_RANGE - 1) {
        //seL4_CapDomain
        true_return = (seL4_CPtr)seL4_CapDomain;
    } else if(n < INIT_CAP_TOP_RANGE) {
        true_return = (seL4_CPtr) n+1;
        #if defined(CONFIG_ARCH_ARM)
            true_return++;
        #endif
        #ifndef CONFIG_IOMMU
            if(true_return >= seL4_CapIOSpace) {
                true_return++;
            }
        #endif
    } else if(n < SHARED_FRAME_RANGE) {
        return bi->sharedFrames.start + (n - INIT_CAP_TOP_RANGE);
    } else if(n < USER_IMAGE_FRAMES_RANGE) {
        return bi->userImageFrames.start + (n - SHARED_FRAME_RANGE);
    } else if(n < USER_IMAGE_PTS_RANGE) {
        return bi->userImagePTs.start + (n - USER_IMAGE_FRAMES_RANGE);
    } else if(n < UNTYPED_RANGE) {
        return bi->untyped.start + (n - USER_IMAGE_PTS_RANGE);
    } else if(n < DEVICE_RANGE) {
        i = 0;
        int current_count = 0;
        while((bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start) + current_count < (n - UNTYPED_RANGE)) {
            current_count += bi->deviceRegions[i].frames.end - bi->deviceRegions[i].frames.start;
            i++;
        }
        return bi->deviceRegions[i].frames.start + (n - UNTYPED_RANGE - current_count);
    }

    return true_return;
}

seL4_CPtr simple_get_cnode(simple_t *simple)
{
    return (seL4_CPtr)seL4_CapInitThreadCNode;
}

/**
 * Get the size of the threads cnode in bits
 *
 * @param data for the underlying implementation
 */
int simple_get_cnode_size_bits(simple_t *simple)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    return ((seL4_BootInfo *)data)->initThreadCNodeSizeBits;
}

seL4_CPtr simple_get_tcb(simple_t *simple)
{
    return (seL4_CPtr)seL4_CapInitThreadTCB;
}

seL4_CPtr simple_get_pd(simple_t *simple) {
    return (seL4_CPtr)seL4_CapInitThreadPD;
}

seL4_CPtr simple_get_irq_ctrl(simple_t *simple) {
    return (seL4_CPtr)seL4_CapIRQControl;
}

/**
 * Get the cap to init caps with numbering based on bootinfo.h
 *
 * @param data for the underlying implementation
 * @param the value of the enum matching in bootinfo.h
 */
seL4_CPtr simple_get_init_cap(simple_t *simple, seL4_CPtr cap) {
    return cap;
}

/**
 * Get the amount of untyped caps availible
 *
 * @param data for the underlying implementation
 *
 */
int simple_get_untyped_count(simple_t *simple)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    return ((seL4_BootInfo *)data)->untyped.end - ((seL4_BootInfo *)data)->untyped.start;
}

/**
 * Get the nth untyped cap that this library can address
 *
 * @param data cookie for the underlying implementation
 * @param the nth starting at 0
 * @param the size of the untyped for the returned cap
 * @param the physical address of the returned cap
 */
seL4_CPtr simple_get_nth_untyped(simple_t *simple, int n, uint32_t *size_bits, uint32_t *paddr)
{
    assert(simple);

    void *data = simple->data;
    assert(data && size_bits && paddr);

    seL4_BootInfo *bi = (seL4_BootInfo *)data;

    if (n < (bi->untyped.end - bi->untyped.start)) {
        if (paddr != NULL) {
            *paddr = bi->untypedPaddrList[n];
        }
        if (size_bits != NULL) {
            *size_bits = bi->untypedSizeBitsList[n];
        }
        return bi->untyped.start + (n);
    }

    return seL4_CapNull;
}

/**
 * Get the amount of user image caps availible
 *
 * @param data for the underlying implementation
 *
 */
int simple_get_userimage_count(simple_t *simple)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    return ((seL4_BootInfo *)data)->userImageFrames.end - ((seL4_BootInfo *)data)->userImageFrames.start;
}

/**
 * Get the nth untyped cap that this library can address
 *
 * @param data cookie for the underlying implementation
 * @param the nth starting at 0
 *
 */
seL4_CPtr simple_get_nth_userimage(simple_t *simple, int n)
{
    assert(simple);

    void *data = simple->data;
    assert(data);

    seL4_BootInfo *bi = (seL4_BootInfo *)data;

    if (n < (bi->userImageFrames.end - bi->userImageFrames.start)) {
        return bi->userImageFrames.start + (n);
    }

    return seL4_CapNull;
}

#ifdef CONFIG_IOMMU
/**
 * Get the IO space capability for the specified pci device and domain ID
 *
 * @param data cookie for the underlying implementation
 * @param domainID domain ID to request
 * @param deviceID PCI device ID
 * @param path Path to where to put this cap
 *
 */
seL4_CPtr simple_get_iospace(simple_t *simple, uint16_t domainID, uint16_t deviceID, cspacepath_t *path)
{
    return seL4_CNode_Mint(path->root, path->capPtr, path->capDepth, seL4_CapInitThreadCNode, seL4_CapIOSpace,
                           32, seL4_AllRights, (seL4_CapData_t){.words = {((uint32_t)domainID << 16) | (uint32_t)deviceID}});
}
#endif


/**
 *
 * Get simple to print all the information it has about its environment
 */
void simple_print(simple_t *simple) {
    assert(simple);

    void *data = simple->data;
    assert(data);

    seL4_BootInfo *info = (seL4_BootInfo *)data;

    /* Parse boot info from kernel. */
    printf("Node ID: %d (of %d)\n",info->nodeID, info->numNodes);
    printf("initThreadCNode size: %d slots\n", (1 << info->initThreadCNodeSizeBits) );

    printf("\n--- Capability Details ---\n");
    printf("Type              Start      End\n");
    printf("Empty             0x%08x 0x%08x\n", info->empty.start, info->empty.end);
    printf("Shared frames     0x%08x 0x%08x\n", info->sharedFrames.start, info->sharedFrames.end);
    printf("User image frames 0x%08x 0x%08x\n", info->userImageFrames.start,
            info->userImageFrames.end);
    printf("User image PTs    0x%08x 0x%08x\n", info->userImagePTs.start, info->userImagePTs.end);
    printf("Untypeds          0x%08x 0x%08x\n", info->untyped.start, info->untyped.end);

    printf("\n--- Untyped Details ---\n");
    printf("Untyped Slot       Paddr      Bits\n");
    for (int i = 0; i < info->untyped.end-info->untyped.start; i++) {
        printf("%3d     0x%08x 0x%08x %d\n", i, info->untyped.start+i, info->untypedPaddrList[i],
                info->untypedSizeBitsList[i]);
    }

    printf("\n--- Device Regions: %d ---\n", info->numDeviceRegions);
    printf("Device Addr     Size Start      End\n");
    for (int i = 0; i < info->numDeviceRegions; i++) {
        printf("%2d 0x%08x %d 0x%08x 0x%08x\n", i,
                                                info->deviceRegions[i].basePaddr,
                                                info->deviceRegions[i].frameSizeBits,
                                                info->deviceRegions[i].frames.start,
                                                info->deviceRegions[i].frames.end);
    }
}

static void *simple_default_get_frame_info(void *data, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *offset)
{
    assert(data && paddr && frame_cap);

    int i;
    seL4_BootInfo *bi = (seL4_BootInfo *)data;
    seL4_DeviceRegion *region;

    *offset = 0;
    *frame_cap = seL4_CapNull;
    region = bi->deviceRegions;
    for (i = 0; i < bi->numDeviceRegions; i++, region++) {

        seL4_Word region_start = region->basePaddr;
        seL4_Word n_caps = region->frames.end - region->frames.start;
        seL4_Word region_end = region_start + (n_caps << region->frameSizeBits);

        if (region_start <= (seL4_Word) paddr && (seL4_Word) paddr < region_end && region->frameSizeBits == size_bits) {
            *frame_cap =  region->frames.start + (((seL4_Word) paddr - region->basePaddr) >> region->frameSizeBits);
            i = bi->numDeviceRegions;
        }
    }

    return NULL;
}

/* basic simple functions */
#ifdef CONFIG_ARCH_ARM
static seL4_Error get_frame_cap(void *data, void *paddr, int size_bits, cspacepath_t *path)
{
    compute_env_data_t *init = (compute_env_data_t *) data;

    //assert(paddr == (void*) DEFAULT_TIMER_PADDR);
    assert(size_bits == seL4_PageBits);


    int error = seL4_CNode_Copy(path->root, path->capPtr, path->capDepth, init->root_cnode,
                                init->timer_frame, seL4_WordBits, seL4_AllRights);
    assert(error == seL4_NoError);

    return error;
}
#endif /* CONFIG_ARCH_ARM */

#ifdef CONFIG_ARCH_IA32
/**
 * Request a cap to the IOPorts on IA32
 *.
 * @param data cookie for the underlying implementation
 * @param start port number that a cap is needed to
 * @param end port number that a cap is needed to
 */
static seL4_CPtr get_IOPort_cap(void *data, uint16_t start_port, uint16_t end_port)
{
    compute_env_data_t *init = (compute_env_data_t *) data;
    //assert(start_port >= PIT_IO_PORT_MIN);
    //assert(end_port <= PIT_IO_PORT_MAX);

    return init->io_port;
}
#endif /* CONFIG_ARCH_IA32 */

static seL4_Error get_irq(void *data, int irq, seL4_CNode root, seL4_Word index, uint8_t depth)
{
    compute_env_data_t *init = (compute_env_data_t *) data;
    //assert(irq == DEFAULT_TIMER_INTERRUPT);

    int error = seL4_CNode_Copy(root, index, depth, init->root_cnode,
                                init->timer_irq, seL4_WordBits, seL4_AllRights);
    assert(error == 0);
    return error;
}


void simple_init_bootinfo(simple_t *simple, seL4_BootInfo *bi)
{
    assert(simple);
    assert(bi);

    bi->ComputeEnvType = kernelBOOTINFO;
    simple->data = bi;
}
