/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef _INTERFACE_SIMPLE_H_
#define _INTERFACE_SIMPLE_H_

#include <autoconf.h>

#include <assert.h>
#include <sel4/sel4.h>

#include <vka/cspacepath_t.h>


/**
 * Get the cap to the physcial frame of memory and put it at specified location
 *
 * @param data cookie for the underlying implementation
 *
 * @param page aligned physical address
 *
 * @param size of the region in bits
 *
 * @param The path to where to put this cap
 */
typedef seL4_Error (*simple_get_frame_cap_fn)(void *data, void *paddr, int size_bits, cspacepath_t *path);

/**
 * Request mapped address to a region of physical memory.
 *
 * Note: This function will only return the mapped virtual address that it knows about. It does not do any mapping its self nor can it guess where mapping functions are going to map.
 *
 * @param data cookie for the underlying implementation
 *
 * @param page aligned physical address
 *
 * @param size of the region in bits
 *
 * Returns the vritual address to which this physical address is mapped or NULL if frame is unmapped
 */

typedef void *(*simple_get_frame_mapping_fn)(void *data, void *paddr, int size_bits);

/**
 * Request data to a region of physical memory.
 *
 * Note: This function will only return the mapped virtual address that it knows about. It does not do any mapping its self nor can it guess where mapping functions are going to map.
 *
 * @param data cookie for the underlying implementation
 *
 * @param page aligned physical address for the frame
 *
 * @param size of the region in bits
 *
 * @param cap to the frame gets set. Will return the untyped cap unless the underlying implementation has access to the frame cap. Check with implementation but it should be a frame cap if and only if a vaddr is returned.
 *
 * @param (potentially) the offset within the untyped cap that was returned
 * Returns the vritual address to which this physical address is mapped or NULL if frame is unmapped
 */
typedef void *(*simple_get_frame_info_fn)(void *data, void *paddr, int size_bits, seL4_CPtr *cap, seL4_Word *ut_offset);

/**
 * Request a cap to a specific IRQ number on the system
 *.
 * @param data cookie for the underlying implementation
 *
 * @param the CNode in which to put this cap
 *
 * @param the index within the CNode to put cap
 *
 * @param Depth of index
 */
typedef seL4_Error (*simple_get_IRQ_control_fn)(void *data, int irq, seL4_CNode cnode, seL4_Word index, uint8_t depth);


/**
 * Request a cap to the IOPorts on IA32
 *.
 * @param data cookie for the underlying implementation
 *
 * @param start port number that a cap is needed to
 *
 * @param end port number that a cap is needed to
 */
typedef seL4_CPtr (*simple_get_IOPort_cap_fn)(void *data, uint16_t start_port, uint16_t end_port);

/**
 * Assign the vpsace to the current threads ASID pool
 *
 * @param data cookie for the underlying implementation
 *
 * @param vspace to assign
*/
typedef seL4_Error (*simple_ASIDPool_assign_fn)(void *data, seL4_CPtr vspace);

/**
 * Get the total number of caps this library can address
 *
 * @param data cookie for the underlying implementation
*/
typedef int (*simple_get_cap_count_fn)(void *data);

/**
 * Get the nth cap that this library can address
 *
 * @param data cookie for the underlying implementation
 *
 * @param the nth starting at 0
*/
typedef seL4_CPtr (*simple_get_nth_cap_fn)(void *data, int n);


/**
 * Get the cap to init caps with numbering based on bootinfo.h
 *
 * @param data for the underlying implementation
 *
 * @param the value of the enum matching in bootinfo.h
*/

typedef seL4_CPtr (*simple_get_init_cap_fn)(void *data, seL4_CPtr cap);

/**
 * Get the size of the threads cnode in bits
 *
 * @param data for the underlying implementation
*/

typedef uint8_t  (*simple_get_cnode_size_fn)(void *data);

/**
 * Get the amount of untyped caps availible
 *
 * @param data for the underlying implementation
 *
*/

typedef int (*simple_get_untyped_count_fn)(void *data);

/**
 * Get the nth untyped cap that this library can address
 *
 * @param data cookie for the underlying implementation
 *
 * @param the nth starting at 0
 *
 * @param the size of the untyped for the returned cap
 *
 * @param the physical address of the returned cap
*/

typedef seL4_CPtr (*simple_get_nth_untyped_fn)(void *data, int n, uint32_t *size_bits, uint32_t *paddr);

/**
 * Get the amount of user image caps availible
 *
 * @param data for the underlying implementation
 *
*/

typedef int (*simple_get_userimage_count_fn)(void *data);

/**
 * Get the nth untyped cap that this library can address
 *
 * @param data cookie for the underlying implementation
 *
 * @param the nth starting at 0
 *
*/

typedef seL4_CPtr (*simple_get_nth_userimage_fn)(void *data, int n);

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
typedef seL4_Error (*simple_get_iospace_fn)(void *data, uint16_t domainID, uint16_t deviceID, cspacepath_t *path);
#endif

/**
 *
 * Get simple to print all the information it has about its environment
 */
typedef void (*simple_print_fn)(void *data);

typedef struct simple_t {
    void *data;
    simple_get_frame_cap_fn frame_cap;
    simple_get_frame_mapping_fn frame_mapping;
    simple_get_frame_info_fn frame_info;
    simple_get_IRQ_control_fn irq;
    simple_ASIDPool_assign_fn ASID_assign;
    simple_get_IOPort_cap_fn IOPort_cap;
    simple_get_cap_count_fn cap_count;
    simple_get_nth_cap_fn nth_cap;
    simple_get_init_cap_fn init_cap;
    simple_get_cnode_size_fn cnode_size;
    simple_get_untyped_count_fn untyped_count;
    simple_get_nth_untyped_fn nth_untyped;
    simple_get_userimage_count_fn userimage_count;
    simple_get_nth_userimage_fn nth_userimage;
#ifdef CONFIG_IOMMU
    simple_get_iospace_fn iospace;
#endif
    simple_print_fn print;
} simple_t;


void *simple_get_frame_info(simple_t *simple, void *paddr, int size_bits, seL4_CPtr *frame_cap, seL4_Word *offset);
seL4_Error simple_get_frame_cap(simple_t *simple, void *paddr, int size_bits, cspacepath_t *path);
void *simple_get_frame_vaddr(simple_t *simple, void *paddr, int size_bits);
seL4_Error simple_get_IRQ_control(simple_t *simple, int irq, cspacepath_t path);
seL4_Error simple_ASIDPool_assign(simple_t *simple, seL4_CPtr vspace);
seL4_CPtr simple_get_IOPort_cap(simple_t *simple, uint16_t start_port, uint16_t end_port);
int simple_get_cap_count(simple_t *simple);
seL4_CPtr simple_get_nth_cap(simple_t *simple, int n);
seL4_CPtr simple_get_cnode(simple_t *simple);
int simple_get_cnode_size_bits(simple_t *simple);
seL4_CPtr simple_get_tcb(simple_t *simple);
seL4_CPtr simple_get_pd(simple_t *simple);
seL4_CPtr simple_get_irq_ctrl(simple_t *simple);
seL4_CPtr simple_get_init_cap(simple_t *simple, seL4_CPtr cap);
int simple_get_untyped_count(simple_t *simple);
seL4_CPtr simple_get_nth_untyped(simple_t *simple, int n, uint32_t *size_bits, uint32_t *paddr);
int simple_get_userimage_count(simple_t *simple);
seL4_CPtr simple_get_nth_userimage(simple_t *simple, int n);

#ifdef CONFIG_IOMMU
seL4_CPtr simple_get_iospace(simple_t *simple, uint16_t domainID, uint16_t deviceID, cspacepath_t *path);
#endif

void simple_print(simple_t *simple);

void simple_init_bootinfo(simple_t *simple, seL4_BootInfo *bi);


#endif /* _INTERFACE_SIMPLE_H_ */
