# Phase 92 — Execve Reply `argv[0]` First-Byte M0 Gate

**Status:** VERIFIED. A clean x86_64/PC99 build that pins the reviewed seL4 NXE commit reached one self-authored RX witness in QEMU 8.2.2 TCG. The target first loads the fixed `argv[0]` pointer from `[rsp + 8]`, then reads only its first byte, and terminates at `ud2` with the zero-extended expected byte in `RAX`.

## Contract

Phase 92 extends the prior Phase 90 pointer observation by exactly one bounded data access. The target uses the established self-authored fixture, isolated TCB/CNode/VSpace, read-only RX interpreter mapping, RW+NX stack mapping, root-only initialization aliases, one syscall-number-59 fault shape, a whole-context root bridge, and one label-zero 16-word reply through `FaultIP`.

```text
mov rax, [rsp + 8]
movzx eax, byte ptr [rax]
ud2
```

The stack is self-authored: its one `argv[0]` pointer designates the in-page string `selinos`. The terminal fault is accepted only when it occurs at interpreter offset `8`, reports invalid-opcode vector `6`, and the root readback of `RAX` equals `0x73`.

| Item | Verified boundary | Not established |
|---|---|---|
| Pointer source | The fixed stack word at `RSP + 8` | Caller-controlled `argv`, `argc` validation, pointer range policy, or multi-element arrays. |
| Data operation | One byte at one self-authored virtual address | NUL search, length calculation, string traversal, arbitrary reads or writes, `envp`, or `auxv` consumption. |
| NX prerequisite | CPUID-gated `EFER.NXE` activation before paging in pinned seL4 | General W^X policy, page-fault recovery, or a full MMU policy framework. |
| Reply path | Root whole-context bridge and one 16-word reply ending at `FaultIP` | Correct normal 18-word `UnknownSyscall` reply-frame restoration. |
| Runtime | One QEMU TCG execution through terminal `ud2` and root `RAX=0x73` check | General `execve`, ELF loading, a dynamic linker, Linux process semantics, or Debian support. |

## Reproducible profile

The successful evidence uses the following bounded command after a clean profile build:

```bash
./bootstrap_sources.sh
cmake -S src -B build-phase92-nxe -GNinja \
  -DSeLinExecveReplyArgv0StringByteM0=ON
cmake --build build-phase92-nxe --parallel 2
cd build-phase92-nxe
./simulate --mem-size=128M --cpu=max --cpu-opt='' \
  --extra-qemu-args='-no-reboot'
```

The environment had no usable `/dev/kvm`; the runtime record therefore uses QEMU 8.2.2 TCG with `cpu=max`, 128 MiB memory, and a 150-second external bound. This CPU profile is material: the default emulated Nehalem configuration did not reach user space within extended bounded runs, while `cpu=max` did. The manifest records the exact images, configuration, source bindings, bootstrap/lock files, kernel NXE implementation, and transcript SHA-256 values.

## Resolved blocker

The first runtime attempt with the earlier execute-disable-only fork produced a target VMFault before the first interpreter load:

```text
IP       = 0x0000000060004000
address  = 0x0000000070002f88
FSR      = 0x000000000000000c
prefetch = 0
```

`FSR=0x0c` is a user-mode reserved-bit page fault. The fork had emitted `xd=1` in leaf PTEs but had not enabled `EFER.NXE`; x86 consequently treated the NX bit as reserved. The new reviewed seL4 commit `5387ba9f0b01481fc7027e0883f1c4587c64fd27` checks CPUID leaf `0x80000001`, requires `EDX[20]`, then sets `EFER.NXE` (`bit 11`) before paging. The confirmed runtime transcript contains no Phase 92 failure marker and reaches the success marker.

## Independent verification

```bash
python3 tools/verify_phase91_reproducible_nx_dependencies.py
python3 tools/verify_execve_reply_argv0_string_byte_m0.py
```

The first verifier binds the complete dependency lock including the kernel NXE commit. The second verifier binds the exact M0 source transaction, profile selector, generated configuration, images, QEMU transcript, gate document, and all explicit non-claims.

## Explicit non-claims

This gate does **not claim** normal 18-word reply-frame restoration or its semantics. Phase 92 does **not** prove normal 18-word reply-frame restoration, a complete initial stack, arbitrary `argv`/`envp`/`auxv` parsing, path-name lookup, argument copying, arbitrary memory dereference, a general `execve` syscall, process replacement, ELF loader correctness, `PT_INTERP` execution, a dynamic linker, libc, POSIX or Linux ABI compatibility, Debian binary execution, `dpkg`, `apt`, or Debian 13.6 package compatibility.
