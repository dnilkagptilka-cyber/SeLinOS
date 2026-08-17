# SeLinOS Linux 6.18.44: программа расширения после M0

## Реализованный базис

SeLinOS M0 проверяет capability-safe lifecycle для одного QEMU edu PCI function: обнаружение, BAR0-only mapping в отдельный VSpace и scoped IOAPIC IRQ notification/ack. Одновременно self-authored `selinos-kapi-6_18` headers позволяют собрать Linux-shaped PCI/IRQ/DMA source probe. Этот фундамент не является заменой Linux kernel internal API, VFS, network stack или `.ko` loader.

## Последовательность закрытия совместимости

| Пакет работ | Обязательные компоненты | Критерий завершения | Зависимости |
|---|---|---|---|
| KAPI-IRQ | `irqd`, driver registration table, `request_irq/free_irq`, threaded handler subset | Linux-shaped edu driver получает callback через KAPI, очищает register и release path не оставляет IRQ active | M0 scoped notification/IRQHandler |
| KAPI-DMA | `dmad`, coherent frame allocator, DMA ownership table, 28-bit mask, revocation | QEMU edu 100-byte DMA roundtrip to/from offset `0x40000`; cross-domain or out-of-mask mapping rejected | IOMMU policy, device-untyped allocator |
| KAPI-device/PCI M2 | Isolated `deviced`, profile-limited PCI ID matching, driver-local shadow `pci_dev`, local probe/remove ordering | Self-authored QEMU edu fixture passes `pci_register_driver()` → probe → enable/master → mediated DMA/IRQ → remove; root-static M1 grants remain | KAPI-IRQ, KAPI-DMA |
| KAPI-device/PCI M3 | Token-indexed multi-device registry, `pcid` ownership, dynamic BAR/IRQ/DMA grant and remove/fault teardown | A driver receives and loses only its own resources through lifecycle tokens; revoke/unmap evidence passes | KAPI-device/PCI M2, dynamic `irqd`/`dmad` |
| KAPI-core | slab/page subset, workqueue/completion/waitqueue, spinlock profile, time source | Chosen Linux 6.18 driver source builds against profile without changing driver logic beyond manifest adapter | Scheduler/timeout service |
| Module loader | signed `selmod` manifest, capability request set, relocation against SeLinOS KAPI profile | Signed source-profile module loads into isolated driver domain and unloads/revokes resources | KAPI-device |
| VFS | `vfsd`, fd table, inode/dentry/open/read/write/ioctl, ROMFS then writable FS | Static SeLinOS process accesses files and device nodes via VFS IPC | Processes, shared-page I/O |
| Linux UAPI | ELF loader, syscall personality, signals, futex, `clone`, `mmap`, `epoll` | Targeted musl/glibc-style test suite passes for the declared profile | VFS, processes, memory |
| Network | `netd`, sockets, TCP/IP, NIC driver profile, DNS/TLS userspace dependencies | `apt` transport prerequisites become available | VFS, UAPI, virtio-net/NIC driver |
| Packaging | `dpkg` database semantics, archive extraction, maintainer script sandbox, repository trust | `apt update` and selected package install work in a declared SeLinOS distribution profile | UAPI, VFS, network |
| KABI (separate) | Linux `.ko` ELF parser, relocations, modversions, exact struct/symbol config profile | A specifically compiled Linux 6.18.44 `.ko` passes declared test matrix | Full selected KAPI closure; this is not implied by prior phases |

## Non-negotiable compatibility rules

The profile remains **versioned**: `SELINOS_LINUX_BASELINE_6_18_44`. A source or binary driver is marked compatible only with a recorded Linux commit/tag, architecture, config fingerprint, compiler ABI and test matrix. No milestone may be described as “all Linux drivers” or “absolute Linux compatibility” unless all targeted APIs and actual hardware models have an independently reproducible test result.

SeLinOS must retain the microkernel security boundary. A driver never obtains `IRQControl`, unrestricted IOPort control, full physical memory, other device BARs or another driver's DMA buffers. `deviced`, `irqd` and `dmad` retain those powers and grant a driver only scoped capabilities. The degree to which an unmodified Linux source driver assumes global authority is an explicit measurable gap in the source profile, not a reason to erode isolation silently.

## Current M2 status and first next target

**KAPI-IRQ/DMA M1, KAPI-device/PCI M2 and bounded M3** now form reproducible QEMU edu slices. The self-authored Linux-shaped fixture invokes `pci_register_driver()`, receives an ID-matched immutable shadow `pci_dev` through isolated `deviced`, and runs its `probe()` callback in the driver domain. Within that callback it executes `pci_enable_device()`, `pci_set_master()`, `dma_set_mask_and_coherent()`, `request_irq()`, coherent DMA, MMIO liveness, IRQ acknowledgement and `pci_unregister_driver()`/`remove()`. M3 verifies two concurrent QEMU edu functions, each with independent `deviced`, `irqd`, `dmad` and driver-domain bundle. `irqd` retains each scoped `IRQHandler`; `dmad` performs CPU unmap and frame-cap revoke/delete before accepted release. The evidence verifies registration, wrong-token rejection, local probe/remove ordering, device acknowledgement, seL4 handler acknowledgement and two independent 100-byte DMA roundtrips. Detailed contracts are [`m2_deviced_pci_probe.md`](m2_deviced_pci_probe.md) and [`m3_tokenized_device_lifecycle_design.md`](m3_tokenized_device_lifecycle_design.md).

This remains deliberately **not** an unbounded dynamic multi-device registry, a coherent allocator, general `pcid`, or IOMMU-backed DMA containment. M3's two QEMU bundles are root-bootstrap allocations; they are not hotplug-aware and do not reuse/zero frames or detect driver faults. The next driver-runtime closure requires an unbounded token-indexed registry, reusable zeroised frames, fault-driven revoke and lifecycle ownership retained until teardown completes. Only after that closure should resource delivery be described as general service-owned rather than bounded fixture-root-static.

## KABI M2 relocation status

A freestanding x86_64 `ET_REL` relocation engine now copies allocatable sections into caller-owned bounded memory, resolves only curated undefined imports and applies the exact relocation subset exercised by the real pinned fixture: `R_X86_64_64`, `R_X86_64_PC32`, `R_X86_64_PLT32` and `R_X86_64_32S`. `selinos-modld`, an isolated QEMU-booted domain, checks the fixture's pinned SHA-256, validates all three `__versions` CRC records through the curated table and completes the 17 relocations with non-zero relocated `init_module` / `cleanup_module` addresses. No entrypoint is called. The independent verification is `tools/verify_kabi_relocation_stage.py`; detailed scope is [`kabi_m2_relocation_stage.md`](kabi_m2_relocation_stage.md).

A development-only `selinos-modexec` service now invokes `init_module()` and `cleanup_module()` for that exact self-authored fixture after the same admission gates. The fixture has no PCI, IRQ or DMA capability, and independent evidence observes two curated `_printk` calls. This is still not a general native Linux module loader: the current pinned x86 mapping profile has no independently testable NX-enforced W^X transition. Remaining blockers are Linux-compatible signature/trust policy, a complete SeLinOS export resolver beyond the tested three symbols, an enforceable W^X module VSpace/CSpace mechanism, capability manifests, a module manager, fault containment and safe unload/reload. Detailed development-profile scope is [`kabi_m3_fixture_execution.md`](kabi_m3_fixture_execution.md).

## References

[1] [The Linux Kernel Driver Interface](https://www.kernel.org/doc/html/latest/process/stable-api-nonsense.html).

[2] [QEMU EDU device specification](https://www.qemu.org/docs/master/specs/edu.html).

[3] [Linux generic IRQ handling](https://docs.kernel.org/core-api/genericirq.html).

[4] [Dynamic DMA mapping using the generic device](https://docs.kernel.org/core-api/dma-api.html).

[5] [Kernel module signing facility](https://docs.kernel.org/admin-guide/module-signing.html).
