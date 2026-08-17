// SPDX-License-Identifier: GPL-2.0
/*
 * Intentionally small external module fixture for SELINOS_LINUX_BASELINE_6_18_44.
 * It is compiled against the pinned Linux reference build but is never loaded
 * into that kernel; SeLinOS consumes it as a KABI parser/verifier test artifact.
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init selinos_kabi_probe_init(void)
{
    pr_info("selinos_kabi_probe: Linux 6.18.44 KABI fixture initialized\n");
    return 0;
}

static void __exit selinos_kabi_probe_exit(void)
{
    pr_info("selinos_kabi_probe: Linux 6.18.44 KABI fixture exited\n");
}

module_init(selinos_kabi_probe_init);
module_exit(selinos_kabi_probe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SeLinOS project");
MODULE_DESCRIPTION("Version-pinned Linux 6.18.44 KABI parser fixture");
