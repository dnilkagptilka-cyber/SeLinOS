# SeLinOS virtio-blk status/feature M2 gate

**Status:** design-only. This is the next potential VM/QEMU functional slice after the separately verified M1 PCI common-capability metadata inspection.

Virtio requires drivers to accept only offered feature bits and to use the ordered device-status protocol; the device must not consume buffers before `DRIVER_OK`.[1] SeLinOS therefore must not bypass the intervening proof obligations by treating the current PCI metadata as a valid block interface.

| Proposed M2 action | Required precondition | Required proof | Still excluded |
|---|---|---|---|
| Read common-config bytes | Exact BAR base/size read-back and arithmetic proof that `offset + common-config minimum length` lies inside that BAR. | Root-only frame mapping with read-only MMIO access, immediate unmap/revoke and negative out-of-range test. | BAR delegation, storage domain, driver MMIO authority. |
| Observe offered features | M2 read map is valid; selector/feature register offsets are bounds-checked. | Captured offered feature words with no feature write. | Accepted features, `FEATURES_OK`, status mutation. |
| Prepare status negotiation | Separate gate, after an explicit unsupported-feature policy. | Reset, `ACKNOWLEDGE`, `DRIVER`, accepted-feature write and observed `FEATURES_OK`; reset on failure. | `DRIVER_OK`, queues, notification, IRQ, DMA and requests. |

> **M2 stop rule:** no descriptor, available/used ring, notification, interrupt acknowledgement, DMA lease or block request may exist in an M2 image. If a capability does not fit wholly inside a root-validated BAR range, the image must fail closed and revoke the temporary mapping.

## Reference

[1]: https://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.html "OASIS Virtual I/O Device (VIRTIO) Version 1.0"

## Local teardown pattern

The existing EDU DMA service provides a relevant **ordering pattern**, not a reusable virtio authority grant: it fail-closes the lease, then unmaps the derived CPU page cap, revokes the child cap and deletes the child slot; on any failure it leaves the lease denied. A root-only virtio M2 temporary mapping must similarly make the mapping unavailable before capability reclamation and must surface each failure. Because M2 has no driver child and no DMA, its exact objects differ, and this pattern must not be represented as an IOMMU or block-I/O proof.

## Local frame-admission prerequisite

The local VKA exposes `vka_alloc_frame_at(vka, size_bits, paddr, frame)`, which is already used in the EDU path. M2 may use it only for a page-aligned physical interval wholly contained in the selected virtio BAR and common-capability range. The resulting frame cap remains root-owned, is never copied to a driver domain, and must be released after the temporary root mapping is unmapped.
