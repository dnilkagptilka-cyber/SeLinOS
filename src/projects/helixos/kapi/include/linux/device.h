/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

/*
 * KAPI profile 6.18.44/M0. The owner domain keeps the authoritative object;
 * this structure is a driver-visible projection, not a global kernel object.
 */
struct device {
    const char *init_name;
    u64 selinos_device_token;
    u32 selinos_domain_id;
    u32 selinos_flags;
};

const char *dev_name(const struct device *dev);
int dev_err(const struct device *dev, const char *message);
int dev_info(const struct device *dev, const char *message);
