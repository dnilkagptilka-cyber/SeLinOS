# SeLinOS: native Debian 13.6.0 compatibility gap map

## Status and target

**Status: planning contract — no Debian package compatibility is claimed.** The requested target is a **native `amd64` execution environment on SeLinOS**, built over seL4 without the Linux kernel, Linux VM, LKL, user-mode Linux, or syscall translation into a Linux host. Existing Debian 13.6 `amd64` ELF binaries, their normal interpreters and libraries, and fixed package-manager corpus must execute directly in the **ordinary primary SeLinOS user environment** against SeLinOS-native kernel services and user-space servers; no container, chroot, alternate personality, or compatibility-mode opt-in is permitted.

Debian 13.6 (`trixie`) was released on 11 July 2026. Debian documents `amd64` as a supported architecture. The package corpus must be pinned by separate `debian` and `debian-security` snapshot timestamps, signed `InRelease` files, package indices, SHA-256/SHA-512 data, source-list configuration, and a generated manifest of each selected `.deb`; a mutable mirror name alone is not a reproducible compatibility definition. [1] [2] [3]

> The contract is **binary compatibility**, not source-porting: an ordinary package must not need SeLinOS-specific source changes, relinking, a new SDK, application rewrite, special launcher, or alternate environment. The implementation mechanism may be SeLinOS-specific internally, provided the externally observable Debian `amd64` ABI and package behavior are proven in the primary SeLinOS environment.

## Contract strata

| Stratum | Native completion condition | Present SeLinOS evidence | Gap that remains before a Debian claim |
|---|---|---|---|
| Executable ABI | Existing `amd64` ET_EXEC/ET_DYN binaries load and start under the Linux x86_64 user ABI, including normal ELF interpreter, auxv, TLS and dynamic-linker needs. | Only a metadata parser/W^X policy gate and bounded fixed-instruction witnesses exist. | General ELF segment mapping, relocation, `PT_INTERP`, dynamic linker, initial stack/auxv, TLS, VDSO policy and unrestricted process startup are absent. |
| Linux syscall contract | Native implementations cover the syscall surface actually reached by fixed corpus binaries, with Linux return/error, structure-layout and restart semantics. | A narrow, test-only M1–M15 syscall subset exists: identities, selected files, small mmap/brk, selected time/TLS/futex/VFS calls. | General file, VM, directory, process, signal, IPC, socket, poll/epoll, credential, namespace, timer and thread semantics are absent. |
| Process and thread substrate | `fork`/`vfork`/`clone`/`execve`, PID/TID, wait/reap, signal delivery, file descriptor inheritance, TLS and scheduler state have tested Linux-visible behavior. | Fresh target construction and two bounded instruction/fault experiments only. | No generic process lifetime, execution continuation, clone/fork, signals, wait/reap, dynamic TLS, FD inheritance or namespace behavior. |
| Filesystem and storage | Durable POSIX-visible hierarchy covers metadata, paths, directories, symlinks, permissions, links, mounts, atomic replace/rename and crash recovery required by `dpkg`. | Immutable ROMFS plus one volatile word; root-only non-DMA storage observations. | No persistent filesystem, directory or metadata semantics; no safe block I/O, durable transaction or recovery. |
| Package state machine | Native `dpkg` performs unpack/configure/remove/purge/upgrades including maintainer-script execution, failure state and error unwind. | No `.deb` extraction, package database or scripts. | Requires durable filesystem, archive/decompression, process execution, `/bin/sh`, permissions, exit status, dependencies, triggers, alternatives/diversions and rollback/error-state tests. Debian Policy requires maintainer-script lifecycle and recovery behavior. [4] |
| Repository acquisition/trust | Native APT front end resolves package dependencies, fetches indices/packages, validates release signatures and hashes, and rejects stale/unauthenticated input. | No network stack, time discipline, TLS or archive parser. | DNS/IP/TCP/HTTP(S), trust/keyring, OpenPGP, SHA-2, compression, repository metadata, current-time/freshness and fail-closed error behavior. APT authentication is mandatory by default. [5] |
| Library ecosystem | The frozen corpus’s public ABI dependencies, loader requirements, locale, NSS, PAM, dbus/systemd and graphics/device interfaces are categorized and tested. | No Debian shared-library runtime proof. | Need dependency-closure manifest and phased integration by package class. Compatibility cannot be inferred from syscall count alone. |
| Hardware/kernel packages | Every package must be classified: native-supportable user package, hardware-conditional package, package that needs a SeLinOS-native counterpart, or an incompatible Linux-kernel artifact. | Bounded Linux KAPI fixture and device experiments; IOMMU absence is a blocker for safe DMA. | Kernel images, Linux kernel modules, installer packages, firmware flows and hardware-specific daemons cannot be silently counted as runnable simply because they are Debian packages. |

## Scope definitions required before any “all packages” acceptance result

The phrase **all Debian 13.6 packages** must become a machine-verifiable manifest with these fields:

| Field | Contract decision |
|---|---|
| Architecture | `amd64`; `i386` multiarch is a separate later compatibility target. |
| Archive roots | Frozen `debian` and `debian-security` Snapshot timestamps, plus separately declared optional archives. |
| Suites and components | `trixie`, `trixie-security`, then an explicit choice of `main`, `contrib`, `non-free`, and `non-free-firmware`. |
| Package identity | Binary package name, Debian version, architecture, filename, size, dependency metadata, SHA-256/SHA-512 and source index provenance. |
| Package managers | At minimum `dpkg`, `apt`, `apt-get`, `apt-cache`, `aptitude`, `tasksel`, `apt-utils`, and separately tested front ends such as `synaptic` or GUI software centers if they are within the requested corpus. |
| Hardware classification | User-space-only, virtual-device-supported, requires specific physical hardware, requires a Linux kernel/module, firmware-only, installer-only, or excluded-with-a-failing-test. |
| Acceptance evidence | Per-package install/configure/remove/upgrade result, stdout/stderr/exit status, changed-file manifest, database state, test result, and negative-trust/failure behavior. |

A class that is unavailable in the SeLinOS platform must not be recorded as “supported”. It must either receive a native SeLinOS implementation that preserves the observed package interface or remain an explicit failing row. This matters especially for packages whose declared purpose is to install or control a Linux kernel: no-Linux-kernel SeLinOS cannot truthfully claim their normal kernel-management behavior without a separately specified SeLinOS-native functional equivalent and evidence.

## Evidence gap measured against the current project

The existing matrix proves isolation primitives, a bounded user-fault mechanism, selected Linux ABI probes, immutable/volatile VFS fixtures, and root-only storage/network observations. It explicitly records **no** Debian-package support. The most immediate runtime blocker is not package parsing but a fail-closed Phase 78 observation: a fresh target `ret` that reads `0x70002ff8` produces a VM fault after a nominal NX stack-map request. This is unverified work and must be fixed or rolled back before it becomes a baseline.

The critical implementation order is therefore: (1) reproducible corpus and test harness, (2) fresh user address-space mapping/translation proof, (3) continuous target execution with managed faults, (4) W^X ELF and initial-stack runtime, (5) process/FD/signal/thread substrate, (6) durable local filesystem and `dpkg` transaction model, (7) native network/trust, (8) APT and full package-corpus campaign.

## Explicit non-claims

This document does not claim that SeLinOS currently starts Debian binaries, executes glibc, loads `.deb` packages, runs any package manager, supports Linux kernel packages, supports all hardware, or is compatible with Debian 13.6. It defines the acceptance work required to make those claims testable.

## References

[1] Debian Project, [“Updated Debian 13: 13.6 released”](https://www.debian.org/News/2026/20260711).

[2] Debian Project, [“Debian ‘trixie’ Release Information”](https://www.debian.org/releases/trixie/).

[3] Debian Snapshot, [archive documentation](https://snapshot.debian.org/).

[4] Debian Policy Manual, [“Package maintainer scripts and installation procedure”](https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html).

[5] APT, [`apt-secure(8)`](https://manpages.debian.org/unstable/apt/apt-secure.8.en.html).
