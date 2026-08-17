# SeLinOS Phase 14 Linux syscall ABI M8 checkpoint

**Status:** verified on the default x86_64/PC99 QEMU TCG image. M8 extends the fault-mediated, isolated userspace probe by three deliberately narrow syscall shapes. It is not a general memory-management, filesystem-position or POSIX file-I/O implementation.

| Syscall | Exact accepted request | Result and retained limit |
|---|---|---|
| `mprotect` | `mprotect(0x70000000, 4096, PROT_READ|PROT_WRITE)` on the page already provisioned by the existing M4 anonymous-mapping proof. | Returns `0` as an acknowledgement. It does **not** alter permissions, remap/unmap memory or establish W^X. |
| `lseek` | `lseek(fd=5, offset=0, SEEK_SET)` only, after the fixed `/selinos-state` open. | Returns `0` as a stateless no-op. No cursor exists in `romfsd`; nonzero offsets and other origins are not supported. |
| `pread64` | `pread64(fd=5, mapped one-page buffer, count=8, offset=0)` only. | Root sends the existing bounded `READ` IPC to `romfsd`, copies exactly the one-word `BOOTM0!!` response and returns `8`. No partial, arbitrary-offset, immutable-FD or multi-page pread semantics exist. |

> **Interpretation rule:** M8 proves an exact syscall-number/register/reply sequence inside the one isolated probe. A successful return is not a claim of Linux VM protection, file-position, descriptor-table or general VFS compatibility.

## Reproduction

```bash
cd /home/ubuntu/helixos
cmake --build build
qemu-system-x86_64 -accel tcg,thread=single -icount 1 -cpu max \
  -nographic -serial mon:stdio -m size=1G \
  -kernel build/images/kernel-x86_64-pc99 \
  -initrd build/images/selinos-root-image-x86_64-pc99
./tools/verify_linux_syscall_abi_m8.py
```

The functional proof records this marker only after the probe validates all expected return values and reaches its final `exit(0)` UnknownSyscall fault.

```text
SeLinOS ABI M8: mapped-page mprotect acknowledgement, zero-origin lseek and fixed-offset volatile pread64 mediated.
```

| Still excluded | Reason |
|---|---|
| General `mprotect`, NX/W^X, `munmap`, `mremap` or heap/VM lifecycle | The pinned x86 seL4 mapping interface still has no audited execute-disable mechanism, and M8 performs no mapping transition. |
| General `lseek`, directory offsets or read cursor state | `romfsd` serves fixed records and retains no client cursor. |
| General `pread64` or partial/multi-page I/O | The root bridge accepts exactly FD 5, offset zero, one 8-byte word and one mapped page. |
| TLS, `ARCH_SET_FS`, clone, signals and dynamic process startup | The existing UnknownSyscall reply frame lacks an FS-base field and the relevant lifecycle semantics are not implemented. |
| `dpkg`, `apt` or Linux-package execution | Persistent filesystem, user/runtime ABI, storage and trusted repository transport remain absent. |

The source, default image and captured serial log are cryptographically bound in `tests/artifacts/selinos_linux_syscall_abi_m8.verification.json` and checked by `tools/verify_linux_syscall_abi_m8.py`.
