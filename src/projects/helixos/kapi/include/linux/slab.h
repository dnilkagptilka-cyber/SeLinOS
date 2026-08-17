/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

void *kmalloc(size_t size, gfp_t flags);
void *kzalloc(size_t size, gfp_t flags);
void kfree(const void *ptr);
