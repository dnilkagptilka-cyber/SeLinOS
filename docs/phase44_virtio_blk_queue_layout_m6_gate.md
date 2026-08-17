# Phase 44: virtio-blk root queue-memory-layout M6 gate

## Status

**Status: verified, bounded M6 proof.** Starting from the separately verified M5 root-only queue-zero size observation, the default-OFF x86_64/PC99 QEMU TCG `SeLinRootVirtioBlkQueueLayoutProbe=ON` profile has independently evidenced one root-owned, one-slot split-ring layout with common-config queue-size and address-pair read-backs while `queue_enable` remains zero. The QEMU modern `virtio-blk-pci` fixture is `1af4:1042` and no driver feature is negotiated. The SHA-bound record is `tests/artifacts/selinos_virtio_queue_layout_m6.verification.json` and the independent checker is `tools/verify_virtio_blk_queue_layout_m6.py`.

> Phase 44 remains deliberately below the DMA and queue-activation boundary. It does not claim DMA safety or absence of device DMA on hardware. The proof requires only that the code performs no queue-enable write, notification, IRQ, bus-mastering, IOSpace, DMA-allocation, descriptor publication, or block-I/O operation and that the QEMU fixture remains unmodified.

The modern PCI common-config fields and queue lifecycle are constrained by the [Virtio 1.3 specification](https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html). The project binds exact field offsets in source and validates capability length before touching a field.

## Closed transaction

Root repeats the M1 common-capability inspection, M2 range validation, and M4 zero-feature status sequence through temporary `DRIVER_OK`. It then selects queue zero and reads its maximum non-zero size. It allocates exactly one ordinary root-owned 4 KiB frame, obtains its physical address through the VKA allocator, maps it only in the root VSpace, and writes a zero-filled split-ring layout for exactly **one** descriptor slot.

| Region | Physical offset in the one 4 KiB root-owned frame | Bytes | Required content |
|---|---:|---:|---|
| Descriptor table | `0` | `16` | One all-zero `virtq_desc`; no address, length, flags or next field is populated. |
| Driver/available area | `16` | `6` | All-zero flags, index and one available-ring entry; no descriptor is offered. |
| Device/used area | `24` | `12` | All-zero flags, index and one used-ring entry; no completion is consumed. |

The selected queue size is written exactly once as `1` and must read back as `1`. The proof writes only the three 64-bit physical address pairs: descriptor at `frame_paddr + 0`, driver area at `frame_paddr + 16`, and device area at `frame_paddr + 24`. Each read-back must match exactly. The layout is valid only if its allocated frame physical address is non-zero, 4 KiB-aligned, and every addressed byte lies within the one allocated 4 KiB frame.

The transaction reads `queue_enable` before and after address programming and requires zero on both reads. It must **never write** `queue_enable`, `queue_notify_off`, a notify capability/register, an MSI-X vector, a PCI command register, a driver-feature selector/value, or any queue field other than queue-select, queue-size and the six address halves. It resets device status to zero, verifies the reset read-back, unmaps the root RAM mapping and releases the frame and reservation on every exit path.

## Required common-config offsets

The source must explicitly bind the following modern common-config byte offsets and require capability length through the last address field:

| Field | Offset | Access in M6 |
|---|---:|---|
| `queue_select` | `0x16` | Write `0`, then read-back. |
| `queue_size` | `0x18` | Read maximum; write/read-back `1`. |
| `queue_enable` | `0x1c` | Read-only assertion that it remains `0`. |
| `queue_desc_lo`, `queue_desc_hi` | `0x20`, `0x24` | One low/high physical-address pair with exact read-back. |
| `queue_driver_lo`, `queue_driver_hi` | `0x28`, `0x2c` | One low/high physical-address pair with exact read-back. |
| `queue_device_lo`, `queue_device_hi` | `0x30`, `0x34` | One low/high physical-address pair with exact read-back. |

## Explicit non-claims

A verified M6 may claim only one temporary root-owned queue-zero split-ring memory layout with all-zero contents, queue-size/address programming, queue-enable-zero read-backs and full local teardown in the named QEMU TCG fixture. It does **not** claim an enabled/configured virtqueue, driver activation, notification, ISR/IRQ, bus mastering, DMA, IOMMU containment, block requests, durable media access, a storage server, filesystem, Linux block/KAPI compatibility, `.ko` execution, `dpkg`, or `apt`.

## Promotion rule

Promotion requires a fresh QEMU serial trace, an unchanged disposable raw disk fixture, a SHA-bound evidence record and an independent verifier. The verifier must prove the exact source order, frame/layout bounds, queue-enable non-write policy, status reset, mapping/frame teardown, forbidden-control absence and the ordered runtime witness. The next phase must separately establish a queue-enable policy and cannot reuse this phase as a DMA or I/O claim.
