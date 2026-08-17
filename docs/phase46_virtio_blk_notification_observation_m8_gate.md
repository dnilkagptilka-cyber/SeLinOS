# Phase 46: virtio-blk root notification-capability observation M8 gate

## Status

**Status: verified, bounded M8 proof.** The default-OFF x86_64/PC99 QEMU TCG `SeLinRootVirtioBlkNotificationObservationProbe=ON` profile has independently evidenced one notification-capability parse, BAR-range validation and queue-zero scalar notification-address calculation in the QEMU modern `virtio-blk-pci` fixture (`1af4:1042`). It starts from a reset device and deliberately does not reuse M7 activation state. The SHA-bound record is `tests/artifacts/selinos_virtio_notification_observation_m8.verification.json` and the independent checker is `tools/verify_virtio_blk_notification_observation_m8.py`.

The [Virtio 1.3 specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html) locates notification through `VIRTIO_PCI_CAP_NOTIFY_CFG`, whose capability payload contains a transport-specific notification offset multiplier. Queue selection exposes `queue_notify_off`; the scalar notification location is derived as the notification capability base plus `queue_notify_off × notify_off_multiplier`.[1]

> Phase 46 does **not** map the notification BAR/page and does **not** write a notification. It makes no claim about enabled queues, DMA behavior, DMA absence, DMA containment, IOMMU/IOSpace policy, interrupts, bus mastering, descriptor consumption, block I/O or hardware safety.

## Closed transaction

| Step | Required bounded behavior |
|---:|---|
| 1 | Re-run modern virtio identity, PCI vendor-capability traversal and common-config range validation. |
| 2 | Find exactly one valid notification capability with a non-zero multiplier; validate its BAR selector, offset, length, overflow-safe BAR range and scalar physical base. |
| 3 | Temporarily map **only the already validated common-config page** root-locally, reset status, stage zero-feature `DRIVER_OK`, select queue zero and read `queue_notify_off`. |
| 4 | Compute `notification_paddr = notification_bar_paddr + notification_cap_offset + queue_notify_off × notify_off_multiplier` with multiplication/addition overflow checks and require it inside the validated notification capability range. |
| 5 | Reset device status to zero and release common mapping, reservation and frame. The notification BAR is never framed, mapped, read or written. |

The profile writes `queue_select=0` exactly once only to select the observed queue. It must write no driver-feature selector/word, `queue_size`, queue address field, `queue_enable`, notification location, ISR/IRQ/MSI-X register, PCI command bit or bus-mastering control. It must allocate no layout frame, IOSpace/DMA object or driver-domain capability. The disposable 8 MiB QEMU raw file must remain all zero.

## Fail-closed acceptance conditions

The capability parser rejects malformed list traversal, wrong cfg type, invalid BAR index, zero multiplier, zero length, page/range overflow, notification address overflow and notification address outside the named capability window. The selected common queue must be zero and have a non-zero maximum size; aside from its single selector write, no queue configuration field may be written. The helper resets status on every failure after a common mapping exists and releases its own temporary frame, reservation and mapping on every return.

## Explicit non-claims

A verified M8 may claim only root-owned notification-capability metadata, queue-zero notification-offset observation and one bounded calculated physical notification address in the named QEMU fixture. It cannot claim notification delivery, a mapped notification register, queue activation, a usable virtqueue, descriptor publication, IRQ, DMA behavior or containment, IOMMU, block request, durable media access, persistent VFS, storage service, Linux block/KAPI compatibility, `dpkg` or `apt`.

## Promotion rule

Promotion requires a fresh QEMU trace, unchanged raw fixture, SHA-bound evidence record, independent verifier, all-profile rebuild and standalone regression. The next phase must treat a notification write as a separate activation-adjacent operation and must not derive a DMA/I/O claim from this observation proof.

## References

[1] [OASIS, *Virtual I/O Device (VIRTIO) Version 1.3*](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html)
