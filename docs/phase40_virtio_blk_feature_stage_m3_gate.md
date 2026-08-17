# Phase 40: virtio-blk root feature-stage M3 gate

## Status

**Status: verified as a separate root-only QEMU TCG proof.** On x86_64/PC99 image `6ae3a581f194e0cec26cd019dc56593771e2233f1ecbc164e18f669d1e12e817`, built with `SeLinRootVirtioBlkFeatureStageProbe=ON`, `verify_virtio_blk_feature_stage_m3.py` SHA-binds the PCI helper/header, root wiring/main, CMake gate, disposable raw fixture, image and fresh serial trace. This gate follows the existing root-only virtio M0 identity, M1 capability inspection and M2 read-only BAR/feature observation proofs. It is a prerequisite for later queue and persistent-storage gates; it is not a storage driver or VFS backend.

> M3 may reset one QEMU modern virtio-blk device, set only the exact ACKNOWLEDGE, DRIVER and FEATURES_OK status sequence with a zero driver-feature set, validate the device acceptance bit, and then reset to status zero. It must never set DRIVER_OK.

## Closed root-only sequence

The default-OFF profile must use the documented QEMU `virtio-blk-pci,disable-legacy=on` fixture with a disposable raw backing file. Root alone repeats capability and bounded BAR validation, maps only one validated common-config page in its own VSpace, writes the status field `0 → ACKNOWLEDGE → ACKNOWLEDGE|DRIVER → ACKNOWLEDGE|DRIVER|FEATURES_OK`, reads back the last status and requires `FEATURES_OK`, then writes zero and reads back zero. No driver feature selector or driver feature word is written; the profile's negotiated driver feature set is exactly zero.

| Authority | M3 permitted behavior | Explicitly excluded |
|---|---|---|
| Root | Temporary validated common-config MMIO mapping, four status writes/read-backs, explicit reset and teardown. | BAR cap delegation, DRIVER_OK, queue selection/size/address/enable, notification, ISR/IRQ, DMA, bus mastering, block request, retained frame or device state. |
| All non-root domains | None. | Any device, frame, mapping, IRQ, DMA, PCI, I/O or IOSpace authority. |

## Mandatory controls

The helper must validate the common configuration range before mapping and must use a bounded writable device mapping only in root. It must write only the status register; any error, rejected FEATURES_OK read-back or reset-read-back mismatch fails closed and tears down the temporary mapping/frame/reservation. The final status must be zero on every successful path. The root does not set PCI bus-mastering, create a virtqueue, allocate queue memory or issue a block command.

The independent verifier must SHA-bind the opt-in image, PCI helper, root wiring, header/protocol, CMake gate and fresh QEMU trace. It must reject `DRIVER_OK`, queue, notify, IRQ, DMA, block request, frame delegation and non-root device source. It must bind an ordered trace for common-capability validation, zero-feature status staging, accepted FEATURES_OK and final reset.

## Promotion rule

The verified M3 may claim only root-owned temporary status staging and reset for the fixed QEMU virtio-blk fixture. It cannot claim device activation for I/O, a usable block device, virtqueue, DMA/IOMMU, persistent VFS, Linux block/KAPI compatibility, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_virtio_blk_feature_stage_m3.verification.json`, checked by `./tools/verify_virtio_blk_feature_stage_m3.py` as part of the final 46-verifier regression suite.
