# SeLinOS reproducibility evidence — development snapshot 2026-08-13

**Release scope:** this is an evidence report for the current development increment. It is **not** a claim of complete Linux ABI, Linux KAPI/KABI, driver, `dpkg`, `apt`, network, persistent-storage or dynamic-linker compatibility.

## Rebuilt default profile

| Field | Value |
|---|---|
| Platform | x86_64 / PC99, QEMU 8.2.2 TCG, `-icount 1` evidence profiles |
| Kernel | pinned seL4; no Linux kernel, LKL or Linux VM |
| Default root image | `build/images/selinos-root-image-x86_64-pc99` |
| SHA-256 | `10eb7f9d6156fd1e9e3570c0b49127049b5c48ddfa9037336a782a6148ee52c2` |
| Standard device profiles | baseline, one QEMU EDU, two QEMU EDU |
| Opt-in experiment | separate `build-virtio-discovery-probe`, `SeLinRootVirtioBlkDiscoveryProbe=ON` |

The default profile explicitly compiles the virtio discovery experiment **off**. Its baseline evidence has no `SeLinOS storage M0: quarantined` marker. The opt-in profile identified modern QEMU virtio block PCI `1af4:1042` but never mapped a BAR, changed PCI command state, negotiated virtio features, constructed queues, issued an IRQ, mapped DMA, performed block I/O or started a storage service.

## Independent verifier set

| Evidence area | Verifier |
|---|---|
| Driver lifecycle | `tools/verify_driver_runtime_m1.py` |
| Dual-device isolation | `tools/verify_m3_two_edu.py` |
| KABI relocation and fixture policy | `tools/verify_kabi_relocation_stage.py`, `tools/verify_kabi_fixture_execution.py` |
| Linux syscall ABI M1–M5 | `tools/verify_linux_syscall_abi_m1.py` through `tools/verify_linux_syscall_abi_m5.py` |
| ROMFS IPC, fixed syscall bridge, CPIO and packed path | `tools/verify_romfs_m1.py`, `tools/verify_romfs_syscall_bridge_m2.py`, `tools/verify_romfs_cpio_m1.py`, `tools/verify_romfs_packed_path_m1.py` |
| ELF metadata parser | `tools/verify_elfrt_m1.py` |
| Bounded KAPI synchronization | `tools/verify_kapi_sync_m1.py` |
| Quarantine virtio discovery | `tools/verify_virtio_blk_discovery_m0.py` |

All sixteen verifiers passed together after rebuilding the default profile and validating the separate opt-in virtio image.

## Current non-claims and gates

| Gate | Current status |
|---|---|
| Native ELF/dynamic loader/module execution | Blocked: pinned x86 mapping attributes expose no audited NX/execute-control policy. |
| Thread TLS and normal glibc startup | Blocked: no authoritative FS-base update path in the used UnknownSyscall reply model. |
| Hardware DMA containment | Blocked in current QEMU: `CONFIG_IOMMU=1`, but boot log says `ACPI: 0 IOMMUs detected`; no IOSpace mapping proof exists. |
| Writable storage/VFS | Not implemented: virtio discovery is identity-only. |
| Network | Not implemented: no NIC BAR/IRQ/DMA delegation or IP stack. |
| Debian package management | Not implemented: no package archive handling, persistent state, script execution or repository trust stack. |

> **Reproduction rule:** a future change that alters an image, a hash-bound source, a runtime log or the asserted behavior must refresh only the affected evidence after a fresh runtime proof, then pass the complete independent verifier set again.

The detailed living matrix is [`current_compatibility_matrix.md`](current_compatibility_matrix.md); individual hard gates are [`elfrt_m2_loader_execution_gate.md`](elfrt_m2_loader_execution_gate.md), [`virtio_blk_storage_m0_gate.md`](virtio_blk_storage_m0_gate.md), [`iommu_vtd_m0_gate.md`](iommu_vtd_m0_gate.md), and [`dpkg_apt_prerequisite_gate.md`](dpkg_apt_prerequisite_gate.md).
