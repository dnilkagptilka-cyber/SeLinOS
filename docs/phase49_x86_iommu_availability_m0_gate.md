# Phase 49: x86 IOMMU availability observation M0 gate

## Status

**Status: verified, bounded M0 blocker proof.** The default-OFF x86_64/PC99 QEMU TCG `SeLinRootIommuAvailabilityProbe=ON` profile independently observed `numIOPTLevels == 0`; the SHA-bound record is `tests/artifacts/selinos_iommu_availability_m0.verification.json` and its checker is `tools/verify_iommu_availability_m0.py`. This gate records only the kernel-exposed `seL4_BootInfo.numIOPTLevels` availability state required to decide whether a later DMA-containment experiment may be proposed. It does not configure an IOMMU, assign a PCI requester, allocate IOSpace, map DMA memory or alter the existing root-only virtio proofs.

> The QEMU fixture used by the existing storage stages is expected to expose `numIOPTLevels == 0`. M0 must make that fact reproducible from a fresh bounded runtime observation rather than infer it from earlier logs. A zero count is an expected **blocker result**, not an error and not an authorization to use DMA without containment.

## Closed transaction

| Step | Required bounded behavior |
|---:|---|
| 1 | Use only existing seL4 boot/topology metadata or a read-only kernel-exposed enumeration interface available to root. |
| 2 | Record the exact read-only `numIOPTLevels` scalar and one availability classification in root-local state. |
| 3 | Emit one distinct marker for `numIOPTLevels == 0` and one for `numIOPTLevels > 0`; either branch resets/releases any temporary observation state. |
| 4 | Never invoke IOSpace/DMA map/unmap, PCI requester assignment, bus-mastering control, MMIO mapping, queue programming, notification, IRQ or block I/O. |

## Acceptance and non-claims

A successful M0 may state only the observed `numIOPTLevels` value and whether the QEMU fixture is unavailable for later containment work. It cannot claim an IOMMU is configured, enabled, reachable by a device, attached to an endpoint, enforcing a page table, capable of fault reporting or sufficient for DMA isolation. It cannot claim DMA behavior or safety, device-driver authority, virtio queue usability, storage I/O, persistent VFS, Linux KAPI compatibility, `dpkg` or `apt`.

## Promotion rule

Promotion requires an independent verifier, SHA-bound QEMU trace and all-profile regression. Any future containment milestone must first establish endpoint-specific IOSpace authority, translation ownership, mapping/revocation lifecycle and fault behavior in a fixture whose bootinfo exposes non-zero IOMMU page-table levels.
