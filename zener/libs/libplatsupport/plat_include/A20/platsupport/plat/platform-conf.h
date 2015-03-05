
/*
 * Copyright 2015, Tongji Operating System Group & elastos.org
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 */

/*
   platform wide configure parameter, OS independent
*/

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__

#define CLK_FREQ (1 * 1024 * 1024 * 1024)

#define TIMER_INTERVAL_TICKS(ns) ((uint32_t)(1ULL * (ns) * CLK_FREQ / 1000 / 1000 / 1000))
#define TIMER_INTERVAL_NS(ticks) ((uint32_t)(1ULL * (ticks) * 1000 * 1000 * 1000 / CLK_FREQ))

#define TIMER_CLK_INTERVAL_TICKS(clk,ns) ((uint32_t)(1ULL * (ns) * clk / 1000 / 1000 / 1000))
#define TIMER_CLK_INTERVAL_NS(clk,ticks) ((uint32_t)(1ULL * (ticks) * 1000 * 1000 * 1000 / clk))

#endif
