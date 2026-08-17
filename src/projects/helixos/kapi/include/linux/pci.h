/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/device.h>

#define PCI_ANY_ID 0xffffu
#define PCI_DEVICE(vendor_id, device_id) \
    .vendor = (vendor_id), .device = (device_id), .subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID

struct pci_device_id {
    u32 vendor;
    u32 device;
    u32 subvendor;
    u32 subdevice;
    u32 class;
    u32 class_mask;
    ulong driver_data;
};

struct pci_dev {
    struct device dev;
    u16 vendor;
    u16 device;
    u8 irq;
    u8 _reserved[3];
    u64 resource_start[6];
    u64 resource_len[6];
};

struct pci_driver {
    const char *name;
    const struct pci_device_id *id_table;
    int (*probe)(struct pci_dev *pdev, const struct pci_device_id *id);
    void (*remove)(struct pci_dev *pdev);
};

int pci_register_driver(struct pci_driver *driver);
void pci_unregister_driver(struct pci_driver *driver);
int pci_enable_device(struct pci_dev *pdev);
void pci_disable_device(struct pci_dev *pdev);
int pci_set_master(struct pci_dev *pdev);
unsigned long pci_resource_start(const struct pci_dev *pdev, unsigned int bar);
unsigned long pci_resource_len(const struct pci_dev *pdev, unsigned int bar);
