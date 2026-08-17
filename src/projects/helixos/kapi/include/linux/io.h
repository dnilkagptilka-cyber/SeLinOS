/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

/*
 * SeLinOS KAPI M2 provisional no-authority surface. `ioremap()` never returns
 * a device mapping in the current profile: it rejects every request by
 * returning NULL. `iounmap()` accepts only the resulting NULL value as a
 * no-op. These declarations intentionally do not provide MMIO authority.
 */
void *ioremap(unsigned long physical_address, size_t size);
void iounmap(void *address);
