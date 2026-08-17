# Phase 47: virtio-blk root zero-descriptor notification M9 gate

## Status

**Status: verified, bounded M9 proof.** The default-OFF x86_64/PC99 QEMU TCG `SeLinRootVirtioBlkZeroDescriptorNotificationProbe=ON` profile has independently evidenced exactly one 16-bit queue-zero notification write after a fresh zeroed M6-equivalent layout and one M7-equivalent queue enable, followed immediately by reset and complete local teardown. The SHA-bound record is `tests/artifacts/selinos_virtio_zero_descriptor_notification_m9.verification.json` and the independent checker is `tools/verify_virtio_blk_zero_descriptor_notification_m9.py`.

The [Virtio 1.3 specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html) describes driver notifications as transport operations on a previously selected virtqueue, with the PCI notification location derived from notification capability metadata and `queue_notify_off`.[1] Phase 47 tests only the described QEMU register path, not a general transport lifecycle.

> A notification is activation-adjacent and may cause device-side activity. This M9 proof does **not** establish that DMA cannot occur, that any DMA is contained, or that the transition is safe on hardware. It makes no IOMMU, IOSpace, bus-mastering, IRQ, descriptor-consumption, block-I/O or media-integrity claim beyond the unchanged disposable QEMU raw fixture.

## Closed transaction

| Step | Required bounded behavior |
|---:|---|
| 1 | Re-run identity, common capability/range and notification capability/range validation; calculate queue-zero notification address with the M8 overflow/range policy. |
| 2 | Create one root-local zero-filled 4 KiB M6-equivalent split-ring frame; only queue size and three address pairs are written and read back. The descriptor table, avail ring and used ring bytes remain zero. |
| 3 | Observe `queue_enable=0`, write/read back `queue_enable=1` once, then perform exactly one volatile 16-bit notification write of queue index `0` at the calculated notification address. |
| 4 | Write no avail index, descriptor, request header, sector, data address, block operation, notification data, interrupt/ISR/MSI-X or PCI command bit. |
| 5 | Immediately reset device status, require `queue_enable=0` and status `0`, then release every temporary notification frame/mapping, common mapping, layout frame and reservation. |

The notification location may be mapped only to the single containing page in root VSpace for one 16-bit write; its page must be derived from the validated scalar M8 location, must not cross capability/BAR bounds, and must be unmapped before return. No frame or mapping can cross the root API boundary.

## Fail-closed acceptance conditions

All common, notification and queue layout bounds checks from M6–M8 remain required. Before notification, the full 4 KiB layout frame must be zeroed; the read-back queue size must be one; all layout addresses must be inside that frame; notification address multiplication/addition and capability-window checks must pass. The helper accepts only queue zero and exactly one notification write. Any failure after mappings exist resets device status where reachable and releases all owned mappings, reservations and frames.

## Explicit non-claims

A verified M9 can claim only a named QEMU TCG trace containing one notification-register write after a zeroed unpublished one-slot layout and immediate reset, plus unchanged disposable media. It cannot claim a usable queue, descriptor publication, completion, interrupt, DMA behavior or containment, IOMMU/IOSpace policy, hardware safety, block request, block read/write/flush, persistent VFS, storage server, Linux block/KAPI compatibility, `dpkg` or `apt`.

## Promotion rule

Promotion requires one fresh QEMU trace and SHA-bound all-zero raw fixture, an independent verifier that rejects descriptor/avail/used writes and any extra notification write, a 19-profile rebuild, standalone evidence regression and compatibility-matrix update. The next stage must separately prove either bounded device completion observation or DMA/IOMMU policy; it must not infer either from M9.

## References

[1] [OASIS, *Virtual I/O Device (VIRTIO) Version 1.3*](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html)
