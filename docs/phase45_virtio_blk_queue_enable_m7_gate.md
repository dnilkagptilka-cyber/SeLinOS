# Phase 45: virtio-blk root queue-enable/reset M7 gate

## Status

**Status: verified, bounded M7 proof.** The default-OFF x86_64/PC99 QEMU TCG `SeLinRootVirtioBlkQueueEnableProbe=ON` profile has independently evidenced one narrow queue-zero transition in the QEMU modern `virtio-blk-pci` fixture (`1af4:1042`): after an unpublished zeroed M6-equivalent layout, `queue_enable` reads zero, is written/read back once as one, then reads zero after immediate device-status reset. The SHA-bound record is `tests/artifacts/selinos_virtio_queue_enable_m7.verification.json` and the independent checker is `tools/verify_virtio_blk_queue_enable_m7.py`.

> `queue_enable=1` is an activation-adjacent device-state change. This phase makes **no** claim that device DMA cannot occur, that DMA is contained, or that the experiment is safe for hardware deployment. It establishes only the stated QEMU TCG register read-backs, source exclusions, zeroed/unpublished layout, reset sequence, teardown, and unchanged disposable backing file.

The [Virtio 1.3 specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html) specifies that a reset device presents `queue_enable=0`; its queue reset wording also treats queue enable as a stateful activation field. The implementation must therefore require reset read-backs rather than infer a general lifecycle model.

## Closed transaction

Root repeats M6 exactly through its one ordinary temporary 4 KiB frame, all-zero one-slot split-ring layout, queue-zero selection, `queue_size=1`, and descriptor/driver/device address-pair read-backs. It additionally performs the following ordered sequence while the device is in the temporary zero-feature `DRIVER_OK` state:

| Step | Required action | Required read-back |
|---:|---|---|
| 1 | Read `queue_enable` | Exactly `0`. |
| 2 | Write `queue_enable=1` exactly once | Exactly `1`. |
| 3 | Write device status `0` exactly once for successful reset | Exactly `0`. |
| 4 | Read `queue_enable` after reset | Exactly `0`. |
| 5 | Release RAM/MMIO mappings, reservations and frames | No retained local mapping or frame. |

The queue remains zero-filled. No descriptor is offered through the available ring; no notification register/capability is mapped or written; no ISR, IRQ, MSI-X, PCI-command, bus-mastering, IOSpace or device-feature state is touched. The QEMU backing file must remain an all-zero 8 MiB file.

## Mandatory fail-closed controls

The common capability must extend through `queue_enable`. The helper must require an ordinary non-zero, page-aligned root frame physical address and in-frame split-ring offsets before enabling the queue. It must reset device status on every failure after the common mapping exists, and must release both temporary mappings, both reservations and both frame objects on every return. The independent verifier must prove there is precisely one write of `queue_enable=1`, no write of zero to `queue_enable`, no notification or descriptor publication, ordered reset/read-back, and no DMA or containment claim.

## Explicit non-claims

Even if verified, M7 demonstrates only a one-time QEMU queue-enable/read-back followed by reset for an unpublished zeroed queue. It does not establish a usable or safe virtqueue, notification, interrupt handling, bus mastering, DMA behavior or containment, IOMMU/IOSpace policy, descriptor consumption, block request, durable media operation, storage server, persistent filesystem, Linux block/KAPI compatibility, `dpkg`, or `apt`.

## Promotion rule

Promotion requires a fresh QEMU trace, unchanged raw fixture, SHA-bound record, independent verifier, all-profile rebuild and standalone evidence-regression suite. A later phase must separately establish notification policy and observable device behavior; no Phase 45 evidence can be reused as a DMA, I/O or hardware-safety assertion.
