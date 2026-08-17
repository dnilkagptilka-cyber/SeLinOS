# Phase 41: virtio-blk root driver-ready/reset M4 gate

## Status

**Status: verified as a separate root-only QEMU TCG proof.** On x86_64/PC99 image `9019dac39219ba4d9e89db8a993b830de6102af341095821cb27114fe61fd658`, built with `SeLinRootVirtioBlkDriverReadyProbe=ON`, `verify_virtio_blk_driver_ready_m4.py` SHA-binds the PCI helper/header, root wiring/main, CMake gate, disposable raw fixture, image and fresh serial trace. M4 depends on the verified root-only M3 zero-feature `FEATURES_OK` stage. It is a strictly temporary device-state proof, not a block driver, storage service or persistent-VFS implementation.

> M4 may extend the fixed M3 zero-feature status sequence with a single `DRIVER_OK` write/read-back, then must reset the device to zero before tearing down its temporary root-only mapping. It does not instantiate a virtqueue or issue I/O.

## Closed transaction

Root repeats M1 common-capability inspection and M2 bounded BAR validation, maps the one common page only in root, and follows this exact sequence:

`0 → ACKNOWLEDGE → ACKNOWLEDGE|DRIVER → ACKNOWLEDGE|DRIVER|FEATURES_OK → ACKNOWLEDGE|DRIVER|FEATURES_OK|DRIVER_OK → 0`.

The driver feature set remains zero. Every staged status must read back exactly. Any failure causes a best-effort reset, followed by mapping/frame/reservation teardown and failure. The final successful reset must read back zero.

| Permitted root behavior | Explicitly excluded |
|---|---|
| Temporary common-config mapping, six status writes/read-backs including `DRIVER_OK` and reset. | Feature selector/driver-feature writes, queue select/size/address/enable, queue notification, ISR/IRQ, DMA, bus mastering, block request or retained device state. |
| Root-only device state with a disposable raw QEMU backing file. | Capability/frame delegation, a storage domain, durable media use, filesystem, package database, `dpkg` or `apt`. |

## Mandatory evidence

The profile is default-OFF and must be separate from M0–M3. The independent verifier must SHA-bind the image, root PCI helper/header/wiring, CMake gate, fixture and QEMU trace; it must confirm `DRIVER_OK` appears once only in the M4 helper, that the final stage is reset to zero, and that no queue/DMA/block APIs or non-root device authority appear.

## Promotion rule

The verified M4 may claim only one root-owned zero-feature `DRIVER_OK` read-back followed by reset for the fixed QEMU virtio-blk fixture. It cannot claim a usable virtio driver, virtqueue, DMA/IOMMU, block I/O, persistence, Linux block/KAPI compatibility, `dpkg` or `apt`.

The promotion evidence is `tests/artifacts/selinos_virtio_blk_driver_ready_m4.verification.json`, checked by `./tools/verify_virtio_blk_driver_ready_m4.py` as part of the final 47-verifier regression suite.
