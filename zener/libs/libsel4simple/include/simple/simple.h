/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

#ifndef _INTERFACE_SIMPLE_H_
#define _INTERFACE_SIMPLE_H_

#include <autoconf.h>

#include <assert.h>
#include <sel4/sel4.h>
#include <vka/cspacepath_t.h>

//value of uint32_t ComputeEnvType
enum COMPUTE_ENV_TYPE {
    kernelBOOTINFO = 1,
    LocalhostCOMPUTE_ENV = 2
};

typedef struct simple_t {
    void *data;
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
