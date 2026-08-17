// SPDX-License-Identifier: MIT
#include "selinos_bootstrap.h"

/*
 * Capabilities are not yet delegated in M0. This policy is nevertheless
 * compiled into the root task so that later code cannot silently widen a
 * bootstrap server's authority without changing and reviewing this table.
 */
static const selinos_domain_policy_t policies[] = {
    {
        .kind = SELINOS_DOMAIN_ROOT,
        .name = "rootd",
        .receives_tcb_control = true,
        .receives_vspace_control = true,
        .receives_untyped_memory = true,
        .receives_irq_control = true,
        .receives_device_memory = true,
    },
    {
        .kind = SELINOS_DOMAIN_OBJECTD,
        .name = "objectd",
        .receives_tcb_control = true,
        .receives_vspace_control = false,
        .receives_untyped_memory = true,
        .receives_irq_control = false,
        .receives_device_memory = false,
    },
    {
        .kind = SELINOS_DOMAIN_TASKD,
        .name = "taskd",
        .receives_tcb_control = true,
        .receives_vspace_control = false,
        .receives_untyped_memory = false,
        .receives_irq_control = false,
        .receives_device_memory = false,
    },
    {
        .kind = SELINOS_DOMAIN_MEMD,
        .name = "memd",
        .receives_tcb_control = false,
        .receives_vspace_control = true,
        .receives_untyped_memory = false,
        .receives_irq_control = false,
        .receives_device_memory = false,
    },
    {
        .kind = SELINOS_DOMAIN_CONSOLED,
        .name = "consoled",
        .receives_tcb_control = false,
        .receives_vspace_control = false,
        .receives_untyped_memory = false,
        .receives_irq_control = false,
        .receives_device_memory = false,
    },
    {
        .kind = SELINOS_DOMAIN_ABI_GATE,
        .name = "abi-gated",
        .receives_tcb_control = false,
        .receives_vspace_control = false,
        .receives_untyped_memory = false,
        .receives_irq_control = false,
        .receives_device_memory = false,
    },
    {
        .kind = SELINOS_DOMAIN_APPLICATION,
        .name = "application",
        .receives_tcb_control = false,
        .receives_vspace_control = false,
        .receives_untyped_memory = false,
        .receives_irq_control = false,
        .receives_device_memory = false,
    },
};

size_t selinos_policy_count(void)
{
    return sizeof(policies) / sizeof(policies[0]);
}

const selinos_domain_policy_t *selinos_policy_for(selinos_domain_kind_t kind)
{
    for (size_t index = 0; index < selinos_policy_count(); ++index) {
        if (policies[index].kind == kind) {
            return &policies[index];
        }
    }
    return NULL;
}

bool selinos_capability_policy_selftest(void)
{
    const selinos_domain_policy_t *root = selinos_policy_for(SELINOS_DOMAIN_ROOT);
    const selinos_domain_policy_t *objectd = selinos_policy_for(SELINOS_DOMAIN_OBJECTD);
    const selinos_domain_policy_t *taskd = selinos_policy_for(SELINOS_DOMAIN_TASKD);
    const selinos_domain_policy_t *memd = selinos_policy_for(SELINOS_DOMAIN_MEMD);
    const selinos_domain_policy_t *console = selinos_policy_for(SELINOS_DOMAIN_CONSOLED);
    const selinos_domain_policy_t *abi_gate = selinos_policy_for(SELINOS_DOMAIN_ABI_GATE);
    const selinos_domain_policy_t *application = selinos_policy_for(SELINOS_DOMAIN_APPLICATION);

    if (root == NULL || objectd == NULL || taskd == NULL || memd == NULL ||
        console == NULL || abi_gate == NULL || application == NULL) {
        return false;
    }

    /* Only root may start with physical-device authority. */
    if (!root->receives_irq_control || !root->receives_device_memory) {
        return false;
    }
    if (objectd->receives_irq_control || objectd->receives_device_memory ||
        taskd->receives_irq_control || taskd->receives_device_memory ||
        memd->receives_irq_control || memd->receives_device_memory ||
        console->receives_irq_control || console->receives_device_memory ||
        abi_gate->receives_irq_control || abi_gate->receives_device_memory ||
        application->receives_irq_control || application->receives_device_memory) {
        return false;
    }

    /* Applications and the ABI gateway must not receive object-creation power. */
    if (abi_gate->receives_untyped_memory || application->receives_untyped_memory ||
        abi_gate->receives_tcb_control || application->receives_tcb_control ||
        abi_gate->receives_vspace_control || application->receives_vspace_control) {
        return false;
    }

    return true;
}
