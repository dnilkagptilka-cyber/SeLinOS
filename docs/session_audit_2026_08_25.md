# SeLinOS development audit — 2026-08-25

**Author:** Manus AI
**Audited repositories:** `dnilkagptilka-cyber/SeLinOS`, `dnilkagptilka-cyber/seL4`, `dnilkagptilka-cyber/seL4_libs`

## Executive status

SeLinOS is an evidence-gated, experimental x86_64/PC99 multi-server system in which seL4 is the only privileged kernel. It is not a Debian replacement and does not currently execute Debian binaries, run `dpkg`/`apt`, or provide general Linux ABI, process, storage, network, dynamic-linker, or package-management compatibility. The project has substantial bounded proof infrastructure, but its claims must remain profile-specific.

| Repository | Audited branch / revision | Result |
|---|---|---|
| `SeLinOS` | `main` at `1c72ab7`; active line `phase63-reply-terminal-fault` at `af58255` | `main` is materially behind the active proof line. Despite its historical name, the active line includes Phase 51 through Phase 90 work. |
| `SeLinOS` | `phase91-reproducible-nx-dependencies` at `641d982` | New published continuation branch. It pins the required seL4 forks and stages a non-promoted Phase 92 candidate. |
| `seL4` | `selinos-x86-nx` at `c7b5e55ec8cdddb506d69d96fdc69dc981088571` | One commit over upstream baseline `1326364...`; introduces the x86 execute-disable attribute plumbing. |
| `seL4_libs` | `selinos-x86-nx` at `37b55704c1480ca6a8234cf962d01c099d20e7a1` | One commit over upstream baseline `d8abd95...`; adds the attribute-aware mapping helper. |

## Architecture and present boundary

The active SeLinOS branch contains narrow QEMU-oriented experiments for independently constructed TCB/CSpace/VSpace domains, ownership/revocation sequences, W^X mapping primitives, fixed ELF metadata/load fixtures, controlled ET_DYN relocation, controlled `PT_INTERP` handoff, and self-authored `execve`-number fault transitions. Phase 90 is the latest recorded completed bounded witness: a root context bridge plus a restricted 16-word `FaultIP` reply reaches an RX `mov rax,[rsp+8]; ud2` sequence and recovers one self-authored `argv[0]` pointer.

This does not repair the earlier normal 18-word `UnknownSyscall` reply-frame limitation, does not provide general initial-stack semantics, and must not be described as `execve`, ELF interpreter, glibc, or Debian binary support. The principal distribution-scale gaps remain continuous process/thread lifecycle, general syscall semantics, persistent VFS/storage with valid DMA policy, network/TLS/time, package trust, and the `dpkg`/APT transaction stack.

## Delivered in the continuation branch

Phase 91 corrects a reproducibility defect. The prior bootstrap selected upstream `seL4` and `seL4_libs` revisions even though the x86 W^X profiles compile against interfaces supplied only by the dedicated forks. `bootstrap_sources.sh` and `sources.lock` now pin the two reviewed fork commits explicitly. `tools/verify_phase91_reproducible_nx_dependencies.py` independently checks the exact repository URLs, commit IDs, full lock tuple, and SHA-bound bootstrap/lock files.

Phase 92 is intentionally labelled an **implemented candidate**, not a verified milestone. Its default-OFF CMake profile reuses the Phase 90 self-authored fixture and context bridge, then changes the terminal RX sequence to `mov rax,[rsp+8]; movzx eax,byte ptr [rax]; ud2`. It expects only the fixed first `argv[0]` byte (`0x73`) and contains one target resume. The static checker validates that narrow contract and rejects any claim that the normal reply frame restores `SP`.

| Check | Result |
|---|---|
| Fresh `bootstrap_sources.sh` with fork pins | Passed. |
| Default image configure/build | Passed. |
| `SeLinX86NxMappingProbe=ON` configure/build | Passed. |
| `SeLinExecveReplyArgv0StringByteM0=ON` configure/build | Passed. |
| Phase 91 independent verifier | Passed. |
| Phase 92 static candidate verifier | Passed. |
| Phase 92 QEMU TCG runtime witness | Inconclusive. The 90-second run reached seL4 startup but no rootserver marker before timeout. It is not counted as a pass. |

## Next required work

The immediate task is to obtain a deterministic Phase 92 runtime transcript. The QEMU profile needs a bounded, observable rootserver start procedure or a sufficiently provisioned reproducible runtime window. Only after its exact terminal marker, target `RAX=0x73` readback, SHA-bound transcript, manifest, independent runtime verifier, and regression results are recorded may Phase 92 be promoted from candidate to verified.

After that, the next compatibility steps should remain vertical and evidence-bound: initial-stack string bounds and NUL policy, a deliberately defined `execve` state transition, executable/loader lifetime and teardown, then process/FD/signal primitives needed by a selected static workload. Persistent storage and package management should not be promoted ahead of the DMA-containment and crash-consistency prerequisites described in the project roadmap.
