# SeLinOS: критический путь к основной нативной среде Debian 13.6.0

## Design position

SeLinOS will not offer Debian как гостевую ОС, контейнер, chroot или «режим совместимости». Цель — сделать Linux x86_64 userspace ABI и необходимые Debian механизмы **обычным ABI и обычными системными службами основной среды SeLinOS**. Пользователь запускает существующую Debian 13.6 `amd64` программу как обычный процесс SeLinOS; внутренне её системные вызовы, файловые операции, сеть, сигналы и права обслуживаются нативными изолированными seL4-серверами.

> Совместимость измеряется наблюдаемым поведением зафиксированных бинарных пакетов, а не похожестью внутренней реализации на Linux. Внутренние модели SeLinOS могут быть безопаснее и многосервернее, но для совместимого пути они обязаны воспроизводить нужные Linux ABI результаты, ошибки, структуры и последовательности событий.

## Dependency-critical path

| Порядок | Программа работ | Обязательный результат | Почему это критический путь |
|---:|---|---|---|
| 0 | **Corpus lock и test oracle** | Подписанный снимок Debian 13.6 `amd64`, manifests, dependency graph, классы пакетов и воспроизводимый acceptance harness. | Без этого «все пакеты» не является проверяемым утверждением. |
| 1 | **Fresh VSpace и stack translation** | Любой выделенный fresh target может читать/писать разрешённые страницы, исполнять RX entry и получать управляемые fault events; Phase 78 VMFault устранён или зафиксирован fail-closed. | Требуется до любого ELF, `/bin/sh`, loader или `dpkg`. |
| 2 | **Непрерывное выполнение и process authority** | Controlled execution beyond one terminal witness; process create/exit, PID/TID, scheduling, fault policy, parent/child wait/reap. | Пакетные инструменты, shell и scripts требуют обычных процессов. |
| 3 | **W^X ELF64 + initial process ABI** | `PT_LOAD`, main executable, `PT_INTERP`, relocation, initial stack `argc`/`argv`/`envp`/auxv, TLS, dynamic linker and libc fixture. | Почти каждый Debian binary зависит от ELF runtime и glibc. |
| 4 | **Linux syscall and descriptor substrate** | Contract-driven `openat`, files/directories, `read`/`write`, `statx`, `getdents64`, `ioctl` boundary, memory, futex, time, random, poll/epoll, pipes, sockets. | User programs and libraries require these interfaces before package tooling can run. |
| 5 | **Persistent POSIX filesystem** | Durable inode/directory/symlink/permission/rename/link/xattr subset, file-lock and crash recovery semantics, root hierarchy. | `dpkg` modifies `/var/lib/dpkg`, `/etc`, `/usr`, logs and conffiles transactionally. |
| 6 | **Native `dpkg` transaction substrate** | `.deb` archive support, compression, status database, package states, dependency order, conffiles, triggers, alternatives/diversions and maintainer-script process execution/recovery. | Debian Policy’s scripts and error-unwind paths are central to package correctness. [1] |
| 7 | **Network, time and trust** | NIC/block drivers under valid DMA policy; DNS, IP, TCP, HTTP(S), clock, DNS/TLS validation, Debian archive keyring and signed index verification. | Native APT needs online archive acquisition and fails closed on unauthenticated metadata. [2] |
| 8 | **Native APT and all declared front ends** | `apt`, `apt-get`, `apt-cache`, `aptitude`, `tasksel`, `apt-utils` plus each declared graphical/automation front end are installed and executed in primary SeLinOS. | Passing only `apt-get update` does not prove the requested package-manager corpus. |
| 9 | **Whole-corpus campaign** | Install/configure/upgrade/remove/purge test per manifest package with package-class-specific hardware outcomes and no unclassified failures. | This is the only point where a bounded all-package claim becomes possible. |

## Parallel workstreams that do not bypass the path

| Workstream | Can progress in parallel | Cannot replace |
|---|---|---|
| Linux 6.18.44 KABI driver surface | Pinned driver API/KABI catalog, static symbols, managed driver domains. | DMA containment, device class support, package/user ABI or kernel-package compatibility. |
| Virtio/e1000 research | Driver state machines and QEMU device behavior. | A safe storage/network service; current QEMU has no observed IOMMU containment. |
| Package-corpus analysis | Dependency closure, ELF imports, syscalls observed, maintainer-script command inventory. | Native ELF/process/filesystem execution. |
| Security hardening | Service decomposition, least authority, W^X, signature checks, rollback policy. | Correct Linux-visible semantics needed by existing binaries. |

## Architecture boundary

The normal SeLinOS system consists of a root authority domain and isolated native servers for memory/object lifetime, process management, VFS, block storage, networking, identity, time, package state, and selected device paths. The Linux-compatible ABI gateway is part of the ordinary process-service dispatch path, not an optional personality. It must not delegate operations to a Linux kernel.

The project must still distinguish **binary userspace package compatibility** from packages whose stated function is to install, boot, configure or load the Linux kernel. For the latter, SeLinOS needs an explicit native semantic replacement and acceptance test; it cannot claim compatibility merely because a `.deb` file unpacked successfully.

## Current stop condition

Phase 78 has an unverified fresh-target `ret` experiment. Its entry code reaches a stack read at `0x70002ff8`, and QEMU reports a VMFault even after a nominal NX stack mapping request. The unverified branch must not be merged into the verified baseline. The immediate engineering action is a page-table translation audit that compares the fresh VSpace’s IPC mapping hierarchy with the stack mapping path, followed by either a proof of mapped stack access or a clean rollback with a documented blocker.

## References

[1] Debian Policy Manual, [“Package maintainer scripts and installation procedure”](https://www.debian.org/doc/debian-policy/ch-maintainerscripts.html).

[2] APT, [`apt-secure(8)`](https://manpages.debian.org/unstable/apt/apt-secure.8.en.html).
