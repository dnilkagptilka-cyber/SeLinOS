/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/device.h>
#include <linux/interrupt.h>

/*
 * Private SeLinOS loader/runtime interface. It is not part of the frozen Linux
 * KAPI surface and is called by the domain/loader that hosts a Linux-shaped
 * driver. The values identify only capabilities already granted to that
 * domain; this interface cannot manufacture hardware authority.
 */
int selinos_kapi_bind_irq(unsigned int irq, unsigned long notification_cap,
                          unsigned long completion_endpoint,
                          unsigned long request_endpoint,
                          unsigned long register_control,
                          unsigned long unregister_control);
int selinos_kapi_dispatch_one_irq(void);

/* Binds the capability that authorises only profile-approved PCI ID matching.
 * It is invoked by a loader/domain after that capability was copied into the
 * driver CSpace; it cannot create PCI config-space authority. */
int selinos_kapi_bind_deviced(unsigned long request_endpoint);

/* A NULL `dev` records a deferred association: `pci_register_driver()` binds
 * the single pre-granted lease to its newly materialised shadow `pci_dev` just
 * before it enters the driver's local probe callback. */
int selinos_kapi_bind_dma_lease(struct device *dev, unsigned long cpu_address,
                                dma_addr_t dma_address, size_t size, u64 dma_mask,
                                unsigned long free_endpoint);
