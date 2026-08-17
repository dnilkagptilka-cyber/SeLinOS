/* SPDX-License-Identifier: MIT */
#pragma once

/* This profile is freestanding: it must not import host libc headers. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned long size_t;
typedef unsigned long ulong;
typedef unsigned long dma_addr_t;
typedef unsigned int gfp_t;
typedef _Bool bool;

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

enum {
    GFP_KERNEL = 0x00u,
    GFP_ATOMIC = 0x20u,
    GFP_DMA = 0x01u,
};
