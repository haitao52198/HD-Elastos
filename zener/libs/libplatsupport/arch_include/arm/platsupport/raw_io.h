/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

/*
Despite the popularity of I/O ports in the x86 world, the main mechanism used to communicate
with devices is through memory-mapped registers and device memory. Both are called I/O memory
because the difference between registers and memory is transparent to software.

I/O memory is simply a region of RAM-like locations that the device makes available to the
processor over the bus. This memory can be used for a number of purposes, such as holding video
data or Ethernet packets, as well as implementing device registers that behave just like I/O
ports (i.e., they have side effects associated with reading and writing them).

The way to access I/O memory depends on the computer architecture, bus, and device being used,
although the principles are the same everywhere.
*/

#ifndef __RAW_IO_H__
#define __RAW_IO_H__


inline unsigned char rawReadUInt8(unsigned int ptr)
{
	return *((volatile unsigned char *)ptr);
}

inline unsigned int rawReadUInt32(unsigned int ptr)
{
	return *((volatile unsigned int *)ptr);
}

inline void rawWriteUInt8(unsigned int ptr, unsigned char value)
{
	*((volatile unsigned char *)ptr) = value;
}

inline void rawWriteUInt32(unsigned int ptr, unsigned int value)
{
	*((volatile unsigned int *)ptr) = value;
}

#endif