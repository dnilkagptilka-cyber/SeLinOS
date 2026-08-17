# Phase 42: virtio-blk root queue-zero observation M5 gate

## Status

**Status: verified as a separate root-only QEMU TCG proof.** On x86_64/PC99 image `47b7834ef306e3abf219fe268a180dea4d6d0ee1e4aa59a3190739f7c08f17fc`, built with `SeLinRootVirtioBlkQueueZeroProbe=ON`, `verify_virtio_blk_queue_zero_m5.py` SHA-binds the PCI helper/header, root wiring/main, CMake gate, disposable raw fixture, image and fresh serial trace. M5 depends on the verified root-only zero-feature M4 `DRIVER_OK`/reset proof. It is metadata observation needed before any later queue-layout gate; it is not queue setup, a virtqueue, DMA or a block driver.

> M5 may select only queue index zero while the fixed zero-feature device is in the temporary `DRIVER_OK` state, read a non-zero queue size, then reset. It must not write queue size, queue addresses, queue enable or a notification register.

## Closed transaction

Root repeats M1 capability inspection and M2 range validation. In one temporary root-only common-config mapping, it executes the verified M4 status sequence through `DRIVER_OK`, writes `queue_select = 0`, reads `queue_size`, requires it to be non-zero, then writes device status zero and reads back zero. The driver feature set remains zero. No queue field other than `queue_select` is written.

| Permitted root behavior | Explicitly excluded |
|---|---|
| One `queue_select=0` MMIO write and one `queue_size` read under temporary root-only `DRIVER_OK` state. | Queue size/address/enable writes, descriptor/driver/device ring memory, notification, ISR/IRQ, DMA, bus mastering, block request, retained state. |
| Temporary mapping/frame/reservation allocation and teardown. | Device/frame/capability delegation, a storage domain, persistent media use, filesystem, Linux block/KAPI, `dpkg` or `apt`. |

## Mandatory controls

The common configuration must be sufficiently long for the status, queue-select and queue-size fields before mapping. The helper must record exactly queue index zero and the observed non-zero queue size, reset on both success and failure, and release mapping/frame/reservation. The independent verifier must isolate this M5 helper source, require exactly one queue-select write and no forbidden queue-control/notification/IRQ/DMA/block source, bind the fresh QEMU trace and prove no disk modification.

## Promotion rule

The verified M5 may claim only root-owned queue-zero size metadata observation under a temporary zero-feature `DRIVER_OK` state followed by reset. It cannot claim a configured/enabled virtqueue, DMA, I/O, persistence, a storage service, Linux block/KAPI compatibility, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_virtio_blk_queue_zero_m5.verification.json`, checked by `./tools/verify_virtio_blk_queue_zero_m5.py` as part of the final 48-verifier regression suite.
