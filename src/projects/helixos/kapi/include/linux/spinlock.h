/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

typedef struct {
    unsigned int locked;
} spinlock_t;

static inline void spin_lock_init(spinlock_t *lock)
{
    if (lock != NULL) {
        lock->locked = 0u;
    }
}

/*
 * Phase 6 lock calls are single-driver-domain bookkeeping only. They do not
 * mask hardware interrupts, disable preemption or provide SMP exclusion.
 */
unsigned long spin_lock_irqsave(spinlock_t *lock);
void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);
