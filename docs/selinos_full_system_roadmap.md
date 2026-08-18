# SeLinOS: Git-first roadmap to evidence-backed Debian/Ubuntu compatibility

**Author:** Manus AI

**Status:** living engineering roadmap
**Scope:** x86_64/PC99, seL4 as the sole privileged kernel, native Linux ABI/KABI-oriented user-space services, QEMU TCG-first evidence. No Linux kernel, Linux VM, LKL, or claim that a package works without a reproducible test.

## 1. Objective and definition of success

SeLinOS is intended to become a secure, performant, multi-server operating system in which seL4 is the only privileged kernel and each OS service or application execution context receives a separately constructed TCB, CSpace, and VSpace. The target is not merely to parse a `.deb` file: it is to run selected native amd64 Debian/Ubuntu packages through a verified `apt`/`dpkg` substrate, preserving package trust and isolating the authority of storage, network, loader, package-management and driver domains.

A literal universal statement that **every** Debian/Ubuntu package will run cannot be made honestly in advance. Debian packages can execute arbitrary maintainer scripts during install, upgrade and removal, and those scripts may rely on daemons, kernel APIs, filesystem features, devices or privilege models that have not yet been implemented. Debian policy describes `preinst`, `postinst`, `prerm`, and `postrm` scripts and their error/recovery behaviour; therefore a package is compatible only after its complete install, run, upgrade/remove path has been exercised in a named profile.[1] This roadmap makes the universal goal operational: package coverage expands through published, reproducible compatibility profiles, never through unsupported blanket claims.

> **Release criterion.** A SeLinOS compatibility claim must name the exact image, source revisions, QEMU command, package repository snapshot, package version and architecture, test transcript, verifier, SHA-256 bindings, expected security authority graph, and explicitly excluded behaviour.

| Compatibility tier | Meaning | Claim policy |
|---|---|---|
| **T0 — ABI microproof** | One bounded syscall, loader, VFS, IPC, device or allocator transaction works in a default-OFF probe. | Existing foundation only; never describe as application support. |
| **T1 — hosted ELF program** | A statically or dynamically linked amd64 program launches in an isolated process domain and passes a named functional test. | Name the executable and test. |
| **T2 — local `.deb` transaction** | A selected package's control archive, data archive, ownership database and maintainer-script path complete from local media. | Name the exact `.deb`, its scripts and test. |
| **T3 — authenticated APT transaction** | A signed repository snapshot is fetched, Release metadata and hashes are verified, a dependency plan is resolved, and exact packages are installed. | Name release snapshot, keys, transport and tests. |
| **T4 — profile compatibility** | A published Debian/Ubuntu workload profile succeeds repeatedly, including install, execution and upgrade/removal recovery cases. | Name all packages and accepted limits. |
| **T5 — broad distribution coverage** | A continuously measured package corpus passes across defined workload classes and device configurations. | Report percentage and failures; never call it absolute. |

The architectural target is realistic only if every T4/T5 result is decomposed into the lower tiers. A Debian binary package normally contains executables, libraries, configuration and control metadata; it is unpacked by `dpkg`, commonly under an `apt` frontend.[2] That dependency makes loader correctness, POSIX-like filesystem semantics, process lifecycle, time, identity, cryptography and network transport primary prerequisites rather than later polish.

## 2. Non-negotiable safety, performance and reproducibility invariants

SeLinOS uses seL4 capabilities as the authority boundary: a capability is an unforgeable permission to access a kernel object or resource; kernel resources initially belong to the root task and must be explicitly delegated.[3] The root must therefore never act as a permanent all-powerful worker. It will become a short-lived bootstrap and policy authority whose delegated capabilities are recorded, minimally rights-bearing, revocable where the construction protocol permits, and independently audited.

| Invariant | Design rule | Evidence required before promotion |
|---|---|---|
| **Least authority** | Every service receives only frame, endpoint, IRQ, I/O-port, notification or CNode authority necessary for its protocol. No ambient root capability enters child CSpaces. | Capability inventory plus negative-path probe proving absence of forbidden authority. |
| **Construction atomicity** | Dynamic TCB/CSpace/VSpace construction is staged, generation-bound and rollback-safe. A partially built child remains suspended and unreachable from package-facing namespaces. | Failure injection at every acquisition step, resource-ledger equivalence before/after rollback. |
| **W^X and loader integrity** | No virtual page is writable and executable at the same time. ELF mappings remain non-executable until validation is complete; relocations follow a separately approved write-then-seal protocol. | Page-table and execution-fault witnesses, loader map ledger, negative execution proof. |
| **DMA containment** | No untrusted storage/NIC driver obtains DMA-capable access until a hardware-backed containment policy is proven in the active profile. | IOMMU/IOSpace proof or an explicit trusted-driver policy; no inferred claim from QEMU presence. |
| **Package trust** | APT accepts only repositories with a configured key and valid signed Release metadata; unsafe repository overrides are absent from production profiles. | Signature, freshness, hash-chain and rejection tests. |
| **Failure containment** | Service crash, malformed IPC or hostile package input cannot silently grant new authority, corrupt another service's state or leave package DB ownership ambiguous. | Fuzzing, fault injection, restart and transaction-recovery evidence. |
| **Performance by measurement** | Optimisations follow baselines. Fast IPC, batching, zero-copy only when ownership and revocation are explicit; no security check is removed for throughput. | Deterministic benchmarks with pinned image/source hashes and regression budgets. |
| **Reproducibility** | Host build artifacts are excluded from Git; pinned sources, build configuration, source manifests, verifier code and durable evidence are tracked. | Clean checkout build and independent verifier run. |

The current Phase 49 result is a hard safety gate, not an inconvenience: the active QEMU profile observed `numIOPTLevels=0`, so it has not proved the DMA containment required for an untrusted DMA driver. This aligns with seL4's documented proof assumptions: DMA must be off or trusted unless separately constrained by suitable hardware/driver policy.[4] Storage and networking can continue with non-DMA design work and status/notification observations, but neither may be promoted to untrusted-driver persistent I/O on that profile.

## 3. Git-first development policy

The repository root is a local Git checkout with private remote `dnilkagptilka-cyber/SeLinOS`. It tracks SeLinOS-controlled source, protocols, documentation, verifiers and portable SHA-bound evidence. It excludes CMake/Ninja outputs, bootstrapped upstream worktrees, local caches, host test binaries and raw QEMU logs. Upstream seL4 ecosystem revisions remain reproducibly described in `sources.lock` and fetched by `bootstrap_sources.sh`.

Every implementation milestone uses an isolated branch named `phaseNN-short-scope`, with small commits that separate contract/documentation, protocol, server/probe, verifier/evidence and compatibility-matrix promotion. Each commit message identifies the evidence gate it affects. A change cannot merge into `main` merely because it compiles: it needs a clean configured build, an isolated QEMU TCG proof, an independent verifier, SHA re-binding and the relevant regression suite. Any unresolved threat-model issue blocks promotion rather than being labelled as a known limitation after merge. Private remote publication is part of the promotion gate; protected `main` and signed release attestation remain recommended follow-up controls.

## 4. Delivery sequence

### Workstream A — dynamic isolated task construction (Phases 51–58)

Phases 50–57 are verified as a narrow construction chain: status-only reservation, inert TCB, generation-bound CNode and PML4 rollback, TCB/CNode/PML4 bundle ownership, ASID plus suspended TCB configuration, one notification cap, then a self-rooted two-cap target CSpace. None constituted process execution or a Linux process claim.

**Phase 58 is verified:** a separate default-OFF QEMU TCG profile rolls back a generation-1 TCB/CNode/PML4/frame bundle, then maps one ordinary 4 KiB IPC-buffer frame at fixed target address `0x70000000` through a generation-2 ASID-assigned PML4. It copies the frame cap into target CSpace and configures the still-suspended TCB with that buffer. The proof includes a status-only ownership/rejection witness, SHA-bound transcript, independent verifier, all-profile rebuild and standalone regression. It does not invoke or resume the target.

**Phase 59 is verified:** the same isolated topology receives one whole 20-word all-zero x86_64 `seL4_UserContext` request through `seL4_TCB_WriteRegisters` with `resume_target=0u`, then one full read-back. Pinned seL4 x86 sanitization returns only `rflags=0x202` (mandatory high/interrupt-enable bits) while the other 19 words are zero. The target still has no entry point, stack, invocation or resume. The proof includes the status-only ownership/rejection witness, SHA-bound transcript, independent verifier, 31-profile rebuild and 65 standalone verifiers.

**Phase 60 is verified:** the default-OFF profile adds a badged target fault endpoint to the Phase 59 isolated target, then writes/reads the requested-zero context, performs exactly one resume, receives one instruction-fetch VM fault at zero IP/address, and deliberately withholds any fault reply. The target is fault-blocked. This proves only a bounded dispatch-to-fault transition: no instruction completion, entry point, stack, ELF runtime, or general task launch is claimed. The proof includes a SHA-bound QEMU transcript, independent verifier, 32-profile rebuild and 66 standalone verifiers.

**Phase 61 is verified:** a separate default-OFF profile allocates distinct entry and stack frames, places them at fixed low-canonical target addresses, maps both with x86 `ExecuteDisable`, and writes/reads a full context with `rip=0x60000000` and ABI-derived `rsp=0x70002ff8`. Root proves canonical-address and misalignment negative guards, while the target remains unresumed. The proof includes a SHA-bound QEMU transcript, independent verifier, 33-profile rebuild and 67 standalone verifiers.

**Phase 62 is verified:** a separate default-OFF profile initializes one root-only entry frame with `nop; ud2`, removes the root alias, maps the entry page executable, keeps the stack NX, performs exactly one resume, and receives one badged invalid-opcode user exception at `rip=0x60000001`. Root withholds reply. This proves one NOP completed before the deliberate trap—nothing more. The proof includes a SHA-bound QEMU transcript, independent verifier, 34-profile rebuild and 68 standalone verifiers.

**Phase 63 is verified:** a separate default-OFF profile starts with the `nop; ud2` entry frame unmapped, receives one instruction-fetch VM fault, maps that exact preinitialized frame, and sends one zero-label VM-fault reply. The restarted target completes one NOP then reaches terminal `ud2` UserException, which root does not reply. This proves a narrow x86-supported VM-fault repair/restart only; the pinned kernel rejects UserException fault replies. The proof includes a SHA-bound QEMU transcript, independent verifier, 35-profile rebuild and 69 standalone verifiers.

**Phase 64 is verified:** a separate default-OFF parser profile accepts one deterministic 4 KiB SSIM-v1 RX-only record with fixed low-canonical entry and zeroed-digest SHA-256 source seal. It rejects altered digest, W+X, non-canonical, reserved-field and source-range variants plus a duplicate status request. The proof is parser-only: it maps no frame, changes no permission, resumes no target and is not an ELF claim. The proof includes a deterministic host negative suite, SHA-bound QEMU TCG transcript, independent verifier, 36-profile rebuild and 70 standalone verifiers.

**Phases 65–72 are verified:** Phase 65 maps one accepted sealed-image fixture under a W^X ledger and observes the terminal witness; Phase 66 records terminal lifecycle and retained-object ownership; Phase 67 authorizes teardown status-only; Phase 68 authorizes the entry-stack-IPC revocation order status-only; Phase 69 executes exactly three successful target `Page_Unmap` calls after the terminal fault; Phase 70 deletes exactly the six target-CNode copies; Phase 71 disposes the target bundle through root-held VKA descriptors; and Phase 72 records one logical fresh-generation-2 authorization while rejecting duplicate generation 2 and retired generation 1 as stale. Phase 72 does not allocate or reuse any resource, and proves neither untyped, slot, ASID or virtual-address reuse nor a process lifecycle, ELF or Linux ABI. The Phase 72 isolated QEMU TCG transcript, SHA-bound evidence, independent verifier, **44-profile rebuild** and **78/78 standalone verifier regression** are recorded in `phase72_static_image_fresh_bundle_authorization_m0_gate.md`.

**Phases 73–76 are verified:** Phase 73 constructs one root-owned fresh generation-2 TCB/CNode/PML4/notification/IPC/entry/stack bundle without asserting physical reuse; Phase 74 configures that fresh TCB with self, IPC and badged-fault CSpace slots, an ASID and mapped IPC frame while it remains suspended; Phase 75 writes and reads back the complete fixed RIP/RSP context while suspended; and Phase 76 materializes `NOP; UD2` through a temporary root alias, maps the fresh entry RX and stack NX, resumes exactly once and receives the fresh target’s badged terminal invalid-opcode exception at `rip=0x60000001`, vector `6`, with no reply. Phase 76 is bounded by its isolated QEMU TCG transcript, SHA-bound evidence, independent verifier, **48-profile rebuild** and **82/82 standalone verifier regression**. It does not claim fault repair/restart, a second instruction beyond terminal `UD2`, a runnable process, ELF, Linux ABI or package compatibility. Linux `clone`, `fork`, `vfork`, `pthread`, PID/TID namespaces, signals and wait/reap remain distinct later milestones.

### Workstream B — executable runtime and Linux process surface (Phases 59–82)

The existing x86_64 NX milestone is a prerequisite, not an ELF loader. SeLinOS must add an ELF64 parser that validates headers, program-header bounds, canonical virtual addresses, alignment, page permissions, segment overlap and integer overflow before mapping. Each `PT_LOAD` region must have a source hash, mapping ledger and W^X transition record. The first T1 program should be a sealed statically linked fixture; later gates add `ET_DYN`, relocations, auxv, argv/envp, TLS, vDSO policy, interpreter handling, libc-required syscall breadth and exec teardown.

Process compatibility then grows vertically around real workloads rather than isolated syscall count: descriptors/dup/pipe, paths and cwd, directory iteration, file metadata, memory mapping/unmapping, poll/epoll, signals, timers, credentials, namespaces policy, process relationships, sockets and `execve`. Each syscall contract must specify copyin/copyout bounds, cancellation/error behaviour and negative tests. A compatibility profile names the exact libc, dynamic linker and test executables used.

### Workstream C — durable filesystem and storage (Phases 83–105)

The storage stack begins only with safe transport. The existing virtio milestones establish device discovery and restricted queue observations but neither descriptor processing nor DMA containment. A production candidate requires either a positive IOMMU proof on a platform/profile that exposes usable I/O page-table levels or a consciously trusted, minimal driver boundary documented outside the untrusted-driver safety claim. It then needs an ownership-safe descriptor allocator, bounce-buffer or mapped-buffer policy, queue completion, interrupt protocol, reset recovery and error injection.

Above it, the VFS must implement stable inodes, directories, hard links/symlinks, metadata, permissions, open-file descriptions, offsets, `fsync`, atomic rename, page cache policy, mounts and crash recovery. A journaled or copy-on-write root filesystem should first support the minimal `dpkg` database paths. Package install atomicity requires a transaction protocol that can reconcile filesystem state, package database state and interrupted maintainer scripts; simple mutable RAM files are never sufficient.

### Workstream D — network, time and cryptographic repository transport (Phases 106–130)

Networking is built as separately constrained NIC, ARP/IPv4/IPv6, routing, DNS, UDP/TCP, resolver and HTTPS/TLS domains. The TCP and TLS stacks need structured fuzzing and deterministic loopback/in-VM test peers before public network use. Entropy, monotonic time, wall clock policy, CA/keyring storage and certificate validation must exist before package acquisition claims.

APT repository support has an explicit trust chain. `apt-secure` checks Release file signatures and then hashes leading to repository metadata and downloaded package files; it refuses unauthenticated archives by default.[5] SeLinOS therefore must prove keyring ownership, signature-algorithm policy, replay/freshness controls, transport certificate verification, Releases/InRelease handling, `Packages` parsing, hash verification and rejection of downgrade/unsigned inputs. An unauthenticated path may exist only as a separately named developer test profile, never as a production default.

### Workstream E — dpkg, apt and workload profiles (Phases 131–165)

The first package milestone is deliberately local and narrow: parse a known `.deb`, verify archive members, extract safe paths, record per-package file ownership and execute an approved test package whose maintainer scripts use only the currently implemented capability profile. The next gates add control fields, version comparison, dependency graph resolution, alternatives, conffile policy, diversions, `Pre-Depends`, replacements/conflicts and failure recovery.

`dpkg` correctness is defined as a state machine, not a successful unpack. Debian policy requires maintainer-script error handling and specifies partially installed and half-configured states through installation, upgrade, configuration and removal flows.[1] Each such state must be replayable after a simulated service loss or power interruption. `apt` then layers index acquisition, solver policy, download cache, authenticated repository handling and handoff to the verified `dpkg` transaction substrate.

The first public profile should be a minimal, immutable package set with signed local repository snapshots and simple dynamically linked command-line programs. Profiles then progress through shell/coreutils-like tools, package-manager self-hosting, developer toolchain, network clients, service managers and desktop stacks. Kernel modules, custom kernel drivers, containers that require Linux namespaces/cgroups, eBPF, GPU stacks and hardware-specific packages remain separate compatibility classes; their status is measured individually.

### Workstream F — Linux KABI, driver domains and hardware portability (continuous)

The Linux 6.18.44 KABI work remains a bounded, version-pinned compatibility programme. It cannot become “all C drivers work” merely by parsing a `.ko`; real support requires a defined symbol/version surface, loader protections, device-model semantics, memory/IRQ/DMA policy, concurrency, timer/workqueue behaviour, power-management policy and hardware test matrices. Any driver domain that can compromise DMA integrity must not receive uncontained authority.

New device support begins with virtual QEMU fixtures, then machine-checkable virtual hardware profiles, then physical hardware. The x86_64/PC99 QEMU TCG configuration remains the functional evidence baseline, but real hardware support requires separate evidence and cannot be inferred from TCG.

## 5. Quality gates and measurement programme

Each milestone produces a gate document, protocol header, server/client implementation, default-OFF CMake profile, QEMU transcript, SHA-bound evidence record, independent verifier and a compatibility-matrix row. The full regression suite runs before promotion; all supported profile images rebuild after changes to shared bootstrap code. Tests are intentionally adversarial: malformed IPC length/labels, stale generations, duplicate releases, capability lookup failures, memory exhaustion, crash/restart at each transaction stage, corrupt filesystem metadata, network replay, invalid signatures and package archive traversal inputs.

Performance work follows functional correctness. Baselines include boot time, IPC latency/throughput, context-switch rates, page-fault cost, filesystem metadata operations, storage/network I/O, `dpkg` transaction time and package dependency-solver time. A proposed optimisation requires a before/after measurement, a formal ownership explanation and a regression threshold. The preferred optimisations are page sharing only with immutable mappings, preallocated per-domain pools, bounded IPC batches, explicit asynchronous notifications, cache-aware worker placement and profile-guided but reproducible builds. Unsafe shortcuts—shared ambient address spaces, permanent root authority, writable executable mappings, unchecked zero-copy DMA, trust-all APT configuration or ignored error recovery—are prohibited.

## 6. Immediate implementation queue

| Priority | Next deliverable | Completion evidence | Explicitly not claimed |
|---|---|---|---|
| **P0** | Phase 77 fresh-target fault reply and restart design. | Architecture-constrained reply feasibility, explicit user-exception reply policy, one repaired continuation proof or a fail-closed rejection result, QEMU witness and verifier. | General signal/fault semantics, scheduler/process lifecycle or Linux ABI compatibility. |
| **P0** | Phase 78 fresh entry/stack initialization policy. | Explicit initial stack-image ledger, executable-content ownership and negative W^X tests. | ELF process launch or Linux ABI compatibility. |
| **P1** | W^X ELF loading path. | `PT_LOAD` validation/map ledger, relocation boundaries, static fixture execution and verifier. | Dynamic linker/general ELF compatibility until separately tested. |
| **P1** | Resolve the DMA-containment execution environment. | Positive IOMMU/IOSpace evidence on a suitable target, or an explicitly trusted-driver research profile. | Safe untrusted DMA on current QEMU profile. |
| **P2** | Persistent VFS transaction design for `dpkg` database paths. | Crash/rollback design plus in-VM fault-injection prototype. | Persistent `apt`/`dpkg` claim. |
| **P2** | Network, time and repository-trust substrate. | In-VM transport, TLS/keyring/freshness rejection tests. | Authenticated repository transaction until end-to-end tested. |

## 7. Current status boundary

The current baseline records **82 standalone verified probes** and a successful **48-profile rebuild** after Phase 76. This is meaningful progress in evidence infrastructure and primitive OS services: after terminal revocation, target-CNode deletion, root-held disposal and status-only fresh-generation authorization, one independently configured fresh target has completed exactly one `NOP` before a deliberate terminal `UD2` fault under a fresh RX-entry/NX-stack mapping. It is **not** a bootable Debian/Ubuntu replacement and it does **not** yet run `apt`, `dpkg`, arbitrary Linux packages, general Linux applications or arbitrary Linux C drivers. The immediate technical blocker within the fresh-target lifecycle is an explicitly constrained fault-reply/restart policy; the larger blockers remain W^X ELF runtime, durable storage under a valid DMA policy, networking/TLS/time and the package transaction stack.

## References

[1]: https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html "Debian Policy Manual — package maintainer scripts and installation procedure"
[2]: https://www.debian.org/doc/manuals/debian-faq/pkg-basics.en.html "Debian FAQ — basics of the Debian package management system"
[3]: https://docs.sel4.systems/Tutorials/capabilities.html "seL4 documentation — capabilities"
[4]: https://sel4.systems/About/FAQ.html "seL4 FAQ — DMA and proof assumptions"
[5]: https://manpages.debian.org/unstable/apt/apt-secure.8.en.html "apt-secure(8) — APT archive authentication"
