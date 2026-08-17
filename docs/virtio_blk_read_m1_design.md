# SeLinOS virtio-blk read M1 design gate

**Purpose:** define the smallest eventual read-only virtio block proof. This document authorises no code path by itself and does not relax the current root-only resource boundary.

The virtio specification defines a device status field, feature bits, configuration space and one or more virtqueues; it also requires the driver to negotiate only a subset of offered features and to use normal bus DMA/interrupt mechanisms.[1] Therefore a credible SeLinOS read proof cannot treat a BAR value or a descriptor buffer as a standalone storage interface.

| M1 step | Required behavior | Forbidden behavior before its evidence passes |
|---|---|---|
| PCI capability inspection | Root reads the modern virtio PCI capability list and validates BAR index, offset and length arithmetic without mapping the BAR into a driver. | Accepting malformed capability chains; granting a device frame. |
| BAR admission | Exact 32-bit BAR transaction/read-back under a dedicated root policy. | Generic BAR delegation or use of unverified preassigned ranges. |
| Feature/status contract | Root records offered features, accepts only a fixed minimal subset and verifies `FEATURES_OK`. | Silently accepting unsupported bits or setting `DRIVER_OK` early. |
| Queue allocation | Root owns bounded, aligned descriptor/available/used memory and records lifecycle. | Sharing queue memory with an untrusted service or treating CPU mappings as IOMMU containment. |
| One read request | Exactly one bounded sector range, a fixed descriptor-chain shape and data digest comparison. | Writes, flushes, multiple queues, arbitrary sector/length requests or general block API. |
| Teardown | Device reset/disable, completion quiescence, unmap/revoke/delete sequence and no reuse before completion. | Reusing DMA backing memory before completion/teardown evidence. |

> **Current decision:** Phase 11 begins with root-only modern PCI capability inspection. Even that inspection must be separately proven before BAR mapping, status writes, feature acceptance, virtqueue creation or block I/O. The current QEMU profile has no IOMMU evidence, so any future queue-memory experiment is functional only, not a hardware-containment proof.

## References

[1]: https://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html "OASIS Virtual I/O Device (VIRTIO) Version 1.0"
