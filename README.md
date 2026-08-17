# SeLinOS M0

**SeLinOS** (*seL4-based Linux Internal-API Operating System*) — экспериментальная многосерверная ОС для **x86_64/PC99**, построенная поверх микроядра **seL4**. В дереве проекта нет Linux-ядра, Linux VM или Linux Kernel Library. Первый загрузочный образ демонстрирует модель, согласованную для проекта: один seL4 в привилегированном режиме и несколько независимых пользовательских доменов, каждый из которых создаётся со своим **TCB**, **CSpace**, **VSpace** и fault endpoint.

> **Важно:** SeLinOS не является заменителем Debian/Ubuntu и не готовой Linux-совместимой ОС. Проверен только узкий Linux x86_64 syscall ABI M4 с ROMFS bridge M2 для последовательности `getpid` → bounded `write(1|2)` → fixed one-page anonymous `mmap` → fixed one-page `brk` query/grow → fixed `openat`/`read`/`close` release-record route → EOF `read(0, NULL, 0)` → `exit(0)` через fault-mediated gateway; отсутствуют general syscall dispatcher, general `brk`, arbitrary `mmap`/`read`, dynamic linker, general VFS, сеть, `dpkg`, `apt`, POSIX threads и запуск существующих Linux ELF. `abi-gated`, `romfsd` и `consoled` остаются отдельными загруженными служебными доменами и точками расширения, а не завершёнными реализациями ABI, файловой системы или драйвера консоли.

## Что уже работает

> **Обновление SeLinOS M0:** в QEMU с `-device edu` подтверждены discovery PCI `1234:11e8`, выдача в отдельный driver domain только BAR0 + scoped IOAPIC IRQ capabilities, MMIO liveness и IRQ raise/wait/ack. Подробности и точные ограничения находятся в [`docs/qemu_edu_mmio_m0.md`](docs/qemu_edu_mmio_m0.md).

> **KABI M1:** SeLinOS сформировала и проверила настоящий Linux 6.18.44 ELF64 `.ko` artifact. Проверены `vermagic`, `.modinfo`, 9 relocation sections, два undefined imports и три `__versions` CRC records против manifest из 12 748 Linux exports. Это проверка module artifact, а не загрузка или исполнение Linux driver. Полный отчёт: [`docs/kabi_m1_module_fixture.md`](docs/kabi_m1_module_fixture.md).

> **Driver-runtime M1:** QEMU edu получает BAR0, mediated IRQ delivery и одну explicit 28-bit DMA frame lease. Изолированный `selinos-irqd` удерживает `IRQHandler` и выполняет kernel acknowledgement только после device-level acknowledgement от driver domain. 100-byte `RAM → EDU → RAM` roundtrip, single-binding KAPI `request_irq()`/`free_irq()` lifecycle и pre-granted KAPI `dma_alloc_coherent()` lease подтверждены. Это QEMU test-only profile с single-lease `dmad` release acknowledgement, но **без IOMMU DMA containment**, dynamic `dmad` allocator или general KAPI RPC runtime; подробности: [`docs/m1_mediated_irq_dma.md`](docs/m1_mediated_irq_dma.md).

> **KAPI-device M2:** Изолированный `selinos-deviced` выполняет synchronous QEMU edu PCI ID matching для `pci_register_driver()`. KAPI shim создаёт driver-local immutable `struct pci_dev` projection и вызывает `probe()` в VSpace driver domain; fixture прошёл `pci_enable_device()` → `pci_set_master()` → DMA/IRQ → `pci_unregister_driver()`/`remove()`. Это один QEMU edu profile, а не general Linux device-model compatibility: BAR/IRQ/DMA grants пока root-static; проверены CPU Page_Unmap и exact DMA child-cap revoke/delete, но отсутствуют dynamic registry, frame reuse и IOMMU. Подробности: [`docs/m2_deviced_pci_probe.md`](docs/m2_deviced_pci_probe.md).

> **KAPI synchronization M1:** В изолированном QEMU edu driver domain проверены bounded `completion`, synchronous `workqueue`, pending-only `timer` и single-domain `spinlock` shims. Они не блокируют, не создают worker threads, не обрабатывают timer callbacks и не маскируют IRQ; следовательно, это narrow source/runtime slice, а не scheduler/timer/SMP locking compatibility. Независимая проверка: [`tools/verify_kapi_sync_m1.py`](tools/verify_kapi_sync_m1.py); подробности: [`docs/kapi_sync_m1_bounded_shims.md`](docs/kapi_sync_m1_bounded_shims.md).

> **Linux syscall ABI M4 + ROMFS bridge M2:** отдельный `selinos-linux-syscall-probe` проходит `getpid` (`RAX=39`) → bounded `write(1, payload, length)` → strict anonymous `mmap(NULL, 4096, RW, PRIVATE|ANON, -1, 0)` → `brk(0)`/`brk(0x71001000)` → fixed `openat`/`read`/`close` route to `selinos-romfsd` → EOF `read(0, NULL, 0)` → `exit(0)`. Root-held seL4 fault endpoint возвращает PID `4242`, безопасно временно map-ит validated one-page write buffer до 127 bytes, выделяет one-page mmap area в `0x70000000` и one-page heap area в `0x71000000`; `exit(0)` принимается лишь после userspace sentinel stores и return checks. Это development slice, не general ABI dispatcher. Подробности: [`docs/linux_syscall_abi_m4.md`](docs/linux_syscall_abi_m4.md).

> **Linux syscall ABI M5:** тот же isolated probe доказал deterministic `getuid`/`getgid`/`geteuid`/`getegid` (=0), `gettid` (=4243) и mapped-page-only `set_tid_address` acknowledgement (=4243). Адрес не сохраняется и не очищается at exit; `clone`, futex, credentials и pthread lifecycle отсутствуют. `arch_prctl(ARCH_SET_FS)` намеренно не реализован: pinned seL4 x86_64 UnknownSyscall reply frame не предоставляет FS-base state. Независимая проверка: [`tools/verify_linux_syscall_abi_m5.py`](tools/verify_linux_syscall_abi_m5.py); подробности: [`docs/linux_syscall_abi_m5_identity_tid.md`](docs/linux_syscall_abi_m5_identity_tid.md).

> **ROMFS/VFS M1:** изолированный `selinos-romfsd` и отдельный probe получают ровно одну endpoint capability в CSpace slot 8. Probe доказал lifecycle `open` → immutable `read` → invalid-FD rejection → `close` для embedded release record. Это direct server IPC foundation, а не Linux file-syscall integration, CPIO parser или general VFS. Подробности: [`docs/romfs_m1_foundation.md`](docs/romfs_m1_foundation.md).

> **ROMFS syscall bridge M2:** Linux syscall probe доказал fixed `openat(AT_FDCWD, "/selinos-release")` → `read(fd=3, mapped_page, 8)` → `close(3)` sequence. Gateway routes lifecycle to isolated ROMFS server and временно map-ит только backing page caller buffer to copy `SELINOS!`; userspace validates returned payload before exit. Это один fixed contract, а не pathname/VFS/file-descriptor implementation. Подробности: [`docs/romfs_syscall_bridge_m2.md`](docs/romfs_syscall_bridge_m2.md).

> **ROMFS CPIO M1:** `selinos-romfsd` теперь сам проверяет и parser-ом читает embedded immutable `newc` CPIO archive. Direct isolated probe доказал два internal pathname projections: `/selinos-release` и `/selinos-banner`, второй с content `CPIO-OK!`; existing release syscall bridge сохранён. Client-supplied arbitrary paths, directories, streaming reads, per-client FD ownership и general VFS отсутствуют. Независимая проверка: [`tools/verify_romfs_cpio_m1.py`](tools/verify_romfs_cpio_m1.py); подробности: [`docs/romfs_cpio_m1_multifile.md`](docs/romfs_cpio_m1_multifile.md).

> **ROMFS packed-path M1:** direct isolated probe передаёт `/selinos-banner` в новом four-word `OPEN_PATH` IPC request как length + два big-endian packed words; `romfsd` принимает только 1–16 printable non-NUL bytes and resolves the path against the immutable CPIO archive. Это не arbitrary pathname API: user pointers, traversal, directories and symlinks are rejected by design. Независимая проверка: [`tools/verify_romfs_packed_path_m1.py`](tools/verify_romfs_packed_path_m1.py).

> **virtio-blk storage M0:** отдельная default-OFF QEMU build конфигурация доказала root-only PCI identity discovery для modern `1af4:1042` virtio block device. В этом proof временная root IOPort cap только читает vendor/device ID и освобождается; BAR, PCI command, feature/status, queue, IRQ, DMA, storage domain и block I/O отсутствуют. Независимая проверка: [`tools/verify_virtio_blk_discovery_m0.py`](tools/verify_virtio_blk_discovery_m0.py); gate: [`docs/virtio_blk_storage_m0_gate.md`](docs/virtio_blk_storage_m0_gate.md).

> **ELF runtime M1:** freestanding `selinos-elfrt` проверяет metadata real ELF64/x86_64 `ET_DYN` fixture: bounded program headers, `PT_LOAD`, policy-controlled `PT_INTERP`, `PT_DYNAMIC`, `DT_NEEDED` и relocation indicators. Он не map-ит, не релокирует, не запускает interpreter или ELF code; это не dynamic linker, glibc или Linux executable support. Подробности: [`docs/elfrt_m1_parser.md`](docs/elfrt_m1_parser.md).

Образ `images/selinos-root-image-x86_64-pc99` собирается из закреплённых официальных исходников seL4 и запускается в QEMU TCG. Root task содержит CPIO с пятнадцатью ELF: десятью permanent service domains, включая отдельные `selinos-modld`, `selinos-modexec` и dedicated `selinos-romfsd`, двумя isolated probes (Linux syscall и ROMFS), двумя QEMU edu driver images и dormant KAPI probe image. `selinos-modld` проверяет pinned SHA-256, три version-pinned `__versions` CRC и релокирует embedded real Linux 6.18.44 fixture в собственную writable memory; он не получает executable mapping и не вызывает `init_module()`/`cleanup_module()`.

| Домен | Назначение на M0 | Изоляция, созданная seL4 | Текущий статус |
|---|---|---|---|
| `rootd` | Bootstrap и временная authority | Первичный TCB/CSpace/VSpace | Работает; пока не отдал bootstrap authority |
| `objectd` | Будущая выдача kernel objects | Отдельные TCB/CSpace/VSpace/fault endpoint | Запускается, IPC ещё не реализован |
| `taskd` | Будущий lifecycle доменов/PID/TID | Отдельные TCB/CSpace/VSpace/fault endpoint | Запускается, IPC ещё не реализован |
| `memd` | Будущие mappings и shared-frame договоры | Отдельные TCB/CSpace/VSpace/fault endpoint | Запускается, IPC ещё не реализован |
| `deviced` | Mediated QEMU edu Linux-shaped PCI/device lifecycle | Отдельные TCB/CSpace/VSpace/fault endpoint | Проверены PCI ID match и authorisation local `probe()`/`remove()` через endpoint; DMA free выполняет unmap и exact child-cap revoke/delete; dynamic registry, frame reuse и IRQ revoke ещё не реализованы |
| `romfsd` | Immutable release-record IPC service | Отдельные TCB/CSpace/VSpace/fault endpoint; только shared ROMFS endpoint | Probe проверил narrow open/read/close lifecycle и invalid-FD guard; нет pathnames, CPIO parser, persistent storage или Linux file syscalls |
| `consoled` | Будущий terminal protocol | Отдельные TCB/CSpace/VSpace/fault endpoint | Запускается; M0 использует debug serial вывод seL4 |
| `abi-gated` | Будущая persistent маршрутизация Linux/POSIX ABI | Отдельные TCB/CSpace/VSpace/fault endpoint | Запускается; M1 `getpid` пока доказан узким root-held fault gateway, а не этим persistent server |
| `selinos-irqd` | Mediated QEMU edu hardware IRQ acknowledgement | Отдельный TCB/CSpace/VSpace; scoped IRQHandler только при наличии edu | Single-binding KAPI `request_irq()`/`free_irq()` lifecycle проверен; general multi-device RPC ещё не реализован |
| `selinos-dmad` | Scoped QEMU edu DMA lease lifecycle | Отдельный TCB/CSpace/VSpace; endpoint только для driver domain | Fixed lease `dma_free_coherent()` performs CPU unmap and exact child-cap revoke/delete; dynamic allocator and reuse remain unimplemented |
| `selinos-modld` | Non-executing Linux 6.18.44 `.ko` preparation | Отдельный TCB/CSpace/VSpace/fault endpoint; no PCI/IRQ/DMA authority | Embedded fixture SHA-256, `__versions` CRC and ET_REL relocation verified; no entrypoint invocation |
| `selinos-modexec` | Development-only execution of pinned fixture | Отдельный TCB/CSpace/VSpace/fault endpoint; no PCI/IRQ/DMA authority | `init_module()` and `cleanup_module()` verified for one self-authored fixture; NX-enforced W^X and general/third-party `.ko` execution remain unproven |

Итоговый вывод QEMU сохранён в [`qemu_boot.log`](qemu_boot.log). В нём подтверждена загрузка seL4, запуск root task и выполнение всех постоянных service domains.

## Проверенная загрузка

Выполненная проверка использовала QEMU 8.2.2 и TCG CPU `max`. Для совместимости эмулятора конфигурация M0 использует `KernelFSGSBase=msr` и отключает `KernelSupportPCID`; это **параметры эмуляционного development build**, а не рекомендация для конечного железа.

```text
SeLinOS M0: seL4 root task started.
SeLinOS M0: bootstrap capabilities present.
SeLinOS M0: capability policy validation passed.
SeLinOS M0: QEMU edu PCI function not present.
SeLinOS ABI M3: mediated Linux write(1) userspace payload passed.
SeLinOS ABI gateway: getpid, write, mmap, brk, ROMFS open/read/close, EOF read and exit(0) mediated.
SeLinOS M0: twelve isolated service domains started.
SeLinOS romfsd: immutable release record service online.
SeLinOS ROMFS probe: open/read/close and invalid-FD guard passed.
SeLinOS deviced: no PCI device record was issued; registry dormant.
SeLinOS modld: module entrypoints discovered; execution intentionally disabled.
SeLinOS KAPI 6.18: QEMU edu absent; PCI probe domain dormant.
SeLinOS M0 service online: abi-gated
SeLinOS M0 service online: consoled
SeLinOS M0 service online: romfsd
SeLinOS M0 service online: memd
SeLinOS M0 service online: taskd
SeLinOS M0 service online: objectd
```

## Быстрый старт

Сначала установите зависимости на Ubuntu 24.04 или эквивалентной системе. Скрипт `bootstrap_sources.sh` получает **только официальные** Git-репозитории и фиксирует точные commits в `sources.lock`.

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build clang lld qemu-system-x86 qemu-utils \
  ccache python3 python3-dev python3-pip python3-venv device-tree-compiler \
  protobuf-compiler libxml2-utils cpio
sudo pip3 install ply

cd selinos
./bootstrap_sources.sh
mkdir build && cd build
cmake -G Ninja -DCMAKE_TOOLCHAIN_FILE=../src/kernel/gcc.cmake ../src
ninja
```

Для запуска с TCG QEMU выполните:

```bash
cd selinos/build
timeout --signal=TERM 10s ./simulate -c max -o "" --reset-terminal

# Интеграционная проверка QEMU edu: mediated PCI enable, BAR0 MMIO, IRQ и 100-byte DMA roundtrip
timeout --signal=TERM 12s ./simulate -c max -o "" --extra-qemu-args='-device edu' --reset-terminal
```

Вывод будет идти в терминал через последовательную консоль. Принудительное завершение по таймауту нормально: service domains намеренно работают в бесконечном `seL4_Yield()` loop до появления IPC event loop.

После успешных QEMU и KABI проверок machine-readable evidence record можно независимо сверить так:

```bash
cd selinos
./tools/verify_driver_runtime_m1.py
./tools/verify_m3_two_edu.py
./tools/verify_kabi_relocation_stage.py
./tools/verify_kabi_fixture_execution.py
./tools/verify_linux_syscall_abi_m1.py
./tools/verify_linux_syscall_abi_m2.py
./tools/verify_linux_syscall_abi_m3.py
./tools/verify_linux_syscall_abi_m4.py
./tools/verify_linux_syscall_abi_m5.py
./tools/verify_romfs_m1.py
./tools/verify_romfs_syscall_bridge_m2.py
./tools/verify_elfrt_m1.py
./tools/verify_kapi_sync_m1.py
./tools/verify_romfs_cpio_m1.py
./tools/verify_romfs_packed_path_m1.py
./tools/verify_virtio_blk_discovery_m0.py
```

## Воспроизводимость

| Артефакт | Расположение |
|---|---|
| Lockfile исходников | [`sources.lock`](sources.lock) |
| Сценарий получения зависимостей | [`bootstrap_sources.sh`](bootstrap_sources.sh) |
| Топ-level seL4 CMake-каркас | [`src/CMakeLists.txt`](src/CMakeLists.txt) |
| Конфигурация x86_64/QEMU | [`src/settings.cmake`](src/settings.cmake) |
| Root task и policy | [`src/projects/selinos/src/`](src/projects/selinos/src/) |
| Отдельные CPIO service ELF и `deviced` IPC server | [`src/projects/selinos/servers/`](src/projects/selinos/servers/) |
| Архитектурная спецификация | [`docs/architecture.md`](docs/architecture.md) |
| Build image | `build/images/selinos-root-image-x86_64-pc99` |
| Базовое QEMU evidence | [`qemu_boot.log`](qemu_boot.log) |
| QEMU edu PCI/MMIO/IRQ evidence | [`selinos_edu_irq.log`](selinos_edu_irq.log) |
| Driver-runtime M1 IRQ/DMA design and limits | [`docs/m1_mediated_irq_dma.md`](docs/m1_mediated_irq_dma.md) |
| KAPI-device M2 mediated PCI probe profile | [`docs/m2_deviced_pci_probe.md`](docs/m2_deviced_pci_probe.md) |
| M3 token-indexed multi-device lifecycle contract | [`docs/m3_tokenized_device_lifecycle_design.md`](docs/m3_tokenized_device_lifecycle_design.md) |
| Next dmad revocation implementation contract | [`docs/phase4_dmad_revocation_design.md`](docs/phase4_dmad_revocation_design.md) |
| VT-d/IOSpace sources and hardware-profile boundary | [`docs/research_dma_iommu_sources_2026-08-13.md`](docs/research_dma_iommu_sources_2026-08-13.md) |
| Driver-runtime M1 QEMU evidence | [`selinos_dma_irq_m1.log`](selinos_dma_irq_m1.log) |
| Driver-runtime M1 machine-readable verification | [`tests/artifacts/selinos_driver_runtime_m1.verification.json`](tests/artifacts/selinos_driver_runtime_m1.verification.json) |
| Linux 6.18.44 KABI baseline | [`docs/profiles/SELINOS_LINUX_BASELINE_6_18_44.md`](docs/profiles/SELINOS_LINUX_BASELINE_6_18_44.md) |
| Full KABI symbol/CRC manifest | [`docs/profiles/selinos-linux-6.18.44-kabi-manifest.json`](docs/profiles/selinos-linux-6.18.44-kabi-manifest.json) |
| KABI M2 non-executing real-module relocation stage | [`docs/kabi_m2_relocation_stage.md`](docs/kabi_m2_relocation_stage.md) |
| KABI M2 integrity/CRC/relocation verifier | [`tools/verify_kabi_relocation_stage.py`](tools/verify_kabi_relocation_stage.py) |
| KABI M3 controlled native module-execution contract | [`docs/kabi_m3_module_execution_design.md`](docs/kabi_m3_module_execution_design.md) |
| KABI M3 verified development-only fixture execution | [`docs/kabi_m3_fixture_execution.md`](docs/kabi_m3_fixture_execution.md) |
| KABI M3 fixture-execution verifier | [`tools/verify_kabi_fixture_execution.py`](tools/verify_kabi_fixture_execution.py) |
| x86_64 W^X module-domain research and current blocker | [`docs/research_module_wx_x86_64_2026-08-13.md`](docs/research_module_wx_x86_64_2026-08-13.md) |
| Bounded dual-EDU M3 verifier | [`tools/verify_m3_two_edu.py`](tools/verify_m3_two_edu.py) |
| Linux syscall ABI M1 subsumed baseline | [`docs/linux_syscall_abi_m1.md`](docs/linux_syscall_abi_m1.md) |
| Linux syscall ABI M1 independent verifier | [`tools/verify_linux_syscall_abi_m1.py`](tools/verify_linux_syscall_abi_m1.py) |
| Linux syscall ABI M2 subsumed getpid/write/exit baseline | [`docs/linux_syscall_abi_m2.md`](docs/linux_syscall_abi_m2.md) |
| Linux syscall ABI M2 independent verifier | [`tools/verify_linux_syscall_abi_m2.py`](tools/verify_linux_syscall_abi_m2.py) |
| Linux syscall ABI M3 subsumed bounded mmap/EOF-read contract | [`docs/linux_syscall_abi_m3.md`](docs/linux_syscall_abi_m3.md) |
| Linux syscall ABI M3 independent verifier | [`tools/verify_linux_syscall_abi_m3.py`](tools/verify_linux_syscall_abi_m3.py) |
| Linux syscall ABI M4 bounded brk contract | [`docs/linux_syscall_abi_m4.md`](docs/linux_syscall_abi_m4.md) |
| Linux syscall ABI M4 independent verifier | [`tools/verify_linux_syscall_abi_m4.py`](tools/verify_linux_syscall_abi_m4.py) |
| Linux syscall ABI M5 identity/TID contract | [`docs/linux_syscall_abi_m5_identity_tid.md`](docs/linux_syscall_abi_m5_identity_tid.md) |
| Linux syscall ABI M5 independent verifier | [`tools/verify_linux_syscall_abi_m5.py`](tools/verify_linux_syscall_abi_m5.py) |
| ROMFS/VFS M1 IPC contract and limits | [`docs/romfs_m1_foundation.md`](docs/romfs_m1_foundation.md) |
| ROMFS/VFS M1 independent verifier | [`tools/verify_romfs_m1.py`](tools/verify_romfs_m1.py) |
| ROMFS syscall bridge M2 contract and limits | [`docs/romfs_syscall_bridge_m2.md`](docs/romfs_syscall_bridge_m2.md) |
| ROMFS syscall bridge M2 independent verifier | [`tools/verify_romfs_syscall_bridge_m2.py`](tools/verify_romfs_syscall_bridge_m2.py) |
| ELF runtime parser M1 contract and limits | [`docs/elfrt_m1_parser.md`](docs/elfrt_m1_parser.md) |
| ELF runtime parser M1 independent verifier | [`tools/verify_elfrt_m1.py`](tools/verify_elfrt_m1.py) |
| ELF runtime M2 execution gate / W^X dependency | [`docs/elfrt_m2_loader_execution_gate.md`](docs/elfrt_m2_loader_execution_gate.md) |
| Planned capability-scoped network M0 scope | [`docs/network_m0_scope.md`](docs/network_m0_scope.md) |
| Root PCI resource allocator M0 prerequisite | [`docs/pci_resource_allocator_m0_gate.md`](docs/pci_resource_allocator_m0_gate.md) |
| Bounded KAPI synchronization M1 | [`docs/kapi_sync_m1_bounded_shims.md`](docs/kapi_sync_m1_bounded_shims.md) |
| KAPI synchronization M1 independent verifier | [`tools/verify_kapi_sync_m1.py`](tools/verify_kapi_sync_m1.py) |
| Bounded CPIO ROMFS M1 | [`docs/romfs_cpio_m1_multifile.md`](docs/romfs_cpio_m1_multifile.md) |
| CPIO ROMFS M1 independent verifier | [`tools/verify_romfs_cpio_m1.py`](tools/verify_romfs_cpio_m1.py) |
| Bounded packed-path ROMFS M1 verifier | [`tools/verify_romfs_packed_path_m1.py`](tools/verify_romfs_packed_path_m1.py) |
| dpkg/apt prerequisite gate | [`docs/dpkg_apt_prerequisite_gate.md`](docs/dpkg_apt_prerequisite_gate.md) |
| virtio-blk writable-storage M0 gate | [`docs/virtio_blk_storage_m0_gate.md`](docs/virtio_blk_storage_m0_gate.md) |
| virtio-blk discovery M0 independent verifier | [`tools/verify_virtio_blk_discovery_m0.py`](tools/verify_virtio_blk_discovery_m0.py) |
| virtio-blk common-capability M1 independent verifier | [`tools/verify_virtio_blk_common_cap_m1.py`](tools/verify_virtio_blk_common_cap_m1.py) |
| Current evidence-led compatibility matrix | [`docs/current_compatibility_matrix.md`](docs/current_compatibility_matrix.md) |
| VT-d / IOSpace hardware-containment M0 gate | [`docs/iommu_vtd_m0_gate.md`](docs/iommu_vtd_m0_gate.md) |
| Current reproducibility evidence snapshot | [`docs/reproducibility_evidence_2026-08-13.md`](docs/reproducibility_evidence_2026-08-13.md) |
| Execution-capable hardware/profile gate | [`docs/execution_capable_hardware_profile_gate.md`](docs/execution_capable_hardware_profile_gate.md) |
| Real-hardware target-admission evidence template | [`docs/phase10_hardware_target_admission_template.md`](docs/phase10_hardware_target_admission_template.md) |
| Q35 virtual-IOMMU M0 experiment gate | [`docs/virtual_iommu_profile_m0_gate.md`](docs/virtual_iommu_profile_m0_gate.md) |
| Bounded virtio block-read M1 design gate | [`docs/virtio_blk_read_m1_design.md`](docs/virtio_blk_read_m1_design.md) |
| Virtio status/feature M2 design gate | [`docs/virtio_blk_status_feature_m2_gate.md`](docs/virtio_blk_status_feature_m2_gate.md) |
| Phase 11 VM/virtio checkpoint | [`docs/phase11_vm_virtio_checkpoint_2026-08-14.md`](docs/phase11_vm_virtio_checkpoint_2026-08-14.md) |
| KABI M1 verified module artifact | [`tests/artifacts/selinos_kabi_probe-6.18.44.ko`](tests/artifacts/selinos_kabi_probe-6.18.44.ko) |
| KABI M1 verifier result | [`tests/artifacts/selinos_kabi_probe-6.18.44.verification.json`](tests/artifacts/selinos_kabi_probe-6.18.44.verification.json) |

## Дорожная карта к Linux ABI и пакетам

Linux package manager сам по себе не делает другую ОС бинарно совместимой. Чтобы выполнять типичные Debian-пакеты без Linux-ядра, SeLinOS должен реализовать и протестировать тот ABI-контракт, который приложения ожидают от Linux: ELF loading/dynamic linking, syscall semantics, signals, `fork`/`exec`, виртуальную память, VFS, file descriptors, `epoll`, sockets, `/proc`, credentials и необходимую драйверную основу. Системный вызов является фундаментальным интерфейсом приложения и Linux kernel.[1]

| Веха | Следующий результат | Препятствие к `apt` |
|---|---|---|
| M1 | Capability minimisation: rootd передаёт authority `objectd`/`taskd`/`memd`; endpoints и IPC protocols | Пока нет реальных объектов/IPC |
| M2 | `consoled` + `romfsd`: read-only VFS, stdin/stdout/stderr, static SeLinOS apps | Нет writable storage и Linux ABI |
| M3 | `libselinos`: `exit`, `write`, `read`, `getpid`, `gettid`, `brk`, anonymous `mmap` | Только программы, собранные для SeLinOS |
| M4 | ELF loader, lifecycle, `clone`, futex, signal baseline | Нет full glibc/dynamic linking |
| M5 | writable FS, pipes/PTY, TCP/IP, DNS/TLS, security identities | Нет `dpkg`/`apt` |
| M6 | Port `dpkg`/`apt` и постепенно расширяемый Linux syscall ABI | Совместимость измеряется тестами, не обещанием «все пакеты» |
| M7 | Отдельно одобренный binary-compatibility слой для Linux ELF: instrumentation или новая проверяемая seL4 personality | Любое изменение seL4 меняет доказательную границу ядра |

## Граница верификации

seL4 имеет формальные доказательства для определённых конфигураций, но M0 собран в debug-development режиме с `KernelVerificationBuild=OFF`. Кроме того, формальные утверждения о seL4 не распространяются автоматически на `rootd`, будущие пользовательские серверы, ABI-слой, драйверы или пакеты. SeLinOS не заявляет формальную верификацию всей ОС.

## Лицензии и исходники

Код, написанный для SeLinOS в `src/projects/selinos/`, лицензирован MIT. Внешние исходники остаются в исходных лицензиях seL4-проектов; их точные commits перечислены в `sources.lock`.

## References

[1] [Linux `syscalls(2)` — man7.org](https://man7.org/linux/man-pages/man2/syscalls.2.html)  
[2] [Threads — seL4 Docs](https://docs.sel4.systems/Tutorials/threads.html)  
[3] [Capabilities — seL4 Docs](https://docs.sel4.systems/Tutorials/capabilities.html)  
[4] [PC99 (64-bit) — seL4 Docs](https://docs.sel4.systems/Hardware/X64.html)  
[5] [Configuring and building an seL4 project — seL4 Docs](https://docs.sel4.systems/projects/buildsystem/using.html)  
[6] [Incorporating into your project — seL4 Docs](https://docs.sel4.systems/projects/buildsystem/incorporating.html)
