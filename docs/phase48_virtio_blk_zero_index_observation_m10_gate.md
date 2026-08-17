# Phase 48: virtio-blk root post-notification zero-index observation M10 gate

## Status

**Status: verified, bounded M10 proof.** The default-OFF x86_64/PC99 QEMU TCG `SeLinRootVirtioBlkZeroIndexObservationProbe=ON` profile has independently evidenced one immediate root-local sample of the zero-filled split-ring avail and used index words after an M9-equivalent zero-valued queue-zero notification, before mandatory reset. The SHA-bound record is `tests/artifacts/selinos_virtio_zero_index_observation_m10.verification.json` and the independent checker is `tools/verify_virtio_blk_zero_index_observation_m10.py`.

For a one-entry split ring, the descriptor region starts at offset `0`; the avail flags and index are at offsets `16` and `18`; after the aligned avail region, the used flags and index are at offsets `24` and `26`. The [Virtio 1.3 specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html) defines the split-virtqueue descriptors, available ring and used ring and treats notifications as a distinct driver operation.[1]

> M10 is a **QEMU trace observation only**. It does not establish that the device did not DMA, that DMA is contained, or that any device completed work. It proves only that four words read as zero in the declared immediate sampling point in the named fixture, after no driver descriptor or avail-index publication.

## Closed transaction

| Step | Required bounded behavior |
|---:|---|
| 1 | Repeat M9 validation: common/notification capability range checks, fresh zero-filled one-slot layout, `queue_size=1`, three layout address pairs, one `queue_enable=1` read-back and one zero-valued queue-zero notification write. |
| 2 | Before and after the single notification, read only the layout-local avail flags/index and used flags/index words. Require all four initial words zero and all four immediate post-notification words zero. |
| 3 | Write no descriptor field, avail index, used index, request header, sector, data address, notification-data payload, PCI command bit, interrupt or IRQ control. |
| 4 | Immediately reset device status, require status and queue-enable zero, then unmap/release notification, common and layout state. |

The M10 helper must use no polling, delay loop, interrupt wait or completion check. The M9 notification page is mapped only for the single write and cannot leave the root helper. The owned layout frame is never delegated and remains zeroed by the driver except for no writes after initialization.

## Explicit non-claims

A verified M10 may claim only the described four zero-valued post-notification samples in the specified QEMU TCG run, with the disposable raw fixture unchanged. It cannot claim no DMA, DMA containment, a device completion, a usable queue, descriptor publication or consumption, IRQ, IOMMU/IOSpace policy, hardware safety, block request, block read/write/flush, persistent VFS, storage server, Linux block/KAPI compatibility, `dpkg` or `apt`.

## Promotion rule

Promotion requires a fresh QEMU trace, SHA-bound zero raw fixture, independent verifier, 20-profile rebuild, standalone verifier regression and compatibility-matrix update. A later stage must separately define both a bounded completion observation and an IOMMU/DMA authority policy; neither may be inferred from M10.

## References

[1] [OASIS, *Virtual I/O Device (VIRTIO) Version 1.3*](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html)
