# Phase 92 — Execve Reply `argv[0]` First-Byte M0 Candidate (Historical)

> **Superseded:** Phase 92 completed runtime verification after the seL4 NXE activation fix. The current contract, evidence, and non-claims are recorded in [`phase92_execve_reply_argv0_string_byte_m0_gate.md`](phase92_execve_reply_argv0_string_byte_m0_gate.md). This file preserves the pre-fix candidate state and must not be treated as the current status.

## Proposed narrow contract

Phase 92 is the immediate successor to the Phase 90 `argv[0]` pointer-read witness. It retains the existing self-authored fixture, isolated target objects, RX interpreter page, RW+NX stack page, root-only initialization aliases, syscall-number-59 fault shape, whole-context read/write bridge, and one label-zero reply restricted through `FaultIP`.

The only proposed behavioral extension is the interpreter payload:

```text
mov rax, [rsp + 8]
movzx eax, byte ptr [rax]
ud2
```

The target stack is self-authored. Its `argv[0]` slot points at the same in-page `selinos` string used by Phase 90. The candidate accepts the terminal invalid-opcode witness only at interpreter offset `8` and only if `RAX == 0x73`, the zero-extended byte value for the first self-authored `s` character.

| Boundary | Candidate behavior | Not established |
|---|---|---|
| Pointer source | Fixed `argv[0]` stack slot at `RSP + 8` | Caller-provided `argv`, `argc` validation, or pointer-range policy. |
| Data access | One byte at the fixed self-authored pointer | String length, NUL search, arbitrary read, write, traversal, `envp`, or `auxv` consumption. |
| Reply path | Existing root TCB-context bridge and one 16-word reply through `FaultIP` | Correct normal 18-word `UnknownSyscall` reply-frame restoration. |
| Runtime | Clean Phase 92 profile compilation | QEMU terminal witness, an ELF loader, general `execve`, dynamic linker, or a Linux process model. |

## Static checks and local build

```bash
cd /path/to/SeLinOS
python3 tools/verify_phase91_reproducible_nx_dependencies.py
python3 tools/verify_execve_reply_argv0_string_byte_m0_candidate.py
cmake -S src -B build-phase92 -GNinja -DSeLinExecveReplyArgv0StringByteM0=ON
cmake --build build-phase92 --parallel 2
```

The static checker ensures the selector is default-OFF, the root dispatcher is separately gated, the candidate uses the exact `mov rax,[rsp+8]; movzx eax,byte ptr [rax]; ud2` sequence, the expected first byte is fixed at `0x73`, there is one target resume, and the candidate does not write `SP` through the normal syscall reply frame.

A local QEMU TCG attempt was limited to 90 seconds. It reached `Starting node #0` and `Mapping kernel window is done`, but no rootserver/Phase 92 marker was emitted before timeout. This is an **inconclusive environment result**, not a passing runtime test and not a failure of the candidate transaction.

## Explicit non-claims

Phase 92 does not prove a normal 18-word reply frame, an ABI-complete initial stack, `argv`/`envp` parsing, arbitrary string dereference, a path-name parser, a general `execve` syscall, ELF interpreter execution, a dynamic linker, libc, POSIX or Linux ABI compatibility, Debian binary execution, `dpkg`, `apt`, or Debian 13.6 package compatibility.
