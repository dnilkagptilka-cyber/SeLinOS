// SPDX-License-Identifier: MIT
#ifndef SELINOS_BOOTSTRAP_H
#define SELINOS_BOOTSTRAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * The bootstrap root initially owns seL4 capabilities. The root task must
 * eventually transfer only the capabilities listed in a domain policy and
 * dispose of the rest. This declaration separates policy from the later
 * object-allocation implementation.
 */
typedef enum selinos_domain_kind {
    SELINOS_DOMAIN_ROOT = 0,
    SELINOS_DOMAIN_OBJECTD,
    SELINOS_DOMAIN_TASKD,
    SELINOS_DOMAIN_MEMD,
    SELINOS_DOMAIN_CONSOLED,
    SELINOS_DOMAIN_ABI_GATE,
    SELINOS_DOMAIN_APPLICATION,
} selinos_domain_kind_t;

typedef struct selinos_domain_policy {
    selinos_domain_kind_t kind;
    const char *name;
    bool receives_tcb_control;
    bool receives_vspace_control;
    bool receives_untyped_memory;
    bool receives_irq_control;
    bool receives_device_memory;
} selinos_domain_policy_t;

/** Prepare static policy metadata from trusted root-task state. */
bool selinos_bootstrap_prepare(void);

/**
 * Verify invariants that must be true before capability delegation begins.
 * This does not create kernel objects; that is a later objectd milestone.
 */
bool selinos_capability_policy_selftest(void);

/**
 * Bootstrap three independent child domains from the embedded CPIO image.
 * Each child is created by seL4utils with its own TCB, CSpace, VSpace and
 * fault endpoint. M0 retains root authority; capability minimisation is the
 * following objectd/taskd/memd implementation step.
 */
bool selinos_domain_manager_start(void);

/** Handle exactly one isolated Phase 35 root-dispatch status request. */
bool selinos_root_dispatch_m0_once(void);

/** Enter the opt-in Phase 35 M1 loop that blocks only on its construction endpoint. */
void selinos_root_construct_m1_dispatch_loop(void);

/** Return the immutable policy for a bootstrap server kind. */
const selinos_domain_policy_t *selinos_policy_for(selinos_domain_kind_t kind);

/** Obtain the number of policy entries. */
size_t selinos_policy_count(void);

#endif
