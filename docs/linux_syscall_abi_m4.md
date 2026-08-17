# SeLinOS Linux syscall ABI M4: bounded `brk`

**Статус:** завершён и воспроизводимо проверен на x86_64/PC99 в QEMU TCG.  
**Граница совместимости:** один фиксированный development heap transition; не general Linux `brk` ABI.

M4 расширяет M3 sequence `getpid` → bounded `write` → fixed anonymous `mmap` → EOF `read` → `exit` двумя Linux-shaped `brk` calls. Сначала isolated probe исполняет `brk(0)`; gateway возвращает fixed heap base `0x71000000` и выделяет ровно одну RW 4 KiB page в VSpace probe. Затем probe запрашивает `brk(0x71001000)`. Только это значение принимается как рост break; оно возвращается в `RAX`. Probe записывает heap sentinel `0x6b` в `RAX - 1` до выполнения последующих `read` и `exit` transitions.

| `brk` stage | Accepted input | Returned value | Allocation / proof |
|---|---:|---:|---|
| Query | `RDI = 0` | `0x71000000` | Gateway provisions exactly one RW page at that base |
| Grow | `RDI = 0x71001000` | `0x71001000` | Probe stores `0x6b` at `0x71000fff` and then reaches EOF read / exit |

The root gateway accepts no second grow, arbitrary target, shrink, overlap, accounting request, reclaim, randomised heap base or `brk` call from another process. A mapped heap page is process-local. As with M3 `mmap`, the only proof of its writeability is the absence of a user VM fault before the later mediated calls; gateway emits the M4 completion marker only after it receives validated EOF `read` and `exit(0)` faults.

> **Why this is not a general heap:** Linux `brk` normally participates in a process-wide virtual-memory policy. M4 deliberately proves only the minimum mapping-and-return path needed to expand SeLinOS ABI coverage without claiming allocator, deallocation, resource accounting or multi-process semantics.

## Reproducible verification

```bash
cd /home/ubuntu/helixos
./tools/verify_linux_syscall_abi_m4.py
```

The verifier rebuilds the image, checks source-bound strict constants and QEMU-boot markers, and rejects any trace containing a rejected brk request. The machine-readable record is [`tests/artifacts/selinos_linux_syscall_abi_m4.verification.json`](../tests/artifacts/selinos_linux_syscall_abi_m4.verification.json). The persistent trace is [`tests/artifacts/selinos_linux_syscall_abi_m4.boot.log`](../tests/artifacts/selinos_linux_syscall_abi_m4.boot.log).

## Still absent

M4 has no general `brk`, arbitrary or reclaimable `mmap`, `munmap`, `mprotect`, copy-on-write, data-producing `read`, VFS, dynamic linker, glibc, network, `dpkg`, `apt`, or execution of standard Linux ELF applications. The fault gateway remains root-held bootstrapping code, not the isolated persistent `selinos-abi-gated` server.
