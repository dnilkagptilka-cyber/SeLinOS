# Phase 45 external source notes

## Authoritative Virtio transport reference

The Phase 45 queue-enable/reset boundary uses the OASIS Virtio 1.3 specification:

- URL: https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html
- Accessed: 2026-08-17
- Relevant scope: modern PCI common configuration, `queue_enable`, device reset, queue reset and virtqueue lifecycle.

The OASIS search result for this document states that the device **MUST present `0` in both `queue_reset` and `queue_enable` when queue reset has completed**. Phase 45 uses only the conservative subset that a device-status reset must be read back as zero and that queue-enable is read back as zero after that reset. It does not infer general queue reset support, DMA safety, device-DMA absence, IOMMU containment or hardware deployability.

## Local phase policy derived from the source

The implementation permits exactly one `queue_enable=1` write after a zero-feature `DRIVER_OK` state and M6’s zero-filled, unpublished one-slot split-ring setup. It then resets device status immediately and requires `queue_enable=0`. No notification, descriptor publication, IRQ, PCI command/bus-mastering write, IOSpace/DMA authority or block request is permitted. The QEMU raw backing file must remain unchanged.

## Phase 46 notification source note

The same OASIS Virtio 1.3 document states that the notification location is found through the `VIRTIO_PCI_CAP_NOTIFY_CFG` capability, followed by the transport-specific multiplier field. Phase 46 may therefore parse and validate that capability, select queue zero, read its notification offset and calculate a bounded scalar address. It must not map the notification page or write the notification location; notification itself is a later activation boundary. Source URL: https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html (accessed 2026-08-17).
