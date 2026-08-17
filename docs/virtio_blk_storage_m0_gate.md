# SeLinOS virtio-blk storage M0 gate

**Status:** `M0 discovery` and `M1 modern common-capability inspection` are verified only in separate default-off QEMU builds. The normal production image invokes neither probe. No virtio BAR is mapped, DMA-enabled, delegated or used by the current production image.

The OASIS virtio specification describes virtio devices as using standard bus interrupts and DMA, with device status, feature bits, configuration space and one or more virtqueues.[1] A writable storage server would therefore require substantially more than the existing root-only PCI BAR transaction and QEMU edu DMA demonstration.

| Required M0 condition | Current status | Minimum acceptance evidence |
|---|---|---|
| Discover QEMU virtio-blk PCI identity | Verified, quarantine-only | Separate QEMU TCG run with `virtio-blk-pci,disable-legacy=on` found documented modern `1af4:1042`; [`verify_virtio_blk_discovery_m0.py`](../tools/verify_virtio_blk_discovery_m0.py) binds code, opt-in image and log. |
| Inspect modern PCI common capability | Verified, quarantine-only | `1af4:1042` capability list walked with finite/bounded validation of common-config type, BAR index, offset and length; metadata only. |
| Assign and validate BAR safely | Root has a quarantined 32-bit BAR primitive, not integrated for virtio | Non-overlap policy, read-back and no-delegation proof for the exact device. |
| Feature/status negotiation | Not implemented | Exact offered/accepted feature trace; driver rejects unsupported features and observes `FEATURES_OK`. |
| Virtqueue memory lifecycle | Not implemented | Root-owned contiguous queue allocations, explicit mapping/DMA ownership and teardown evidence. |
| IRQ completion routing | Not implemented | Token-bound IRQ registration and reset/teardown behavior, without reusing QEMU edu assumptions. |
| Read-only block request | Not implemented | One bounded sector read with descriptor-chain validation and data integrity evidence. |
| Writable transactional storage | Not implemented | Flush/order semantics, persistent image test, crash recovery and filesystem transaction proof. |

> **Safety rule:** a future storage domain must not receive a BAR frame, IRQ or DMA lease until its exact root-issued device token, supported feature subset, queue memory bounds and teardown sequence have independent evidence. The existing QEMU edu lease proves none of these virtio-specific invariants.

The current SeLinOS package-manager prerequisite remains read-only and non-executing. `dpkg`/`apt` claims are blocked by this storage gate together with native execution, TLS/process lifecycle, network and repository trust gates.

> **Verified discovery boundary:** the isolated probe obtained a temporary root PCI configuration IOPort capability, read only vendor/device identifiers, released the capability and reported the modern QEMU block identity. It did not read or program BARs, mutate PCI command state, negotiate virtio features, construct queues, issue IRQs, obtain DMA or create a storage domain.

## References

[1]: https://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html "OASIS Virtual I/O Device (VIRTIO) Version 1.0"

## Current code-bound discovery gap

The current `selinos_pci.c` scanner is intentionally specialized for QEMU edu (`1234:11e8`), and the only quarantined non-EDU assignment helper targets QEMU e1000 (`8086:100e`). A generic storage driver contract still does not exist: M1 adds only a modern QEMU common-capability metadata result, not a BAR frame, IRQ, DMA or virtio status contract. Therefore the first implementation task for this gate is a **root-only discovery primitive** that records a specific tested virtio-blk PCI identity and leaves BAR frames, interrupts, DMA leases and driver-domain startup absent until its dedicated verifier passes.

## QEMU identity policy for a future discovery-only experiment

QEMU documents legacy virtio block as PCI `1af4:1001`; its modern virtio device IDs use `0x1040 + virtio device ID`, making modern block `1af4:1042` for virtio device ID 2.[2] A first discovery-only profile must select **one** of these explicit QEMU configurations and bind its PCI ID in evidence. It must not scan an unrestricted vendor range or infer block semantics from an arbitrary virtio device.

[2]: https://www.qemu.org/docs/master/specs/pci-ids.html "QEMU PCI IDs"
