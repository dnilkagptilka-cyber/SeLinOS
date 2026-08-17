# SeLinOS Phase 23 `uname` ABI M14 gate

**Status:** implementation contract approved; no runtime evidence exists yet.

M14 uses the x86_64 Linux `struct utsname` byte layout: six contiguous arrays of `65` bytes, for a total of `390` bytes. Root may copy this entire record only to one validated, pre-existing anonymous mapped page owned by the isolated ABI probe. Every field is NUL-terminated and all unused bytes are zero.

| Offset | Field | Pinned M14 value |
|---:|---|---|
| 0 | `sysname[65]` | `Linux` |
| 65 | `nodename[65]` | `selinos` |
| 130 | `release[65]` | `6.18.44-selinos` |
| 195 | `version[65]` | `SeLinOS ABI M14` |
| 260 | `machine[65]` | `x86_64` |
| 325 | `domainname[65]` | `localdomain` |

> **Scope boundary:** M14 emulates one pinned return record for a bounded x86_64 syscall probe. It does not expose a mutable hostname, UTS namespace, host/QEMU identity, a general Linux kernel personality, dynamic loader, package runtime, `dpkg` or `apt`.
