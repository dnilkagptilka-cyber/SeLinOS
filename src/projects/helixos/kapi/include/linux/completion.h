/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

/*
 * Bounded single-domain completion shim. `done` is consumed by one successful
 * wait. `wait_for_completion_timeout()` is non-blocking in Phase 6 and returns
 * 1 only when a prior `complete()` is available; it otherwise returns 0.
 */
struct completion {
    unsigned int done;
};

static inline void init_completion(struct completion *completion)
{
    if (completion != NULL) {
        completion->done = 0u;
    }
}

void complete(struct completion *completion);
unsigned long wait_for_completion_timeout(struct completion *completion,
                                          unsigned long timeout);
