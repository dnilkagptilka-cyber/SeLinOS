/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

struct work_struct {
    void (*func)(struct work_struct *work);
    unsigned int pending;
};

#define INIT_WORK(work, callback)            \
    do {                                      \
        (work)->func = (callback);            \
        (work)->pending = 0u;                 \
    } while (0)

/*
 * Phase 6 executes accepted work synchronously in the calling driver domain.
 * A non-zero result means the callback ran exactly once; this shim does not
 * provide a kernel worker thread, cross-domain queue or deferred execution.
 */
bool schedule_work(struct work_struct *work);
bool cancel_work_sync(struct work_struct *work);
