/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

typedef enum {
    IRQ_NONE = 0,
    IRQ_HANDLED = 1,
} irqreturn_t;

typedef irqreturn_t (*irq_handler_t)(int irq, void *dev_id);

#define IRQF_SHARED 0x00000080ul

int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
                const char *name, void *dev_id);
void free_irq(unsigned int irq, void *dev_id);
void disable_irq(unsigned int irq);
void enable_irq(unsigned int irq);
