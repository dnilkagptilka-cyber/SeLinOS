# SeLinOS: программа доказательства нативной совместимости с Debian 13.6.0

## Status

**Planning only.** This program defines how a future compatibility claim will be accepted. It does not claim that SeLinOS presently executes any Debian 13.6 package, library, package manager, or Linux kernel package.

The target is the **primary SeLinOS environment**. No compatibility container, chroot, virtual machine, Linux kernel, host-Linux syscall delegation, special launcher, or application source change is allowed in an acceptance run.

## Acceptance artifact set

Every named corpus run must produce the following immutable artifacts:

| Artifact | Required content |
|---|---|
| `corpus.lock.json` | Snapshot archive timestamp(s), `InRelease` bytes and hashes, signing-key fingerprints, suite/component/architecture fields, package-index hashes and package manifest hash. |
| `packages.jsonl` | One record per binary package: package/version/architecture, filename, SHA-256/SHA-512, source index, dependency fields, package class and expected result. |
| `environment.json` | SeLinOS commit, build profile, seL4 commit/configuration, QEMU version/arguments, virtual-device layout and clock/trust policy. |
| `run/<package>.json` | Install/configure/upgrade/remove/purge commands, exit codes, logs, file/database deltas, spawned-program results and negative tests. |
| `summary.json` | Counts by outcome: passed, expected-hardware-conditional, explicitly unsupported, failed, unclassified. A release cannot claim completeness if `failed` or `unclassified` is non-zero. |
| Independent verifier | Validates hashes, run completeness, no prohibited execution path, and the declared outcome for every manifest package. |

## Compatibility levels

| Level | Definition | Claim that becomes allowed |
|---|---|---|
| L0 — archive integrity | Signed repository metadata and package hashes are parsed and rejected on any trust/freshness failure. | Native Debian archive trust primitive only. |
| L1 — package representation | `.deb` records and compressed payloads are parsed/extracted with path, hash and metadata validation. | Native package-format support only. |
| L2 — primary-process runtime | Existing fixed `amd64` binaries run in ordinary SeLinOS with ELF, loader, libc, syscall, signal and FD behavior required by test fixtures. | Named binary-fixture compatibility. |
| L3 — `dpkg` core | Native `dpkg` can execute install/configure/remove/purge and failure recovery against a durable SeLinOS filesystem. | Named `dpkg` transaction scenarios. |
| L4 — APT core | Native `apt`/`apt-get` resolve an authenticated frozen repository and cause L3 transactions. | Named APT install/update/upgrade scenarios. |
| L5 — manager corpus | Every declared package-management front end runs in primary SeLinOS with named functional tests. | Named manager-corpus compatibility. |
| L6 — frozen package corpus | Every entry in the locked Debian 13.6 `amd64` manifest reaches an evidence-recorded declared outcome. | Complete support statement only for package classes whose expected result is `passed`; all other classes remain explicit non-support. |

## Work packages and evidence gates

| Wave | SeLinOS work package | Required proof | Blocks |
|---:|---|---|---|
| A | Address-space ownership and Phase 78 repair | Fresh stack page is mapped, read and written under explicit permission ledger; fault report contains expected translation fields; regression remains green. | Any true process execution. |
| B | Executable lifetime | More-than-one-instruction target continuation, fault policy, teardown/reuse and scheduler accounting. | ELF, shell and scripts. |
| C | ELF64 process start | Static fixture then `PT_INTERP`/dynamic linker, relocation, auxv, TLS and initial standard stack. | glibc and ordinary Debian binaries. |
| D | ABI inventory harness | Corpus binary import/syscall observation, glibc loader diagnostics, deterministic missing-ABI reports. | Prioritized syscall/service implementation. |
| E | Linux-visible process/FD model | `execve`, fork/clone policy, wait/reap, signals, pipes, descriptor tables, poll/epoll, credentials and namespaces as corpus demands. | `dpkg` scripts and service tools. |
| F | Persistent VFS | Directories, metadata, symlinks, rename/link, ownership/modes, atomic update, crash recovery, package database durability. | Package extraction and state. |
| G | Native `dpkg` | Archive extraction, package states, scripts, triggers, conffiles, alternatives/diversions and failure unwind. | APT installation. |
| H | Native transport/trust | Safe device/storage path, DNS/IP/TCP/HTTP(S), time, archive keyring, signature/hashes and reject tests. | Online APT. |
| I | Native APT/front ends | `apt`, `apt-get`, `apt-cache`, `aptitude`, `tasksel`, `apt-utils` and any included declared front end. | Whole-corpus result. |
| J | Whole-corpus campaign | Per-package matrix including install/configure/upgrade/remove/purge and required functional smoke tests. | Any “all package” assertion. |

## Package-class policy

“Every package” needs a truthful observable policy. The initial manifest classifies each package before testing; it does not silently drop difficult categories.

| Class | Acceptance criterion in primary SeLinOS | Examples of required decision |
|---|---|---|
| User-space command/library | Installs and runs expected corpus smoke test using native SeLinOS ABI. | CLI tools, shared libraries, language runtimes. |
| Daemon/service | Installs, starts and stops through the native SeLinOS service model while preserving the package-visible interface. | Network/system services. |
| GUI package | Installs and passes stated display/session test after native graphics/session services exist. | Desktop tools/front ends. |
| Hardware-conditional package | Install/state behavior plus functional test on a declared supported device or explicit compatibility test device. | Firmware/updaters/device utilities. |
| Linux-kernel artifact | Must receive a named SeLinOS-native semantic equivalent with evidence, or remain explicitly non-supported. | Linux kernel images/modules/installers. |
| Build/development package | Install, compile and link a fixed source fixture against expected ABI, without a SeLinOS-specific port. | Compilers, headers, build tools. |

## Prohibited shortcuts

An acceptance verifier must reject a run if it detects any of the following: Linux kernel boot, LKL, user-mode Linux, a Debian VM, Docker/OCI/LXC container, host syscall proxying, host package execution substituted for the SeLinOS process, a chroot used as the functional environment, a package rebuilt specifically for SeLinOS, or skipped/unclassified manifest entries presented as successful.

## Completion statement

The final claim must enumerate the precise `corpus.lock.json` identity and summary counts. It must say **“native compatibility with the locked corpus”**, not “all Debian packages” in the abstract. If the manifest includes a package class for which no SeLinOS-native semantics exist, that class remains a visible failure rather than a hidden exception.
