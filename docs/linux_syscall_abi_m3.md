# SeLinOS Linux syscall ABI M3: bounded anonymous `mmap` and EOF `read`

**Статус:** завершён и воспроизводимо проверен на x86_64/PC99 в QEMU TCG.  
**Граница совместимости:** фиксированный development contract, а не general Linux memory-management ABI.

M3 сохраняет M2 sequence `getpid` → bounded `write` → `exit` и добавляет один реальный anonymous mapping. Изолированный probe делает Linux-shaped `mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)`, получает page address в `RAX`, записывает byte sentinel `0x5a` через этот адрес, затем вызывает узкий EOF `read(0, NULL, 0)` и только после его zero return завершает `exit(0)`.

| Linux syscall | Number | M3 accepted contract | Proof boundary |
|---|---:|---|---|
| `getpid` | 39 | Deterministic return `4242` | Probe tests `RAX` before later calls |
| `write` | 1 | `fd=1|2`, 1–127 bytes, one user page | Root prints copied userspace payload |
| `mmap` | 9 | `addr=0`, `len=4096`, `prot=3`, `flags=0x22`, `fd=-1`, `offset=0` | One RW page mapped only into probe VSpace at `0x70000000` |
| `read` | 0 | `fd=0`, buffer `0`, length `0` | Returns 0 as an EOF stub; no transfer occurs |
| `exit` | 60 | status `0` only | Reached only after sentinel store and EOF result check |

## Memory mapping contract

Gateway reserves and allocates exactly one 4 KiB page at fixed address `0x70000000` in the probe VSpace. It rejects non-null hints, sizes other than 4096, any protection other than `PROT_READ|PROT_WRITE`, any flags other than `MAP_PRIVATE|MAP_ANONYMOUS`, a descriptor other than `-1`, and a non-zero offset. Mapping is not shared with root and does not establish a general allocator, address chooser, `munmap`, `mprotect`, file mapping or copy-on-write implementation.

> **Userspace liveness proof:** after root returns the mapped address in `RAX`, probe executes `movb $0x5a, (%rax)`. A missing or non-writable mapping would cause a fault rather than the subsequent `read` and `exit(0)` faults. The final M3 completion marker is emitted only after gateway receives and validates those later faults.

The M2 `write` transfer retains its capability boundary. Root takes the backing cap of one validated probe page only long enough to duplicate-map it, copy at most 127 bytes, unmap and delete the duplicate cap. The new anonymous mapping does not broaden that transfer rule.

## Reproducible verification

```bash
cd /home/ubuntu/helixos
./tools/verify_linux_syscall_abi_m3.py
```

The verifier rebuilds the current image and runs QEMU through a PTY-backed serial capture. It requires the following ordered markers:

```text
SeLinOS ABI M3: mediated Linux write(1) userspace payload passed.
SeLinOS ABI gateway: getpid, bounded write, mmap, EOF read and exit(0) mediated.
SeLinOS M0: eleven isolated service domains started.
```

The record is [`tests/artifacts/selinos_linux_syscall_abi_m3.verification.json`](../tests/artifacts/selinos_linux_syscall_abi_m3.verification.json); the most recent boot trace is [`tests/artifacts/selinos_linux_syscall_abi_m3.boot.log`](../tests/artifacts/selinos_linux_syscall_abi_m3.boot.log).

## Still absent

M3 does not implement a process heap or `brk`, arbitrary `mmap`, `munmap`, file descriptors, a data-producing `read`, VFS, dynamic linking, glibc, network, `dpkg`, `apt`, or existing Linux ELF execution. The dispatcher remains root-held bootstrap code; `selinos-abi-gated` has not yet become the persistent isolated fault-handling authority.
