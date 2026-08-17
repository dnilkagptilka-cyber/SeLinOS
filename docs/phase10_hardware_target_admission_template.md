# SeLinOS Phase 10 hardware target admission record

**Status:** blank evidence template. Completing fields does not approve a target; all required artifacts must be captured and independently checked before device authority is delegated.

| Field | Required value/evidence |
|---|---|
| Target identifier | Non-secret lab asset or machine label. |
| CPU and chipset | Exact vendor/model and chipset/IOMMU capability. |
| Firmware settings | VT-d/IOMMU enabled; relevant virtualization and Secure Boot state recorded. |
| seL4 build | Pinned commit, complete CMake cache/config hash and image SHA-256. |
| Boot transport | Serial console parameters and recovery/rollback method. |
| ACPI/IOMMU discovery | Full boot log showing IOMMU detection, not merely `CONFIG_IOMMU=1`. |
| Test PCI BDF | Dedicated noncritical device; vendor/device/class/BDF discovery capture. |
| IOSpace mint | Exact nonzero domain ID and encoded BDF, root-only derivation audit. |
| Mapping tests | Positive allowed 4 KiB IO mapping and negative rejected mapping results. |
| Teardown tests | IO unmap/invalidation before frame reuse; device disabled before authority removal. |
| W^X tests | Only after an audited mapping interface supports execute policy: positive/negative page mapping evidence. |
| Verifier update | New independent verifier, machine-readable record and full regression run. |

> **Stop rule:** if IOMMU discovery is absent, PCI identity differs from the test record, an IOMMU map/unmap operation fails unexpectedly, or the W^X mechanism is not auditable, do not grant driver-domain BAR, IRQ or DMA authority. Preserve the log and update the relevant gate instead.
