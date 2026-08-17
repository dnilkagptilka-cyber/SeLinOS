# SeLinOS Phase 11 VM/virtio checkpoint — 2026-08-14

The active default PC99/QEMU-TCG production image is:

```text
740705c29bb3a98d3ed838d9273ad268f97de4a34be19c0e969d3bd5363217d0
```

Two separate, explicit opt-in QEMU storage proofs are now available. **M0** discovers the modern QEMU virtio block PCI identity `1af4:1042` with temporary root PCI configuration authority only. **M1** walks the finite PCI capability list and validates metadata for a modern common-configuration capability: type, BAR index, 4-byte aligned offset and bounded length. Both proofs withhold BAR mapping, device status/features, queues, IRQ, DMA, block I/O and storage-domain startup.

The default build has both virtio experiment switches off. Current Q35 vIOMMU experiments remain non-admissible for containment evidence: under TCG the pinned seL4 boot log reports `ACPI: 0 IOMMUs detected`, and this sandbox has no `/dev/kvm` for the documented KVM split-irqchip profile.

The next candidate is an M2 **root-only BAR-range and feature-observation** proof. It must first validate that every common-configuration address lies wholly inside the selected BAR. It must perform no feature or status writes, create no queue and issue no DMA/IRQ or block request. The detailed stop rule is in `docs/virtio_blk_status_feature_m2_gate.md`.

The checkpoint image hash was rechecked after all documentation updates and remains `740705c29bb3a98d3ed838d9273ad268f97de4a34be19c0e969d3bd5363217d0`.

Implementation review confirms that the proven EDU path maps full BAR frames into a driver process and coordinates IRQ/DMA lifecycle. That model is intentionally too strong for virtio M2. Any virtio common-config read experiment must instead establish a distinct root-only, temporary and bounds-checked mapping path and revoke it before driver-domain delegation is even considered.

A source scan found no existing root-local device-MMIO temporary map helper. Existing full-BAR frame allocation and mapping occur in the EDU driver-domain flow, so virtio M2 requires a separate root-local helper with a fixed temporary virtual range, page-aligned BAR bounds validation and mandatory unmap/object reclamation.

Root bootstrap has a proven `sel4utils_dup_and_map`/`sel4utils_unmap_dup` temporary mapping pattern for an already-owned user frame. However, the pinned implementation hardcodes `cacheable=1`, so it is not admissible for device MMIO. M2 must instead reserve and map an explicit root-local VSpace range with `cacheable=0`, after it admits a page-aligned physical BAR frame through a separate root policy helper.

The pinned local VKA exposes `vka_alloc_frame_at(vka, size_bits, paddr, frame)`, and the existing EDU path already uses it for physical device frames. This is the concrete primitive M2 can use only after it validates the virtio BAR base, size, capability offset and page-aligned subrange; the frame cap must remain root-owned and be freed after the temporary map.

The M2 design gate now explicitly binds temporary root mapping to the local VKA frame-admission primitive: the selected physical interval must be page-aligned and wholly inside the validated virtio BAR/common configuration; no frame cap may be copied outside root.

The pinned `sel4utils_unmap_dup` teardown sequence is retained as an ownership model only: `vspace_unmap_pages(..., VSPACE_PRESERVE)`, delete the temporary cap, free its CSpace slot, then separately free the root-owned `vka_object_t` device frame. M2 must perform this sequence around an explicit uncached VSpace mapping rather than call the cached duplicate-map helper. Any failure must remain fail-closed.

The active default build configuration was rechecked: both `CONFIG_SELINOS_ROOT_VIRTIO_BLK_DISCOVERY_PROBE` and `CONFIG_SELINOS_ROOT_VIRTIO_BLK_CAPABILITY_PROBE` are disabled.

Local QEMU 8.2.2 confirms `virtio-blk-pci` supports `disable-legacy`, `disable-modern`, `queue-size`, `iommu_platform` and `ats`. The M2 fixture remains `disable-legacy=on`; it leaves `iommu_platform`/`ats` off because current VM containment evidence is absent, and it does not instantiate queues.

## M2 completion update

The dedicated `SeLinRootVirtioBlkBarFeatureProbe=ON` image has now passed its independent VM evidence verifier: `tools/verify_virtio_blk_bar_feature_m2.py`. The QEMU 8.2.2 modern non-legacy `virtio-blk-pci` fixture exposes its common configuration through **BAR4, a 64-bit prefetchable memory BAR**. The initial 32-bit-only validator rejected it before any device access; this was a correct fail-closed result. The completed validator now has separate bounded 32-bit and 64-bit paths. Each backs up the original BAR word(s) and PCI command word, quiesces only memory decoding and bus mastering, executes the all-ones sizing transaction, requires exact BAR read-back after restoration, and restores the command word before returning.

The M2 helper validates that the declared common-configuration interval, including the two read offsets, lies wholly inside the sized BAR. It admits only one page-aligned physical frame, reserves and maps it **uncached and read-only in root’s own VSpace**, reads only `device_feature_select` and `device_feature`, and then performs `vspace_unmap_pages(..., VSPACE_PRESERVE)`, frees the reservation and frees the root-owned frame object. It neither writes the selector nor claims full feature enumeration. No frame cap crosses the root boundary.

The M2 serial evidence states both the successful root-only observation and the explicit absence of BAR-cap delegation, feature acceptance, status write, queue, IRQ, DMA and block I/O. This remains a VM/QEMU functional observation only; it supplies neither a storage driver domain nor IOMMU containment evidence. The default production image remains separate with all three virtio experiment gates disabled.
