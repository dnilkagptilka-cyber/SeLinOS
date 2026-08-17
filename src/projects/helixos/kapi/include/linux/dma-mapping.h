/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/device.h>

int dma_set_mask_and_coherent(struct device *dev, u64 mask);
void *dma_alloc_coherent(struct device *dev, size_t size, dma_addr_t *dma_handle,
                         gfp_t flags);
void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr,
                       dma_addr_t dma_handle);
