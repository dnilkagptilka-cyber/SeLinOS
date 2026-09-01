# SeLinOS development audit — 2026-08-25

**Author:** Manus AI
**Audited repositories:** `dnilkagptilka-cyber/SeLinOS`, `dnilkagptilka-cyber/seL4`, `dnilkagptilka-cyber/seL4_libs`

## Executive status

SeLinOS is an evidence-gated, experimental x86_64/PC99 multi-server system in which seL4 is the only privileged kernel. It is not a Debian replacement and does not currently execute Debian binaries, run `dpkg`/`apt`, or provide general Linux ABI, process, storage, network, dynamic-linker, or package-management compatibility. The project has substantial bounded proof infrastructure, but its claims must remain profile-specific.

| Repository | Audited branch / revision | Result |
|---|---|---|
| `SeLinOS` | `main` at `1c72ab7`; active line `phase63-reply-terminal-fault` at `af58255` | `main` is materially behind the active proof line. Despite its historical name, the active line includes Phase 51 through Phase 90 work. |
| `SeLinOS` | `phase91-reproducible-nx-dependencies` | Published continuation branch; its post-Phase-91 commits pin NXE activation and verify Phase 92 M0. |
| `seL4` | `selinos-x86-nxe` at `5387ba9f0b01481fc7027e0883f1c4587c64fd27` | Two commits over upstream baseline `1326364...`; adds execute-disable plumbing and CPUID-gated `EFER.NXE` activation. |
| `seL4_libs` | `selinos-x86-nx` at `37b55704c1480ca6a8234cf962d01c099d20e7a1` | One commit over upstream baseline `d8abd95...`; adds the attribute-aware mapping helper. |

## Architecture and present boundary

The active SeLinOS branch contains narrow QEMU-oriented experiments for independently constructed TCB/CSpace/VSpace domains, ownership/revocation sequences, W^X mapping primitives, fixed ELF metadata/load fixtures, controlled ET_DYN relocation, controlled `PT_INTERP` handoff, and self-authored `execve`-number fault transitions. Phase 90 is the latest recorded completed bounded witness: a root context bridge plus a restricted 16-word `FaultIP` reply reaches an RX `mov rax,[rsp+8]; ud2` sequence and recovers one self-authored `argv[0]` pointer.

This does not repair the earlier normal 18-word `UnknownSyscall` reply-frame limitation, does not provide general initial-stack semantics, and must not be described as `execve`, ELF interpreter, glibc, or Debian binary support. The principal distribution-scale gaps remain continuous process/thread lifecycle, general syscall semantics, persistent VFS/storage with valid DMA policy, network/TLS/time, package trust, and the `dpkg`/APT transaction stack.

## Delivered in the continuation branch

Phase 91 corrects a reproducibility defect. The prior bootstrap selected upstream `seL4` and `seL4_libs` revisions even though the x86 W^X profiles compile against interfaces supplied only by the dedicated forks. `bootstrap_sources.sh` and `sources.lock` now pin the two reviewed fork commits explicitly. `tools/verify_phase91_reproducible_nx_dependencies.py` independently checks the exact repository URLs, commit IDs, full lock tuple, and SHA-bound bootstrap/lock files.

Phase 92 is now a **verified M0 milestone**. Its default-OFF CMake profile reuses the Phase 90 self-authored fixture and context bridge, then reaches `mov rax,[rsp+8]; movzx eax,byte ptr [rax]; ud2`. It accepts only the fixed first self-authored `argv[0]` byte (`RAX=0x73`) and contains one target resume. The initial runtime VMFault (`FSR=0x0c`) exposed that the execute-disable fork emitted XD leaf bits without enabling `EFER.NXE`; the new reviewed seL4 pin adds a CPUID-gated NXE activation before paging. The Phase 92 manifest independently binds this implementation, the kernel source, clean build outputs, QEMU transcript, and its explicit non-claims.

| Check | Result |
|---|---|
| Fresh `bootstrap_sources.sh` with fork pins | Passed. |
| Default image configure/build | Passed. |
| `SeLinX86NxMappingProbe=ON` configure/build | Passed. |
| `SeLinExecveReplyArgv0StringByteM0=ON` configure/build | Passed. |
| Phase 91 independent verifier | Passed. |
| Phase 92 static contract verifier | Passed. |
| Phase 92 clean QEMU 8.2.2 TCG runtime witness (`cpu=max`, 128 MiB, no KVM) | Passed; terminal `ud2` readback proves `RAX=0x73`. |

## Next required work

The next compatibility steps should remain vertical and evidence-bound: initial-stack string bounds and NUL policy, a deliberately defined `execve` state transition, executable/loader lifetime and teardown, then process/FD/signal primitives needed by a selected static workload. Persistent storage and package management should not be promoted ahead of the DMA-containment and crash-consistency prerequisites described in the project roadmap.

## Phase 93 M1 continuation

Phase 93 M1 is verified on top of Phase 92 M0. The default-OFF profile adds exactly one fixed NUL-sentinel byte read at `argv[0] + 7`, while preserving the pointer in `RAX`, placing the first byte in `RCX`, and requiring `RDX=0` at the terminal `ud2`. The initial implementation exposed a register-clobber issue: `movzx eax, byte ptr [rax]` destroyed the pointer before the second read and produced a VMFault at address `0x7a`. The corrected witness uses `movzx ecx, byte ptr [rax]` and passed the bounded runtime test.

| Phase 93 M1 check | Result |
|---|---|
| Clean `SeLinExecveReplyArgv0StringNulM1=ON` build | Passed. |
| Default-OFF isolation | Passed; M1 is not enabled in default configuration. |
| QEMU 8.2.2 TCG runtime (`cpu=max`, 128 MiB, bounded 150 seconds) | Passed; terminal marker reached and root readback required `RAX=0x70002f00`, `RCX=0x73`, `RDX=0x00`. |
| M0, Phase 91 and M1 independent verifiers | Passed. |

M1 remains a fixed self-authored sentinel proof. It does not establish a string-walk loop, NUL search, bounded copy, general `argv`/`envp`/`auxv` handling, normal reply-frame restoration, general `execve`, ELF execution, Linux ABI compatibility, or Debian 13.6 compatibility.

The current working branch contains the full M1 evidence gate, SHA-bound manifest and canonical runtime transcript. The next technical increment should be selected only after reviewing whether the project wants a bounded fixed-length copy proof or a separately specified initial-stack policy; neither should be promoted as general `execve` without a new contract.
