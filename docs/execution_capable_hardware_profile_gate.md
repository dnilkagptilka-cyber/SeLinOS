# SeLinOS execution-capable hardware/profile gate

**Decision:** the existing QEMU TCG profile remains the reproducible functional-development profile, but it is insufficient for hardware DMA-containment evidence because its boot log reports `ACPI: 0 IOMMUs detected`. The next containment profile must be **real PC99 x86_64 hardware** with an Intel VT-d-capable chipset, firmware VT-d enabled, a serial console and a controlled test PCI device.

seL4 lists PC99 x64 with VT-D support and documents that x86 images can run both in simulation and on hardware.[1] Its configuration guidance cautions that experimental combinations can have undocumented incompatibilities and should be validated with baseline tests.[2] Recent Microkit guidance independently shows that x86_64 IOMMU setup is an explicit configuration obligation: DMA users stop working if IOMMU setup is omitted, and the reference profile exposes an IOMMU address-space element and a DMA test example.[3]

| Requirement | QEMU TCG development profile | Required Phase 10 target profile | Acceptance record |
|---|---|---|---|
| x86_64 boot and serial output | Verified | Required | Boot log, pinned image hash and serial capture. |
| IOMMU hardware detection | Fails: `ACPI: 0 IOMMUs detected` | Required: VT-d detected by seL4 at boot | ACPI/boot record and generated configuration hash. |
| IOSpace authority | Generated API exists; unexercised | Required for a selected PCI BDF | Root mint record: nonzero domain ID + exact BDF. |
| DMA containment | Not provable | Required | IO-page-table map, allow-map, deny-map, unmap/invalidation and memory-reuse proofs. |
| Native executable mappings | API gate remains unresolved | Required only after an auditable W^X/NX mechanism exists | Positive/negative mapping test plus loader lifetime proof. |
| NIC/storage driver delegation | Not provided | Deferred until the preceding proofs pass | Exact resource delegation and teardown verifier. |

> **Independent W^X condition:** moving from QEMU to real hardware does not by itself repair the current execute-control gap. The selected target may supply CPU NX, but SeLinOS cannot claim W^X until the pinned seL4 userspace mapping interface exposes and the system verifies an auditable execute-permission policy.

## Target admission checklist

The target candidate must have Intel VT-d enabled in firmware, a usable serial console, known CPU and chipset identifiers, a dedicated noncritical PCI test function, and a recovery method. Before granting any driver-domain MMIO, IRQ or DMA authority, SeLinOS must capture the initial ACPI/IOMMU discovery information, demonstrate a root-only IOSpace mint for that exact BDF, and show that rejected mappings do not become device-visible. A QEMU functional test may remain part of CI, but it cannot substitute for a detected-IOMMU record.

## References

[1]: https://docs.sel4.systems/Hardware/ "seL4 supported platforms"
[2]: https://docs.sel4.systems/projects/sel4/configurations.html "seL4 configurations"
[3]: https://docs.sel4.systems/releases/microkit/2.3.0.html "Microkit Release 2.3.0"
