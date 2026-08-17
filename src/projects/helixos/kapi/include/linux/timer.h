/* SPDX-License-Identifier: MIT */
#pragma once

#include <linux/types.h>

struct timer_list {
    void (*function)(struct timer_list *timer);
    unsigned long expires;
    unsigned int pending;
};

#define timer_setup(timer, callback, flags)   \
    do {                                      \
        (void)(flags);                        \
        (timer)->function = (callback);       \
        (timer)->expires = 0ul;               \
        (timer)->pending = 0u;                \
    } while (0)

/*
 * Phase 6 records only a single pending timer per object. No wall-clock,
 * tick, callback dispatch or cross-domain scheduling exists yet.
 */
int mod_timer(struct timer_list *timer, unsigned long expires);
int del_timer_sync(struct timer_list *timer);
