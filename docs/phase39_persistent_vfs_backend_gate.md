# Phase 39: persistent VFS backend gate

## Status

**Status: blocked at design gate.** The current virtio-blk implementation ends at root-only PCI identity, common-capability inspection, BAR sizing/restoration and a temporary uncached read-only feature observation. It deliberately does not enable the device, negotiate features, create a virtqueue, issue DMA, perform a block request, retain a device frame, or delegate storage authority. The current VFS server owns one volatile word that is explicitly reset on every open and boot.

> No persistent VFS, package database, package extraction, `.deb` installation, `dpkg` or `apt` claim may be made from the present implementation.

## Evidence observed before implementation

| Existing component | Present bounded behavior | Missing prerequisite for persistence |
|---|---|---|
| `selinos_pci_observe_qemu_virtio_blk_features()` | Temporarily maps one common-config page read-only, observes feature values, unmaps it and releases its frame. | Retained device lifecycle, feature negotiation, queue setup and status transitions. |
| QEMU virtio profiles M0–M2 | Identity/capability/BAR-range/feature observation only. | Block request transport, completion/IRQ policy, buffer lifecycle and durable media test. |
| `romfsd` | Three immutable CPIO records plus a one-word server-owned volatile state reset on open/boot. | Backing-store abstraction, metadata/data commit protocol, crash/reboot recovery and multi-record namespace. |
| Current QEMU TCG platform | Reports `ACPI: 0 IOMMUs detected`. | Audited DMA containment before a driver-domain queue/DMA claim. |

## Required staged prerequisites

A later storage implementation must first create separate evidence gates for each stage below. They must not be folded into a nominal VFS milestone.

1. **Virtio device activation gate.** Root-only exact status reset/ACK/DRIVER/FEATURES_OK/DRIVER_OK policy, negotiated feature whitelist and exact restoration/failure behavior.
2. **DMA and queue-memory gate.** Fixed queue layout, physical-address provenance, descriptor validation, ownership, completion/interrupt policy and teardown. On the current target this cannot claim hardware isolation while no IOMMU is detected.
3. **One-block root persistence gate.** One fixed block write followed by a fresh-QEMU reboot and read-back, including explicit media initialization and durable write boundary. This is not yet a filesystem.
4. **Persistent state-record gate.** Versioned bounded record with checksum, two-phase commit/recovery and corruption behavior.
5. **Persistent VFS namespace gate.** Explicit names, metadata, directories, offsets, concurrency, failure/recovery and authorization semantics.

## Non-claims

This gate does not authorize software emulation of disk state as a replacement for durable media, host files as a substitute for a seL4-owned block backend, direct DMA from an uncontained driver domain, `dpkg`, `apt`, generic POSIX filesystems, Linux block/KAPI compatibility, or package management.
