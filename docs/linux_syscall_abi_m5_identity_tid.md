# SeLinOS Linux syscall ABI M5: bounded identity and TID slice

**Status:** verified for the single isolated `selinos-linux-syscall-probe` on x86_64/PC99 QEMU TCG. M5 adds mediated returns for four identity calls, `gettid()` and a deliberately non-persistent `set_tid_address()` acknowledgement. It is a deterministic compatibility probe, **not** Linux credentials, TLS, clone, pthread or scheduler support.

## M5 syscall contract

| Linux x86_64 syscall | Number | M5 accepted input | M5 verified result | Omitted Linux semantics |
|---|---:|---|---:|---|
| `getuid` | 102 | No constrained arguments | `0` | Credentials, user namespaces and permission checks. |
| `getgid` | 104 | No constrained arguments | `0` | Groups, namespaces and credentials. |
| `geteuid` | 107 | No constrained arguments | `0` | Effective-credential transition and access control. |
| `getegid` | 108 | No constrained arguments | `0` | Effective-group transition and access control. |
| `gettid` | 186 | No constrained arguments | deterministic test TID `4243` | Per-thread allocation, PID/TID relationships and thread groups. |
| `set_tid_address` | 218 | Nonzero `RDI` range wholly within a page mapped by the caller | deterministic test TID `4243` | Persistent `clear_child_tid`, zero-on-exit store, futex wakeup, clone interaction and lifetime ownership. |

Linux documents `gettid()` as returning the calling thread’s ID; in a single-threaded Linux process it equals the PID, whereas threads in a multithreaded process have distinct TIDs.[1] SeLinOS intentionally returns a distinct deterministic value (`4243`) from its current deterministic test PID (`4242`) to make clear that this milestone does not claim Linux’s actual PID/TID model.

Linux `set_tid_address()` sets per-thread `clear_child_tid`; on termination Linux may zero that address and issue a futex wakeup.[2] M5 validates only that the probe supplied a nonzero address contained in a mapped user page, then returns the test TID. It does not retain the pointer, dereference it, zero memory at exit or issue a futex operation. This is specifically narrower than the Linux interface.

> **Safety boundary:** root captures each UnknownSyscall register frame before replying and before any unrelated IPC. `set_tid_address` permits only a mapped single-page address and produces no persistent root-held user pointer or capability.

## TLS gate: `arch_prctl(ARCH_SET_FS)` remains unimplemented

On x86-64 Linux, `ARCH_SET_FS` sets the 64-bit FS base and `ARCH_GET_FS` returns that base through a user pointer.[3] This is central to common TLS startup paths, but the pinned seL4 x86_64 `UnknownSyscall` fault/reply frame exposes only general-purpose registers, fault instruction pointer, stack pointer and flags. It does **not** expose an FS-base reply register. Consequently, acknowledging syscall 158 without actually updating thread architectural state would be false compatibility and is prohibited.

| Requested prerequisite | Current result | Required before implementation |
|---|---|---|
| `arch_prctl(ARCH_SET_FS)` | Blocked; not accepted | A seL4-supported, audited per-thread FS-base set/get mechanism that preserves isolation across scheduling and fault reply. |
| `ARCH_GET_FS` | Blocked; not accepted | The same authoritative per-thread FS-base state plus safe user-pointer copy semantics. |
| `set_tid_address` exit action | Explicitly not performed | Task lifecycle, per-thread state ownership, termination sequencing and a mediated futex wakeup design. |
| `clone`/pthread startup | Explicitly not performed | New TCB/CSpace/VSpace lifecycle, thread-group model, TLS setup, scheduling and exit/robust-list semantics. |

## Evidence

The probe executes, in order, `getpid`, the four credential calls, `gettid`, mapped-page `set_tid_address`, then the established write/mmap/brk/ROMFS/EOF-read/exit sequence. The root prints:

```text
SeLinOS ABI M5: getuid/getgid/geteuid/getegid/gettid and mapped set_tid_address mediated.
```

The standalone verifier [`tools/verify_linux_syscall_abi_m5.py`](../tools/verify_linux_syscall_abi_m5.py) pins the production image, root dispatcher, isolated assembly probe and QEMU runtime record. It also checks that the M5 record explicitly retains TLS, clone and credential exclusions.

```bash
cd /home/ubuntu/helixos
./tools/verify_linux_syscall_abi_m5.py
```

## References

[1]: https://man7.org/linux/man-pages/man2/gettid.2.html "Linux manual page: gettid(2)"
[2]: https://man7.org/linux/man-pages/man2/set_tid_address.2.html "Linux manual page: set_tid_address(2)"
[3]: https://man7.org/linux/man-pages/man2/arch_prctl.2.html "Linux manual page: arch_prctl(2)"
