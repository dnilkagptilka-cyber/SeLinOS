# SeLinOS x86_64 module W^X research — 2026-08-13

The official seL4 mapping tutorial states that user-level owns virtual-memory management beyond kernel paging primitives, and that a page mapping receives a rights argument such as read-only `seL4_CanRead`; it also describes virtual-memory attributes as architecture-dependent cache attributes.[1]

In the pinned SeLinOS x86_64 generated interface, `seL4_X86_Page_Map` accepts `seL4_CapRights_t` and `seL4_X86_VMAttributes`. The generated `seL4_CapRights_new` constructor exposes grant-reply, grant, read and write fields. The pinned x86 VM-attribute enum contains only default/cache values (`WriteBack`, `WriteThrough`, `CacheDisabled`, `Uncacheable`, `WriteCombining`), not an execute-disable/NX choice. Therefore SeLinOS can demonstrate write removal via read-only mappings, but the current API profile does not by itself provide an independently testable NX mapping for writable data.

| Finding | Consequence for native `.ko` execution |
|---|---|
| Root/user-level manages VSpace mappings | A dedicated module domain is architecturally feasible using a fresh process/VSpace/CSpace. |
| Page mapping rights expose read/write rather than execute | A writable-to-read-only transition is possible, but not sufficient to prove W^X. |
| x86 VM attributes are cache-related in pinned source | No direct NX policy knob exists in this profile. |
| SeLinOS requires reproducible security boundaries | Module code execution remains disabled until an enforceable W^X or explicitly justified alternative mechanism is implemented and tested. |

> This finding does not diminish existing KABI M2 evidence. It sets a strict blocker on progressing from non-executing module preparation to executable native module loading.

## References

[1] [seL4 Mapping tutorial](https://docs.sel4.systems/Tutorials/mapping.html).

[2] [Pinned SeLinOS x86 `seL4_X86_VMAttributes` definition](../src/kernel/libsel4/arch_include/x86/sel4/arch/types.h).

[3] [Pinned generated `seL4_CapRights_new` definition](../build/libsel4/include/sel4/shared_types_gen.h).
